
#include <unistd.h>
#include <sys/prctl.h>
#include "cvi_vb.h"
#include "cvi_vi.h"
#include "cvi_isp.h"
#include "ctx.h"
#include "teaispdrc_helper.h"

#include "bmruntime_interface.h"

#define DRC_IN_IMG      (0)
#define DRC_IN_EMA      (1)
#define DRC_IN_COEFF    (2)
#define DRC_IN_NUM      (3)

#define DRC_OUT_IMG     (0)
#define DRC_OUT_EMA     (1)
#define DRC_OUT_NUM     (2)

#define DRC_OUT_VB_MAX  (2)

typedef struct {
	void *p_bmrt;
	const char **net_names;
	bm_tensor_t *input_tensors;
	bm_tensor_t *output_tensors;
	void **input_vaddr;
	int input_num;
	int output_num;
/*-----------------------------------*/
	pthread_t tid;
	uint8_t enable_drc_thread;
	uint8_t read_vb_index;
	uint8_t write_vb_index;
	int8_t vb_flag[DRC_OUT_VB_MAX];
	pthread_mutex_t vb_lock[DRC_OUT_VB_MAX];
	VB_BLK vb_blk[DRC_OUT_VB_MAX];
	uint64_t vb_phy_addr[DRC_OUT_VB_MAX];
	VIDEO_FRAME_INFO_S dst[DRC_OUT_VB_MAX];
/*-----------------------------------*/
	pthread_mutex_t param_lock;
	float drc_param[TEAISP_DRC_PARAM_LENGHT];
} TEAISP_DRC_CTX_S;

static bm_handle_t bm_handle;

static TEAISP_DRC_CTX_S *pdrc_ctx[SERVICE_CTX_ENTITY_MAX_NUM];

static void teaisp_drc_dump_input(TEAISP_DRC_CTX_S *ctx, int pipe)
{
	void *addr_tmp = NULL;
	int size = 0;
	FILE *fp = NULL;
	char path[128] = {0};

	snprintf(path, sizeof(path), "/tmp/drc_input%d_img.bin", pipe);
	size = ctx->input_tensors[DRC_IN_IMG].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		ctx->input_tensors[DRC_IN_IMG].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/drc_input%d_ema.bin", pipe);
	size = ctx->input_tensors[DRC_IN_EMA].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		ctx->input_tensors[DRC_IN_EMA].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/drc_input%d_coeff.bin", pipe);
	size = ctx->input_tensors[DRC_IN_COEFF].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		ctx->input_tensors[DRC_IN_COEFF].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	printf("Successfully written input_tensors to files!\n");
}

static void teaisp_drc_dump_output(TEAISP_DRC_CTX_S *ctx, int pipe)
{
	void *addr_tmp = NULL;
	int size = 0;
	FILE *fp = NULL;
	char path[128] = {0};

	snprintf(path, sizeof(path), "/tmp/drc_output%d_img.bin", pipe);
	size = ctx->output_tensors[DRC_OUT_IMG].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		ctx->output_tensors[DRC_OUT_IMG].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	snprintf(path, sizeof(path), "/tmp/drc_output%d_ema.bin", pipe);
	size = ctx->output_tensors[DRC_OUT_EMA].device_mem.size;
	addr_tmp = CVI_SYS_MmapCache(
		ctx->output_tensors[DRC_OUT_EMA].device_mem.u.device.device_addr, size);
	fp = fopen(path, "wb");
	fwrite(addr_tmp, size, 1, fp);
	fclose(fp);
	CVI_SYS_Munmap(addr_tmp, size);

	printf("Successfully written output_tensors to files!\n");
}

