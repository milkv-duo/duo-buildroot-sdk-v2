#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

//#include <net/if.h>     /* for IFNAMSIZ and co... */
#include <linux/types.h>
#include <linux/wireless.h>
#include "bl_util.h"

#include <linux/netlink.h>
#include <signal.h>

#define IFNAMSIZ 16
int sockfd;
char iface_name[IFNAMSIZ];
int bl_util_stop_flag = 0;

static int bl_util_send_cmd(int argc, char ** argv, struct iwreq * wrq, int type, u32 cmd_id)
{
    int i = 0, temp, offset = 0, ret = BL_UTIL_SUCCESS;
    u8 * buf = NULL, * pos = NULL;

    pos = buf = (u8 *)malloc(BL_UTIL_BUFF_LEN);
    if (!buf) {
        printf("Error:%s, can't alloc command buf.\n", __func__);
        return BL_UTIL_FAIL;
    }

    memset(buf, 0, BL_UTIL_BUFF_LEN);
    memset(wrq, 0, sizeof(struct iwreq));
    strncpy(wrq->ifr_ifrn.ifrn_name, iface_name, IFNAMSIZ);
    wrq->ifr_ifrn.ifrn_name[IFNAMSIZ - 1] = '\0';

    wrq->u.data.pointer = (void *) buf;
    wrq->u.data.length = 0;

    // copy cmd id
    * (u32 *)pos = cmd_id;
    wrq->u.data.length += sizeof(cmd_id);
    pos += wrq->u.data.length;	
	
    switch (type)
    {
        case TYPE_BYTE:
            /* Number of args to fetch */
            wrq->u.data.length += argc -3;
            if (argc <=3)
               break;

            /* Fetch args */
            for(i=3; i < argc ; i++) {
                sscanf(argv[i], "%i", &temp);
                pos[i-3] = (char) temp;
			printf("%x  ", temp);
            }
	    printf("\n");
            break;

        case TYPE_INT:
            /* Number of args to fetch */
            wrq->u.data.length += argc -3;
            if (argc <=3)
                break;
            /* Fetch args */
            for(i=3; i < argc ; i++) {
                sscanf(argv[i], "%i", &temp);	
                ((__s32 *) pos)[i-3] = (__s32) temp;
			
			printf("%x  ", temp);
            }
			
			printf("\n");
            break;
			
        default:
            break;
	}

    printf("dump ");
    for (i = 0; i < wrq->u.data.length; i++)
        printf(":0x%x", *(buf + i));
    printf("dump end\n");

    ret = ioctl(sockfd, BL_DEV_PRIV_IOCTL_DEFAULT, wrq);

    return ret;
}


static void bl_util_free_cmd(struct iwreq * wrq)
{
    if (wrq->u.data.pointer)
        free(wrq->u.data.pointer);
}

static int bl_util_version(int argc, char ** argv, u32 cmd_id)
{
    int ret = BL_UTIL_SUCCESS, i = 0;
    struct iwreq wrq;
    //argv[0]=./blutl
    //argv[1]=wlan0
    //argv[2]=version

    ret = bl_util_send_cmd(argc, argv, &wrq, TYPE_BYTE, cmd_id);
    if (ret) {
        printf("Error: blutl version fail %d\n", ret);
    } else {
        printf("%s:%s\n", __func__, (char *)wrq.u.data.pointer + sizeof(cmd_id));
    }

    bl_util_free_cmd(&wrq);

    return ret;
}

static int bl_util_temp(int argc, char ** argv, u32 cmd_id)
{
    int ret = 0;
    struct iwreq wrq;

    //argv[0]=./blutl
    //argv[1]=wlan0
    //argv[2]=temp


    ret = bl_util_send_cmd(argc, argv, &wrq, TYPE_BYTE, cmd_id);
    if (ret) {
        printf("Error: blutl read temperture fail %d\n", ret);
    } else {
        printf("%s:temperature=%d\r\n", __func__, *(int32_t *)(wrq.u.data.pointer + sizeof(cmd_id)));
    }

    bl_util_free_cmd(&wrq);

    return ret;
}

struct util_cmd_node bl_util_cmd_table[] = {
    {"version",     BL_UTIL_CMD_VERSION,            bl_util_version},
    {"temp",        BL_UTIL_CMD_TEMP,               bl_util_temp},
};


/* struct payload  is full ieee80211_mgmt frame following  kernel struct <struct ieee80211_mgmt>
 * struct payload_len is ieee80211_mgmt frame len
 */
void bl_util_parsing_mgmt_frame(u8 * payload, u16 payload_len)
{
    int i = 0;
#if 0
    for (i = 0; i < payload_len; i++)
        printf(":0x%x", payload[i]);
#endif
    return;
}

void bl_util_sig_hdl(int sig)
{
    printf("%s 0x%x\n", __func__, sig);
    bl_util_stop_flag = 1;
}

int bl_util_read_event(int sk_fd, struct nlmsghdr *nlmsg_hdr, struct msghdr *msg_hdr)
{
    int byte_cnt = 0;
    //struct bl_nl_event * bl_event = NULL;

    byte_cnt = recvmsg(sk_fd, msg_hdr, 0);

    if (byte_cnt < 0 || byte_cnt > NLMSG_SPACE(BL_NL_BUF_MAX_LEN))
        return -1;

    //bl_event = (struct bl_nl_event *) NLMSG_DATA(nlmsg_hdr);

    //printf("%s byte_cnt=%d, event_id=0x%x, nlmsg_len=%d seq=%d payload_len=%d\n", __func__, byte_cnt, 
    //    bl_event->event_id, nlmsg_hdr->nlmsg_len, nlmsg_hdr->nlmsg_seq, bl_event->payload_len);

    return byte_cnt;
}

