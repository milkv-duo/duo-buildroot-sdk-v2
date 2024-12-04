#include <common.h>
#include <command.h>
#include <cvi_boot_mode_nand.h>
#include <linux/mtd/mtd.h>
#include <nand.h>
/* malloc */
#include <malloc.h>
#include <env.h>
/* crc32 */
#include <u-boot/crc.h>
/* MISC_START */
#include <cvipart.h>
#include <cvi_update.h>

#define MAX_BOOT_TRY "1"

/**
 * Check nand r/w/e parameters.
 */
static int check_off_size(loff_t off, size_t size, loff_t *maxsize_p,
                          uint64_t chipsize)
{
    if (off >= chipsize) {
        puts("Offset exceeds device limit\n");
        return -1;
    }

    *maxsize_p = chipsize - off;

    if (size > *maxsize_p) {
        puts("Size exceeds partition or device limit\n");
        return -1;
    }

    return 0;
}

/**
 * Check whether the slot can be booted.
 */
static bool slot_is_bootable(AvbABSlotData *slot)
{
    return (slot->priority > 0) &&
           (slot->successful_boot || (slot->tries_remaining > 0));
}

static AvbABFlowResult fill_ab_data(struct AvbABData* ab_data)
{
    AvbABFlowResult ret;

    if (   env_get("priority_a")        == NULL
        || env_get("tries_remaining_a") == NULL
        || env_get("successful_boot_a") == NULL
        || env_get("priority_b")        == NULL
        || env_get("tries_remaining_b") == NULL
        || env_get("successful_boot_b") == NULL
        || env_get("boot_slot")         == NULL) {
        printf("[%s] WARNING ab_data is incorrectness\n", __FUNCTION__);
        ret = AVB_AB_FLOW_RESULT_ERROR;
        goto out;
    }

    ab_data->boot_slot                =
        (uint8_t)simple_strtoul(env_get("boot_slot")        , NULL, 10);

    ab_data->slots[0].priority        =
        (uint8_t)simple_strtoul(env_get("priority_a")       , NULL, 10);
    ab_data->slots[0].tries_remaining =
        (uint8_t)simple_strtoul(env_get("tries_remaining_a"), NULL, 10);
    ab_data->slots[0].successful_boot =
        (uint8_t)simple_strtoul(env_get("successful_boot_a"), NULL, 10);

    ab_data->slots[1].priority        =
        (uint8_t)simple_strtoul(env_get("priority_b")       , NULL, 10);
    ab_data->slots[1].tries_remaining =
        (uint8_t)simple_strtoul(env_get("tries_remaining_b"), NULL, 10);
    ab_data->slots[1].successful_boot =
        (uint8_t)simple_strtoul(env_get("successful_boot_b"), NULL, 10);

    ret = AVB_AB_FLOW_RESULT_OK;
out:
    return ret;
}

static AvbABFlowResult set_and_save_env(struct AvbABData* ab_data)
{
    AvbABFlowResult ret;
    char boot_slot[2], priority_a[3], priority_b[3], tries_remaining_a[2],
         tries_remaining_b[2], successful_boot_a[2], successful_boot_b[2];

    sprintf(boot_slot        , "%d", ab_data->boot_slot               );
    sprintf(priority_a       , "%d", ab_data->slots[0].priority       );
    sprintf(tries_remaining_a, "%d", ab_data->slots[0].tries_remaining);
    sprintf(successful_boot_a, "%d", ab_data->slots[0].successful_boot);
    sprintf(priority_b       , "%d", ab_data->slots[1].priority       );
    sprintf(tries_remaining_b, "%d", ab_data->slots[1].tries_remaining);
    sprintf(successful_boot_b, "%d", ab_data->slots[1].successful_boot);

    /* set env */
    if (   env_set("boot_slot"        , boot_slot        ) != 0
        || env_set("priority_a"       , priority_a       ) != 0
        || env_set("tries_remaining_a", tries_remaining_a) != 0
        || env_set("successful_boot_a", successful_boot_a) != 0
        || env_set("priority_b"       , priority_b       ) != 0
        || env_set("tries_remaining_b", tries_remaining_b) != 0
        || env_set("successful_boot_b", successful_boot_b) != 0) {
        printf("[%s] WARNING: set env fail\n", __FUNCTION__);
        ret = AVB_AB_FLOW_RESULT_ERROR;
        goto out;
    }

    /* save env */
    if(env_save() != 0) {
        printf("[%s] WARNING: save env fail\n", __FUNCTION__);
        ret = AVB_AB_FLOW_RESULT_ERROR_IO;
        goto out;
    }

    ret = AVB_AB_FLOW_RESULT_OK;
out:
    return ret;
}