static int teaisp_drc_unload_model(int pipe)
{
	TEAISP_DRC_CTX_S *ctx = pdrc_ctx[pipe];

	pthread_mutex_destroy(&ctx->param_lock);

	for (int i = 0; i < DRC_OUT_VB_MAX; i++) {
		pthread_mutex_destroy(&ctx->vb_lock[i]);
	}

	for (int i = 0; i < ctx->input_num; i++) {

		if (i == DRC_IN_IMG) {
			continue;
		}

		if (ctx->input_vaddr && ctx->input_vaddr[i]) {
			bm_mem_unmap_device_mem(bm_handle,
				ctx->input_vaddr[i],
				(int) ctx->input_tensors[i].device_mem.size);
			ctx->input_vaddr[i] = NULL;
		}

		if (ctx->input_tensors) {
			if (ctx->input_tensors[i].device_mem.u.device.device_addr != 0) {
				bm_free_device(bm_handle, ctx->input_tensors[i].device_mem);
			}
		}
	}

	for (int i = 0; i < ctx->output_num; i++) {

		if (i == DRC_OUT_IMG) {
			continue;
		}

		if (ctx->output_tensors) {
			if (ctx->output_tensors[i].device_mem.u.device.device_addr != 0) {
				bm_free_device(bm_handle, ctx->output_tensors[i].device_mem);
			}
		}
	}

	for (int i = 0; i < DRC_OUT_VB_MAX; i++) {
		if (ctx->vb_blk[i] != VB_INVALID_HANDLE ||
			ctx->vb_blk[i] != 0x00) {
			CVI_VB_ReleaseBlock(ctx->vb_blk[i]);
			ctx->vb_blk[i] = VB_INVALID_POOLID;
		}
	}

	if (ctx->input_vaddr) {
		free(ctx->input_vaddr);
		ctx->input_vaddr = NULL;
	}

	if (ctx->input_tensors) {
		free(ctx->input_tensors);
		ctx->input_tensors = NULL;
	}

	if (ctx->output_tensors) {
		free(ctx->output_tensors);
		ctx->output_tensors = NULL;
	}

	if (ctx->net_names) {
		free(ctx->net_names);
		ctx->net_names = NULL;
	}

	if (ctx->p_bmrt) {
		bmrt_destroy(ctx->p_bmrt);
		ctx->p_bmrt = NULL;
	}

	return 0;
}