/* Return total nlmsg len, include <NLMSG_HDRLEN + NLMSG_DATA(nlmsg_hdr)>
 * NLMSG_DATA(nlmsg_hdr) is struct bl_nl_event 
 * struct bl_nl_event.event_id is enum bl_event_id
 * struct bl_nl_event.payload  is full ieee80211_mgmt frame following  kernel struct <struct ieee80211_mgmt>
 * struct bl_nl_event.payload_len is ieee80211_mgmt frame len
*/
int bl_util_wait_event(int nl_socket, int timeout_s, struct nlmsghdr *nlmsg_hdr, struct msghdr *msg_hdr)
{
    struct timeval * timeout = NULL, time;
    fd_set read_fds;
    int i = 0, ret = 0;

    FD_ZERO(&read_fds);
    FD_SET(nl_socket, &read_fds);

    /* Initialize timeout value */
    if (timeout_s != 0) {
        time.tv_sec = timeout_s;
        time.tv_usec = 0;
        timeout = &time;
    }

    ret = select(nl_socket + 1, &read_fds, NULL, NULL, timeout);
    if (ret == -1) {
        // wait abnormal
        bl_util_stop_flag = 1;
        return ret;
    } else if (ret == 0) {
        //wait timeout
        return ret;
    }

    if (FD_ISSET(nl_socket, &read_fds)) {
        ret = bl_util_read_event(nl_socket, nlmsg_hdr, msg_hdr);
    }

    return ret;
}

void bl_util_event_loop(void)
{
    int i = 0, ret = 0;
    int nl_socket = -1;
    struct nlmsghdr *nlmsg_hdr = NULL;
    struct msghdr    msg_hdr;
    struct iovec     io_vec;
    struct sockaddr_nl src_addr, dest_addr;
    struct bl_nl_event * bl_event;

    bl_util_stop_flag = 0;
    memset(&src_addr, 0, sizeof(src_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));

    nl_socket = socket(PF_NETLINK, SOCK_RAW, BL_NL_SOCKET_NUM);
    if (nl_socket < 0) {
        printf("%s socket creat fail\n", __func__);
        goto done;
    }

    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    src_addr.nl_groups = BL_NL_BCAST_GROUP_ID;

    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = BL_NL_BCAST_GROUP_ID;

    if (bind(nl_socket, (struct sockaddr *) &src_addr, sizeof(src_addr)) < 0) {
        printf("%s socket%d bind fail\n", __func__, nl_socket);
        goto done;
    }

    nlmsg_hdr = (struct nlmsghdr *) malloc(NLMSG_SPACE(BL_NL_BUF_MAX_LEN));
    if (!nlmsg_hdr) {
        printf("%s nlmsg buffer alloc fail\n", __func__);
        goto done;
    }
    memset(nlmsg_hdr, 0, NLMSG_SPACE(BL_NL_BUF_MAX_LEN));

    io_vec.iov_base = (void *) nlmsg_hdr;
    io_vec.iov_len = NLMSG_SPACE(BL_NL_BUF_MAX_LEN);

    memset(&msg_hdr, 0, sizeof(struct msghdr));
    msg_hdr.msg_name = (void *) &dest_addr;
    msg_hdr.msg_namelen = sizeof(dest_addr);
    msg_hdr.msg_iov = &io_vec;
    msg_hdr.msg_iovlen = 1;

    signal(SIGTERM, bl_util_sig_hdl);
    signal(SIGINT, bl_util_sig_hdl);
    signal(SIGALRM, bl_util_sig_hdl);

    while (1) {
        if (bl_util_stop_flag) {
            printf("Stopping!\n");
            break;
        }
        ret = bl_util_wait_event(nl_socket, 0, nlmsg_hdr, &msg_hdr);

        if (ret == -1) {
            printf("wait abnormal, try again\n");
            continue;
        }
        if (ret == 0) {
            printf("wait timeout\n");
            continue;
        }

        bl_event = (struct bl_nl_event *) NLMSG_DATA(nlmsg_hdr);

        printf("%s recv_len=%d, event_id=0x%x, nlmsg_len=%d seq=%d payload_len=%d\n", __func__, ret,
            bl_event->event_id, nlmsg_hdr->nlmsg_len, nlmsg_hdr->nlmsg_seq, bl_event->payload_len);

        switch (bl_event->event_id) {
			case BL_EVENT_ID_RESET:
				{
					printf("Driver in abnormal status, need to reset \n");
				}
				break;

                default:
                    printf("unknown event:0x%x len=%d\n", bl_event->event_id, bl_event->payload_len);
                    break;
        }

        fflush(stdout);
    }

done:
    if (nl_socket > 0)
        close(nl_socket);
    if (nlmsg_hdr)
        free(nlmsg_hdr);
    return;
}

int main(int argc, char * argv [ ])
{
    int i = 0, ret = -1;
    struct util_cmd_node * util_cmd = NULL;

    if(argc < 3) {
        fprintf(stderr, "input params num is %d, should >= 3\n", argc);
         return -1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("creat global socket fail\n");
        return -1;
    }

    strncpy(iface_name, argv[1], IFNAMSIZ);

    printf("argc=%d, interface %s\n", argc, iface_name);

    if (!strcasecmp("event", argv[2])) {
        bl_util_event_loop();
        return 0;
    }

    for (i = 0; i < (int)ARRAY_SIZE(bl_util_cmd_table); i++) {
        util_cmd = &bl_util_cmd_table[i];
        if (!strcasecmp(util_cmd->name, argv[2])) {
            ret = util_cmd->hdl(argc, argv, util_cmd->cmd_id);
            break;
        }
    }

    if(i == ARRAY_SIZE(bl_util_cmd_table)) {
        printf("command(%s) handler not found\n", argv[2]);
    }

    return ret;
}