/**
 * Transform slot to boot_mode_t.
 */
static boot_mode_t set_init_env(bool slot_suffix)
{
    boot_mode_t boot_mode = BOOT_MODE_INVALID;

    if (slot_suffix == 0) {
        debug("use slot A\n");
        boot_mode = BOOT_MODE_A;
    } else if (slot_suffix == 1) {
        debug("use slot B\n");
        boot_mode = BOOT_MODE_B;
    }

    return boot_mode;
}

/**
 * Reads the A/B bootmode from the env and determines which slot
 * should be booted. The result is stored in the select_slot.
 */
static AvbABFlowResult avb_get_current_slot(bool *select_slot,
                                            struct AvbABData *ab_data)
{
    AvbABFlowResult ret;

    /* select boot slot */
    if (   slot_is_bootable(&ab_data->slots[0])
        && slot_is_bootable(&ab_data->slots[1])) {
        if (ab_data->slots[1].priority > ab_data->slots[0].priority) {
            *select_slot = 1;
        } else {
            *select_slot = 0;
        }
    } else if (slot_is_bootable(&ab_data->slots[0])) {
        *select_slot = 0;
    } else if (slot_is_bootable(&ab_data->slots[1])) {
        *select_slot = 1;
    } else {
        printf("[%s] ERROR: No bootable slots found.\n", __FUNCTION__);
        ret = AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS;
        goto out;
    }

    ret = AVB_AB_FLOW_RESULT_OK;
out:
    return ret;
}

/**
 * Checks if the A/B bootmode in the uboot env is valid.
 */
static AvbABFlowResult avb_ab_slot_vertify(struct AvbABData *ab_data)
{
    AvbABFlowResult ret;

    /* fill AvbABData */
    ret = fill_ab_data(ab_data);
    if(ret != AVB_AB_FLOW_RESULT_OK) {
        printf("[%s] WARNING: ab data is not initialized\n", __FUNCTION__);
    }

    return ret;
}

/**
 * Initializes the A/B metadata in the uboot env.
 */
static AvbABFlowResult avb_ab_data_init(struct AvbABData *ab_data)
{
    AvbABFlowResult ret;

    /* init boot param, default is boot _a */
    env_set("priority_a"       , "15"        );
    env_set("priority_b"       , "14"        );
    env_set("successful_boot_a", "0"         );
    env_set("successful_boot_b", "0"         );
    env_set("tries_remaining_a", MAX_BOOT_TRY);
    env_set("tries_remaining_b", MAX_BOOT_TRY);
    /* 0(A), 1(B) */
    env_set("boot_slot"        , "0"         );

    /* write to flash */
    if(env_save() != 0) {
        printf("[%s] WARNING: save env fail\n", __FUNCTION__);
        ret = AVB_AB_FLOW_RESULT_ERROR_IO;
        goto out;
    }

    /* fill AvbABData */
    ret = fill_ab_data(ab_data);

out:
    return ret;
}

/**
 * Updates the A/B param in the uboot env.
 */
static AvbABFlowResult avb_ab_slot_update(bool slot_suffix,
                                          struct AvbABData *ab_data)
{
    /* tagged boot partition */
    ab_data->boot_slot = slot_suffix;

    /* the last boot was unsuccessful */
    if (!ab_data->slots[slot_suffix].successful_boot &&
        ab_data->slots[slot_suffix].tries_remaining > 0) {
        ab_data->slots[slot_suffix].tries_remaining -= 1;
    }
    /* the last boot was successful */
    if (ab_data->slots[slot_suffix].successful_boot) {
        ab_data->slots[slot_suffix].tries_remaining =
            (uint8_t)simple_strtoul(MAX_BOOT_TRY, NULL, 10) - 1;
        /**
         * We guarantee that boot_successful is 0 before each boot.
         * If the boot is successful, kernel will change it to 1.
         */
        ab_data->slots[slot_suffix].successful_boot = 0;
    }

    /* write back */
    return set_and_save_env(ab_data);
}