static int teaisp_drc_load_model(int pipe, const char *path)
{
	bm_status_t status = BM_SUCCESS;
	TEAISP_DRC_CTX_S *ctx = pdrc_ctx[pipe];
	const bm_net_info_t *net_info = NULL;

	pthread_mutex_init(&ctx->param_lock, NULL);

	for (int i = 0; i < DRC_OUT_VB_MAX; i++) {
		pthread_mutex_init(&ctx->vb_lock[i], NULL);
	}

	ctx->p_bmrt = bmrt_create(bm_handle);
	if (ctx->p_bmrt == NULL) {
		printf("[AIDRC] bmrt create fail\n");
		goto load_model_fail;
	}

	if (!bmrt_load_bmodel(ctx->p_bmrt, path)) {
		printf("[AIDRC] load bmodel fail, %s\n", path);
		goto load_model_fail;
	}

	bmrt_get_network_names(ctx->p_bmrt, &ctx->net_names);

	net_info = bmrt_get_network_info(ctx->p_bmrt, ctx->net_names[0]);

	printf("[AIDRC] net name: %s, in: %d, out: %d\n",
		ctx->net_names[0], net_info->input_num, net_info->output_num);

	ctx->input_num = net_info->input_num;
	ctx->output_num = net_info->output_num;

	if (ctx->input_num != DRC_IN_NUM || ctx->output_num != DRC_OUT_NUM) {
		printf("model param num not match!!!! in: %d, %d, out: %d, %d\n",
			ctx->input_num, DRC_IN_NUM, ctx->output_num, DRC_OUT_NUM);
		goto load_model_fail;
	}

	ctx->input_tensors = (bm_tensor_t *) calloc(ctx->input_num, sizeof(bm_tensor_t));
	if (ctx->input_tensors == NULL) {
		printf("[AIDRC] calloc fail, %ld\n", ctx->input_num * sizeof(bm_tensor_t));
		goto load_model_fail;
	}

	ctx->input_vaddr = (void **) calloc(ctx->input_num, sizeof(void *));
	if (ctx->input_vaddr == NULL) {
		printf("[AIDRC] calloc fail, %ld\n", ctx->input_num * sizeof(void *));
		goto load_model_fail;
	}

	ctx->output_tensors = (bm_tensor_t *) calloc(ctx->output_num, sizeof(bm_tensor_t));
	if (ctx->output_tensors == NULL) {
		printf("[AIDRC] calloc fail, %ld\n", ctx->output_num * sizeof(bm_tensor_t));
		goto load_model_fail;
	}

	// input
	for (int i = 0; i < ctx->input_num; i++) {
		ctx->input_tensors[i].dtype = net_info->input_dtypes[i];
		ctx->input_tensors[i].shape = net_info->stages[0].input_shapes[i];
		ctx->input_tensors[i].st_mode = BM_STORE_1N;

		if (i == DRC_IN_IMG) {
			memset(&ctx->input_tensors[i].device_mem, 0, sizeof(bm_device_mem_t));
			ctx->input_tensors[i].device_mem.size = net_info->max_input_bytes[i];
		} else {
			status = bm_malloc_device_byte(bm_handle,
				&ctx->input_tensors[i].device_mem,
				net_info->max_input_bytes[i]);
			if (status != BM_SUCCESS) {
				goto load_model_fail;
			}

			unsigned long long vmem;

			bm_mem_mmap_device_mem_no_cache(bm_handle,
				&ctx->input_tensors[i].device_mem, &vmem);

			memset((void *) vmem, 0, net_info->max_input_bytes[i]);

			ctx->input_vaddr[i] = (void *) vmem;
		}

		printf("[AIDRC] in: %d, dtype: %d, shape: %dx%dx%dx%d, %d, 0x%lx, size: %d\n", i,
			ctx->input_tensors[i].dtype,
			ctx->input_tensors[i].shape.dims[0], ctx->input_tensors[i].shape.dims[1],
			ctx->input_tensors[i].shape.dims[2], ctx->input_tensors[i].shape.dims[3],
			ctx->input_tensors[i].shape.num_dims,
			ctx->input_tensors[i].device_mem.u.device.device_addr,
			(int) net_info->max_input_bytes[i]);
	}

	// output
	for (int i = 0; i < ctx->output_num; i++) {
		ctx->output_tensors[i].dtype = net_info->output_dtypes[i];
		ctx->output_tensors[i].shape = net_info->stages[0].output_shapes[i];
		ctx->output_tensors[i].st_mode = BM_STORE_1N;

		if (i == DRC_OUT_IMG) {
			memset(&ctx->output_tensors[i].device_mem, 0, sizeof(bm_device_mem_t));
			ctx->output_tensors[i].device_mem.size = net_info->max_output_bytes[i];
		} else {
			status = bm_malloc_device_byte(bm_handle,
				&ctx->output_tensors[i].device_mem,
				net_info->max_output_bytes[i]);
			if (status != BM_SUCCESS) {
				goto load_model_fail;
			}

			unsigned long long vmem;

			bm_mem_mmap_device_mem(bm_handle,
				&ctx->output_tensors[i].device_mem, &vmem);

			memset((void *) vmem, 0, net_info->max_output_bytes[i]);

			bm_mem_flush_device_mem(bm_handle,
				&ctx->output_tensors[i].device_mem);
			bm_mem_unmap_device_mem(bm_handle,
				(void *) vmem, (int) net_info->max_output_bytes[i]);
		}

		printf("[AIDRC] out: %d, dtype: %d, shape: %dx%dx%dx%d, %d, 0x%lx, size: %d\n", i,
			ctx->output_tensors[i].dtype,
			ctx->output_tensors[i].shape.dims[0], ctx->output_tensors[i].shape.dims[1],
			ctx->output_tensors[i].shape.dims[2], ctx->output_tensors[i].shape.dims[3],
			ctx->output_tensors[i].shape.num_dims,
			ctx->output_tensors[i].device_mem.u.device.device_addr,
			(int) net_info->max_output_bytes[i]);
	}

	for (int i = 0; i < DRC_OUT_VB_MAX; i++) {
		ctx->vb_blk[i] = CVI_VB_GetBlock(VB_INVALID_POOLID, net_info->max_output_bytes[DRC_OUT_IMG]);
		if (ctx->vb_blk[i] == VB_INVALID_HANDLE) {
			printf("[AIDRC] get vb blk fail, %ld\n", net_info->max_output_bytes[DRC_OUT_IMG]);
			continue;
		}
		ctx->vb_flag[i] = -1;
		ctx->vb_phy_addr[i] = CVI_VB_Handle2PhysAddr(ctx->vb_blk[i]);
	}

	ctx->write_vb_index = ctx->read_vb_index = 0;

	return 0;

load_model_fail:

	teaisp_drc_unload_model(pipe);

	return -1;
}

