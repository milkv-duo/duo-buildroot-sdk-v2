#ifndef _BL_UTIL_H_
#define _BL_UTIL_H_

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define BL_UTIL_FAIL        (-1)
#define BL_UTIL_SUCCESS     (0)
#define BL_UTIL_BUFF_LEN    (2048)

#define BL_DEV_PRIV_IOCTL_DEFAULT    (SIOCDEVPRIVATE)

//#define SIOCDEVPRIVATE   0x89F0  /* to 89FF */
//#define SIOCIWFIRST       0x8B00
//#define SIOCIWLAST        SIOCIWLASTPRIV      /* 0x8BFF */
//#define SIOCIWFIRSTPRIV   0x8BE0
//#define SIOCIWLASTPRIV    0x8BFF

#define BL_UTIL_CMD_VERSION        0x6001
#define BL_UTIL_CMD_TEMP           0x6002
#define BL_UTIL_CMD_SCAN           0x6003
#define BL_UTIL_CMD_SCAN_CHAN      0x6004
#define BL_UTIL_CMD_SCAN_IE        0x6005
#define BL_UTIL_CMD_GET_COUNTRY_CODE     0x6006
#define BL_UTIL_CMD_GET_MAC_ADDR         0x6008
#define BL_UTIL_CMD_GET_STATUS           0x6009
#define BL_UTIL_CMD_GET_RSSI             0x600a
#define BL_UTIL_CMD_READ_EFUSE           0x600b



#define BL_NL_BCAST_GROUP_ID   1
#define BL_NL_SOCKET_NUM       31
#define BL_NL_BUF_MAX_LEN      256

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef signed char         s8;
typedef short               s16;
typedef int                 s32;
typedef long long           s64;


#define TYPE_NONE 0x0
#define TYPE_BYTE 0x1
#define TYPE_INT 0x2

enum bl_event_id {
    BL_EVENT_ID_NONE           = 0x1000,
    BL_EVENT_ID_RESET          = 0x1001,   
};

struct util_cmd_node {
    char *name;
    u32   cmd_id;
    int (*hdl) (int argc, char ** argv, u32 cmd_id);
};


struct bl_nl_event {
    u32  event_id;
    u32  payload_len;
    u8   payload[];
};



#endif /* _BL_UTIL_H_ */