/**
 * The main logic of A/B boot mode.
 */
static boot_mode_t cvi_boot_flow(void)
{
    boot_mode_t boot_mode;
    bool slot_suffix;

    AvbABData *ab_data = (AvbABData*)malloc(sizeof(AvbABData));
    if (ab_data == NULL) {
        printf("[%s] ERROR: malloc ab data fail\n", __FUNCTION__);
        boot_mode = BOOT_MODE_INVALID;
        goto out;
    }

    /* check ab bootmode, if not exist, init it */
    if (avb_ab_slot_vertify(ab_data) != AVB_AB_FLOW_RESULT_OK) {
        if(avb_ab_data_init(ab_data) != AVB_AB_FLOW_RESULT_OK) {
            printf("[%s] ERROR: init ab data fail\n", __FUNCTION__);
            boot_mode = BOOT_MODE_INVALID;
            goto free;
        }
    }

    /* get current slot, (slot is 0(A) or 1(B)) */
    if (avb_get_current_slot(&slot_suffix, ab_data) != AVB_AB_FLOW_RESULT_OK) {
        boot_mode = BOOT_MODE_INVALID;
        goto free;
    }

    /* updata ab data */
    if (avb_ab_slot_update(slot_suffix, ab_data) != AVB_AB_FLOW_RESULT_OK) {
        /**
         * When updating ab_data fails, we simply inform the current
         * error without stopping the kernel from booting.
         */
        printf("[%s] WARNING: updata ab data fail\n", __FUNCTION__);
    }

    /* get boot mode */
    boot_mode = set_init_env(slot_suffix);

free:
    free(ab_data);
    ab_data = NULL;
out:
    return boot_mode;
}

/**
 * Main A/B nand boot cmd function.
 */
static int do_loadboot_nand(struct cmd_tbl *cmdtp, int flag, int argc,
                            char * const argv[])
{
    boot_mode_t boot_mode;
    loff_t part_offset = 0, maxsize = 0;
    size_t part_size = 0, r_size = 0;
    int check_ret;
    enum command_ret_t ret;
    struct mtd_info *mtd = get_nand_dev_by_index(nand_curr_device);
    void *uimage_addr = (void *)CVIMMAP_UIMAG_ADDR;

    /* A/B boot main */
    boot_mode = cvi_boot_flow();

    switch (boot_mode) {
    case BOOT_MODE_A:
        part_offset = simple_strtoul(env_get("BOOT_PART_OFFSET"), NULL, 16);
        part_size = simple_strtoul(env_get("BOOT_PART_SIZE"), NULL, 16);
        env_set("root", ROOTARGSA); // set rootfs parameter
        printf("[%s] INFO: boot mode A\n", __FUNCTION__);
        break;
    case BOOT_MODE_B:
        part_offset = simple_strtoul(env_get("BOOT_B_PART_OFFSET"), NULL, 16);
        part_size = simple_strtoul(env_get("BOOT_B_PART_SIZE"), NULL, 16);
        env_set("root", ROOTARGSB);  // set rootfs parameter
        printf("[%s] INFO: boot mode B\n", __FUNCTION__);
        break;
    default:
        printf("%s] ERROR: Unknown boot mode\n", __FUNCTION__);
        ret = CMD_RET_FAILURE;
        goto out;
    }

    /* read image to mm */
    r_size = part_size;
    if (check_off_size(part_offset, r_size, &maxsize, mtd->size) != 0)
        return CMD_RET_FAILURE;

    check_ret = nand_read_skip_bad(mtd, part_offset, &r_size,
                            NULL, maxsize,
                            (u_char *)uimage_addr);
    /* check and print */
    if (check_ret != 0 || r_size != part_size) {
        printf("[%s] ERROR: Could not read boot image\n", __FUNCTION__);
        ret = CMD_RET_FAILURE;
        goto out;
    } else {
        printf("load 0x%lx byte image data from 0x%llx to 0x%llx success!\n",
               r_size, part_offset, (loff_t)uimage_addr);
    }

    ret = CMD_RET_SUCCESS;
out:
    return ret;
}

U_BOOT_CMD(loadboot_nand, 1, 0, do_loadboot_nand,
    "loadboot - according to abdata load uimage from a or b partition\n",
    "loadboot - according to abdata load uimage from a or b partition\n");