int get_teaisp_drc_video_frame(int pipe, void *video_frame)
{
	TEAISP_DRC_CTX_S *ctx = pdrc_ctx[pipe];
	uint8_t index = ctx->read_vb_index % DRC_OUT_VB_MAX;

	VIDEO_FRAME_INFO_S *frame = (VIDEO_FRAME_INFO_S *) video_frame;

try_again:
	pthread_mutex_lock(&ctx->vb_lock[index]);

	if (ctx->vb_flag[index] == 0 ||
		ctx->vb_flag[index] == -1) {
		pthread_mutex_unlock(&ctx->vb_lock[index]);
		usleep(10 * 1000);
		goto try_again;
	}

	*frame = ctx->dst[index];

	return index;
}

int put_teaisp_drc_video_frame(int pipe, void *video_frame)
{
	TEAISP_DRC_CTX_S *ctx = pdrc_ctx[pipe];
	uint8_t index = ctx->read_vb_index % DRC_OUT_VB_MAX;

	UNUSED(video_frame);

	ctx->read_vb_index++;
	ctx->vb_flag[index] = 0;

	pthread_mutex_unlock(&ctx->vb_lock[index]);

	return (ctx->read_vb_index % DRC_OUT_VB_MAX);
}

static int get_write_vb_index(TEAISP_DRC_CTX_S *ctx)
{
	uint8_t index = ctx->write_vb_index % DRC_OUT_VB_MAX;

	pthread_mutex_lock(&ctx->vb_lock[index]);

	return index;
}

static int put_write_vb_index(TEAISP_DRC_CTX_S *ctx)
{
	uint8_t index = ctx->write_vb_index % DRC_OUT_VB_MAX;

	ctx->write_vb_index++;
	ctx->vb_flag[index] = 1;

	pthread_mutex_unlock(&ctx->vb_lock[index]);

	return (ctx->write_vb_index % DRC_OUT_VB_MAX);
}

static void *run_teaisp_drc(void *param)
{
#define __VPSS_TIMEOUT (3000)

	CVI_S32 ret = CVI_SUCCESS;
	bm_status_t status = BM_SUCCESS;
	SERVICE_CTX_ENTITY *ent = (SERVICE_CTX_ENTITY *) param;
	TEAISP_DRC_CTX_S *ctx = pdrc_ctx[ent->ViPipe];
    VIDEO_FRAME_INFO_S src = {};
	uint8_t index = 0;
	bool is_first_frame = true;
	float drc_param[TEAISP_DRC_PARAM_LENGHT];

	prctl(PR_SET_NAME, "AIDRC", 0, 0, 0);

	while (ctx->enable_drc_thread) {

		pthread_mutex_lock(&ctx->param_lock);
		memcpy(drc_param, ctx->drc_param, sizeof(float) * TEAISP_DRC_PARAM_LENGHT);
		pthread_mutex_unlock(&ctx->param_lock);

		ret = CVI_VPSS_GetChnFrame(ent->VpssGrp, ent->VpssChn, &src, __VPSS_TIMEOUT);
		if (ret != CVI_SUCCESS) {
			printf("[AIDRC] get vpss frame Grp: %u Chn: %u failed with %#x\n",
				ent->VpssGrp, ent->VpssChn, ret);
			continue;
		}

		index = get_write_vb_index(ctx);

		if (ctx->vb_flag[index] != -1 &&
			ctx->dst[index].stVFrame.u64PhyAddr[0] != ctx->vb_phy_addr[index]) { // disable drc case
			//printf("[AIDRC] %f, %d, %ld, %ld\n", drc_param[0], ctx->vb_flag[index],
			//	ctx->dst[index].stVFrame.u64PhyAddr[0],
			//	ctx->vb_phy_addr[index]);
			ret = CVI_VPSS_ReleaseChnFrame(ent->VpssGrp, ent->VpssChn, &ctx->dst[index]);
			if (ret != CVI_SUCCESS) {
				printf("[AIDRC] release vpss frame Grp: %u Chn: %u failed with %#x\n",
					ent->VpssGrp, ent->VpssChn, ret);
			}
		}

		if (drc_param[0] == 0.0) { // disable drc
			ctx->dst[index] = src;
			put_write_vb_index(ctx);
			is_first_frame = true;
			continue;
		}

		if (is_first_frame) {
			drc_param[0] = 1.0;
			is_first_frame = false;
		} else {
			drc_param[0] = 0.0;
		}

		ctx->input_tensors[DRC_IN_IMG].device_mem.u.device.device_addr = src.stVFrame.u64PhyAddr[0];
		ctx->output_tensors[DRC_OUT_IMG].device_mem.u.device.device_addr = ctx->vb_phy_addr[index];

		bm_tensor_t temp;

		temp = ctx->input_tensors[DRC_IN_EMA];
		ctx->input_tensors[DRC_IN_EMA] = ctx->output_tensors[DRC_OUT_EMA];
		ctx->output_tensors[DRC_OUT_EMA] = temp;

		//static float input_param[9] = {0.7, -0.05628, 0.9099, 0.143, 0.002997, 1.0, 1.0, 0.5, 1};

		//memcpy(ctx->input_vaddr[DRC_IN_COEFF], input_param, sizeof(float) * 9);
		memcpy(ctx->input_vaddr[DRC_IN_COEFF], drc_param, sizeof(float) * TEAISP_DRC_PARAM_LENGHT);

		//struct timeval tv1, tv2;

		//gettimeofday(&tv1, NULL);

		uint32_t core_id = 0;
		// int pipe = (int) (uint64_t) param;
		int pipe = ent->ViPipe;

		if (access("/tmp/teaisp_drc_dump", F_OK) == 0) {
			teaisp_drc_dump_input(ctx, pipe);
			system("rm /tmp/teaisp_drc_dump;touch /tmp/teaisp_drc_dump_output");
		}

		bool ret = bmrt_launch_tensor_multi_cores(ctx->p_bmrt, ctx->net_names[0],
			ctx->input_tensors, ctx->input_num,
			ctx->output_tensors, ctx->output_num, true, false,
			(const int *) &core_id, 1);

		if (ret) {
			status = bm_thread_sync_from_core(bm_handle, core_id);
		}

		if (!ret || BM_SUCCESS != status) {
			printf("[AIDRC] %s, inference failed...\n", ctx->net_names[0]);
		}

		if (access("/tmp/teaisp_drc_dump_output", F_OK) == 0) {
			teaisp_drc_dump_output(ctx, pipe);
			system("rm /tmp/teaisp_drc_dump_output");
		}

		//gettimeofday(&tv2, NULL);

		//printf("[AIDRC] launch time diff, %ld\n",
		//	((tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec)));

		ctx->dst[index] = src;

		ctx->dst[index].stVFrame.u64PhyAddr[0] = ctx->vb_phy_addr[index];
		ctx->dst[index].stVFrame.u64PhyAddr[1] = ctx->dst[index].stVFrame.u64PhyAddr[0] +
			ctx->dst[index].stVFrame.u32Length[0];
		ctx->dst[index].stVFrame.u64PhyAddr[2] = ctx->dst[index].stVFrame.u64PhyAddr[1] +
			ctx->dst[index].stVFrame.u32Length[1];

		ctx->dst[index].stVFrame.pu8VirAddr[0] = NULL;
		ctx->dst[index].stVFrame.pu8VirAddr[1] = NULL;
		ctx->dst[index].stVFrame.pu8VirAddr[2] = NULL;

		put_write_vb_index(ctx);

		ret = CVI_VPSS_ReleaseChnFrame(ent->VpssGrp, ent->VpssChn, &src);
		if (ret != CVI_SUCCESS) {
			printf("[AIDRC] release vpss frame Grp: %u Chn: %u failed with %#x\n",
				ent->VpssGrp, ent->VpssChn, ret);
		}
	}

	return NULL;
}

static int teaisp_drc_param_update_callback(int pipe, void *param) {

	TEAISP_DRC_CTX_S *ctx = pdrc_ctx[pipe];

	pthread_mutex_lock(&ctx->param_lock);
	memcpy(ctx->drc_param, param, sizeof(float) * TEAISP_DRC_PARAM_LENGHT);
	pthread_mutex_unlock(&ctx->param_lock);

	return 0;
}

int init_teaisp_drc(SERVICE_CTX *ctx)
{
	for (int i = 0; i < ctx->dev_num; i++) {
		SERVICE_CTX_ENTITY *pEntity = &ctx->entity[i];

		if (!pEntity->enableTeaispDrc) {
			continue;
		}

		if (bm_handle == NULL) {
			bm_status_t status = bm_dev_request(&bm_handle, 0);
			if (status != BM_SUCCESS) {
				printf("[AIDRC] request dev fail, %d\n", status);
				return -1;
			}
		}

		if (pdrc_ctx[i] == NULL) {
			pdrc_ctx[i] = (TEAISP_DRC_CTX_S *) calloc(1, sizeof(TEAISP_DRC_CTX_S));
			if (pdrc_ctx[i] == NULL) {
				printf("[AIDRC] calloc fail, %ld\n", sizeof(TEAISP_DRC_CTX_S));
				return -1;
			}

			CVI_TEAISP_DRC_RegParamUpdateCallback(pEntity->ViPipe, teaisp_drc_param_update_callback);

			if (teaisp_drc_load_model(i, ctx->teaispdrc_model_path) < 0) {
				return -1;
			}
			pdrc_ctx[i]->enable_drc_thread = 1;
			if (pthread_create(&pdrc_ctx[i]->tid, NULL, run_teaisp_drc, pEntity) != 0) {
				printf("[AIDRC] create drc thread fail...\n");
				return -1;
			}
		}
	}

	return 0;
}

int deinit_teaisp_drc(SERVICE_CTX *ctx)
{
	for (int i = 0; i < ctx->dev_num; i++) {
        SERVICE_CTX_ENTITY *pEntity = &ctx->entity[i];

		if (!pEntity->enableTeaispDrc) {
			continue;
		}

		if (pdrc_ctx[i]) {
			pdrc_ctx[i]->enable_drc_thread = 0;
			pthread_join(pdrc_ctx[i]->tid, NULL);
			teaisp_drc_unload_model(i);
			free(pdrc_ctx[i]);
			pdrc_ctx[i] = NULL;
		}
	}

	if (bm_handle) {
		bm_dev_free(bm_handle);
		bm_handle = NULL;
	}

	return 0;
}


