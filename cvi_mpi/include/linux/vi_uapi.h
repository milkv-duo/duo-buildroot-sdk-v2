#ifndef __U_VI_UAPI_H__
#define __U_VI_UAPI_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/time_types.h>
#endif
#include <linux/cvi_comm_vi.h>
#include <linux/cvi_comm_sys.h>

#define VI_IOC_MAGIC		'V'
#define VI_IOC_BASE		0x20

#define VI_IOC_G_CTRL		_IOWR(VI_IOC_MAGIC, VI_IOC_BASE, struct vi_ext_control)
#define VI_IOC_S_CTRL		_IOWR(VI_IOC_MAGIC, VI_IOC_BASE + 1, struct vi_ext_control)

/*
 * VI IOCTL.
 */
enum VI_IOCTL {
	VI_IOCTL_ONLINE,                /* Enable online mode */
	VI_IOCTL_HDR,                   /* Enable HDR (High Dynamic Range) */
	VI_IOCTL_3DNR,                  /* Enable 3D Noise Reduction */
	VI_IOCTL_TILE,                  /* Enable tiling */
	VI_IOCTL_COMPRESS_EN,           /* Enable compression */
	VI_IOCTL_STS_MEM,               /* Status memory operation */
	VI_IOCTL_STS_GET,               /* Get status */
	VI_IOCTL_STS_PUT,               /* Put status */
	VI_IOCTL_POST_STS_GET,          /* Get post status */
	VI_IOCTL_POST_STS_PUT,          /* Put post status */
	VI_IOCTL_USR_PIC_CFG,           /* User picture configuration */
	VI_IOCTL_USR_PIC_ONOFF,         /* Enable/disable user picture */
	VI_IOCTL_USR_PIC_PUT,           /* Put user picture */
	VI_IOCTL_AE_CFG,                /* Auto Exposure configuration */
	VI_IOCTL_AWB_CFG,               /* Auto White Balance configuration */
	VI_IOCTL_AF_CFG,                /* Auto Focus configuration */
	VI_IOCTL_USR_PIC_TIMING,        /* User picture timing configuration */
	VI_IOCTL_GET_LSC_PHY_BUF,       /* Get LSC physical buffer */
	VI_IOCTL_CSIBDG_CFG,            /* CSI bridge configuration */
	VI_IOCTL_GET_TUN_ADDR,          /* Get tuning address */
	VI_IOCTL_SET_SNR_INFO,          /* Set sensor information */
	VI_IOCTL_SET_SNR_CFG_NODE,      /* Set sensor configuration node */
	VI_IOCTL_GET_PIPE_DUMP,         /* Get pipe dump */
	VI_IOCTL_PUT_PIPE_DUMP,         /* Put pipe dump */
	VI_IOCTL_SET_RGBMAP_IDX,        /* Set RGB map index */
	VI_IOCTL_HDR_DETAIL_EN,         /* Enable HDR details */
	VI_IOCTL_YUV_BYPASS_PATH,       /* Enable YUV bypass path */
	VI_IOCTL_BE_ONLINE,             /* Enable backend online mode */
	VI_IOCTL_SUBLVDS_PATH,          /* Set sub LVDS path */
	VI_IOCTL_GET_IP_INFO,           /* Get IP information */
	VI_IOCTL_TRIG_PRERAW,           /* Trigger pre-raw data */
	VI_IOCTL_SET_PROC_CONTENT,       /* Set processing content */
	VI_IOCTL_SC_ONLINE,             /* Enable smart capture online mode */
	VI_IOCTL_MMAP_GRID_SIZE,        /* Get memory map grid size */
	VI_IOCTL_RGBIR,                 /* RGBIR configuration */
	VI_IOCTL_AWB_STS_GET,           /* Get AWB status */
	VI_IOCTL_AWB_STS_PUT,           /* Put AWB status */
	VI_IOCTL_GET_FSWDR_PHY_BUF,     /* Get FSWDR physical buffer */
	VI_IOCTL_GET_SCENE_INFO,        /* Get scene information */
	VI_IOCTL_CLK_CTRL,              /* Clock control */
	VI_IOCTL_GET_BUF_SIZE,          /* Get buffer size */
	VI_IOCTL_SET_DMA_BUF_INFO,      /* Set DMA buffer information */
	VI_IOCTL_ENQ_BUF,               /* Enqueue buffer */
	VI_IOCTL_DQEVENT,               /* Dequeue event */
	VI_IOCTL_START_STREAMING,       /* Start streaming */
	VI_IOCTL_STOP_STREAMING,        /* Stop streaming */
	VI_IOCTL_SET_SLICE_BUF_EN,      /* Enable slice buffer */
	VI_IOCTL_GET_CLUT_TBL_IDX,      /* Get CLUT table index */
	VI_IOCTL_SDK_CTRL,              /* SDK control commands */
	VI_IOCTL_GET_RGBMAP_LE_PHY_BUF, /* Get RGB map left physical buffer */
	VI_IOCTL_GET_RGBMAP_SE_PHY_BUF, /* Get RGB map right physical buffer */
	VI_SINGEL_FRAME_ENABLE,		/* Singel frame mode*/
	VI_IOCTL_AI_ISP_CFG,            /* Config AI ISP */
	VI_IOCTL_AI_ISP_SET_BUF,        /* Set AI ISP buffer */
	VI_IOCTL_GET_AI_ISP_RAW,        /* Get AI isp raw phy buffer*/
	VI_IOCTL_PUT_AI_ISP_RAW,        /* Put AI isp raw phy buffer*/
	VI_IOCTL_MAX,                   /* Maximum value for enumeration */
};

/*
 * SDK IOCTL.
 */
enum VI_SDK_CTRL {
	VI_SDK_SET_DEV_ATTR,             /* Set device attributes */
	VI_SDK_GET_DEV_ATTR,             /* Get device attributes */
	VI_SDK_ENABLE_DEV,              /* Enable device */
	VI_SDK_DISABLE_DEV,             /* Disable device */
	VI_SDK_CREATE_PIPE,             /* Create a processing pipe */
	VI_SDK_DESTROY_PIPE,            /* Destroy a processing pipe */
	VI_SDK_SET_PIPE_ATTR,            /* Set pipe attributes */
	VI_SDK_GET_PIPE_ATTR,            /* Get pipe attributes */
	VI_SDK_START_PIPE,              /* Start processing pipe */
	VI_SDK_STOP_PIPE,               /* Stop processing pipe */
	VI_SDK_SET_CHN_ATTR,             /* Set channel attributes */
	VI_SDK_GET_CHN_ATTR,             /* Get channel attributes */
	VI_SDK_ENABLE_CHN,              /* Enable channel */
	VI_SDK_DISABLE_CHN,             /* Disable channel */
	VI_SDK_SET_MOTION_LV,           /* Set motion level */
	VI_SDK_ENABLE_DIS,              /* Enable digital image stabilization */
	VI_SDK_DISABLE_DIS,             /* Disable digital image stabilization */
	VI_SDK_SET_DIS_INFO,            /* Set digital image stabilization information */
	VI_SDK_SET_PIPE_FRM_SRC,        /* Set pipe frame source */
	VI_SDK_SEND_PIPE_RAW,           /* Send raw data to pipe */
	VI_SDK_SET_DEV_TIMING_ATTR,     /* Set device timing attributes */
	VI_SDK_GET_DEV_TIMING_ATTR,     /* Get device timing attributes */
	VI_SDK_GET_CHN_FRAME,           /* Get channel frame */
	VI_SDK_RELEASE_CHN_FRAME,       /* Release channel frame */
	VI_SDK_SET_CHN_CROP,            /* Set channel crop parameters */
	VI_SDK_GET_CHN_CROP,            /* Get channel crop parameters */
	VI_SDK_GET_PIPE_FRAME,           /* Get pipe frame */
	VI_SDK_RELEASE_PIPE_FRAME,       /* Release pipe frame */
	VI_SDK_START_SMOOTH_RAWDUMP,    /* Start smooth raw dump */
	VI_SDK_STOP_SMOOTH_RAWDUMP,     /* Stop smooth raw dump */
	VI_SDK_GET_SMOOTH_RAWDUMP,       /* Get smooth raw dump */
	VI_SDK_PUT_SMOOTH_RAWDUMP,       /* Put smooth raw dump */
	VI_SDK_SET_CHN_ROTATION,        /* Set channel rotation */
	VI_SDK_SET_CHN_LDC,             /* Set lens distortion correction for channel */
	VI_SDK_ATTACH_VB_POOL,          /* Attach video buffer pool */
	VI_SDK_DETACH_VB_POOL,          /* Detach video buffer pool */
	VI_SDK_GET_PIPE_DUMP_ATTR,		/* Get pipe dump attr */
	VI_SDK_SET_PIPE_DUMP_ATTR,		/* Set pipe dump attr */
	VI_SDK_GET_DEV_STATUS			/* Get dev status */
};

/*
 * Events.
 */
enum VI_EVENT {
	VI_EVENT_BASE,                  /* Base event */
	VI_EVENT_PRE0_SOF,             /* Pre-frame 0 start of frame event */
	VI_EVENT_PRE1_SOF,             /* Pre-frame 1 start of frame event */
	VI_EVENT_PRE2_SOF,             /* Pre-frame 2 start of frame event */
	VI_EVENT_VIRT_PRE3_SOF,        /* Virtual pre-frame 3 start of frame event */
	VI_EVENT_PRE0_EOF,             /* Pre-frame 0 end of frame event */
	VI_EVENT_PRE1_EOF,             /* Pre-frame 1 end of frame event */
	VI_EVENT_PRE2_EOF,             /* Pre-frame 2 end of frame event */
	VI_EVENT_VIRT_PRE3_EOF,        /* Virtual pre-frame 3 end of frame event */
	VI_EVENT_POST_EOF,             /* Post-frame end of frame event */
	VI_EVENT_POST1_EOF,            /* Post-frame 1 end of frame event */
	VI_EVENT_POST2_EOF,            /* Post-frame 2 end of frame event */
	VI_EVENT_VIRT_POST3_EOF,       /* Virtual post-frame 3 end of frame event */
	VI_EVENT_ISP_PROC_READ,        /* ISP processing read event */
	VI_EVENT_AWB0_DONE,            /* AWB (Auto White Balance) 0 completion event */
	VI_EVENT_AWB1_DONE,            /* AWB (Auto White Balance) 1 completion event */
	VI_EVENT_MAX,                  /* Maximum value for enumeration */
};

/*
 * Structure of VI SDK, etc.
 */
struct _vi_sdk_cfg {
	__s32 dev;          /* Device identifier */
	__s32 pipe;         /* Pipe identifier */
	__s32 chn;          /* Channel identifier */
	void *ptr;          /* Pointer to additional configuration data */
	__s32 val;          /* Additional value for configuration */
};

/*
 * Structure of EVENT, etc.
 */
struct vi_ext_control {
	__u32 id;                     /* Control ID */
	__u32 sdk_id;                 /* SDK ID for extended control */
	struct _vi_sdk_cfg sdk_cfg;   /* SDK configuration structure */
	union {
		__s32 value;              /* 32-bit value */
		__s64 value64;            /* 64-bit value */
		void *ptr;                /* Pointer to additional data */
	};
} __attribute__ ((packed));       /* Ensure no padding in structure */

/*
 * Structure for storing image plane.
 */
struct vi_plane {
	__u64 addr;                   /* Address of the memory plane */
};

/*
 * @index:
 * @length: length of planes.
 * @planes: to describe buf.
 * @reserved
 */
struct vi_buffer {
	__u32 index;                  /* Buffer index */
	__u32 length;                 /* Length of the buffer */
	struct vi_plane planes[3];    /* Array of planes (up to 3) */
	__u32 reserved;               /* Reserved for future use */
};

/*
 * Structure that transmits VI behavior.
 */
struct vi_event {
	__u32			dev_id;			/* Device identifier */
	__u32			type;			/* Event type */
	__u32			frame_sequence;	/* Frame sequence number */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct __kernel_timespec   timestamp;
#else
	struct timeval		timestamp;	/* Timestamp of the event */
#endif
};

#define MO_TBL_SIZE 2048
/*
 * Configure the structure of motion information.
 */
struct mlv_info_s {
	__u8	raw_num;	/*raw num*/
	__u8	motion_th;	/*motion threshold*/
};

/*
 * Structure for configuring start and end coordinates of image cropping.
 */
struct crop_size_s {
	__u16 start_x;                /* X coordinate of the start point */
	__u16 start_y;                /* Y coordinate of the start point */
	__u16 end_x;                  /* X coordinate of the end point */
	__u16 end_y;                  /* Y coordinate of the end point */
};

/*
 * Configure the start and end coordinates of image clipping
 * and the structure of the corresponding sensor.
 */
struct dis_info_s {
	__u8   sensor_num;			/* Number of sensors */
	__u32  frm_num;				/* Frame number */
	struct crop_size_s dis_i;	/* Crop size information for DIS */
};

/*
 * Configure the structure corresponding to whether the channel rotates.
 */
struct vi_chn_rot_cfg {
	VI_CHN ViChn;                 /* Channel identifier */
	ROTATION_E enRotation;        /* Rotation setting */
};

/*
 * Configure the structure of LDC.
 */
struct vi_chn_ldc_cfg {
	VI_CHN ViChn;                 /* Channel identifier */
	ROTATION_E enRotation;        /* Rotation setting */
	VI_LDC_ATTR_S stLDCAttr;      /* LDC attributes */
	CVI_U64 meshHandle;           /* Mesh handle for LDC */
};

/*
 * Configure VI to attach the structure of VBpool.
 */
struct vi_vb_pool_cfg {
	VI_PIPE ViPipe;               /* Pipe identifier */
	VI_CHN ViChn;                 /* Channel identifier */
	__u32 VbPool;                 /* Video buffer pool identifier */
};

/*
 * Configure ai isp.
 */

typedef struct ai_isp_bnr_cfg {
	__u64 swap_buf_index; // __32 *
	__u32 swap_buf_count;
	CVI_BOOL ai_rgbmap;
} ai_isp_bnr_cfg_t;

/*
 * Configure ai isp type.
 */

enum ai_isp_type {
	AI_ISP_TYPE_BNR,
	AI_ISP_TYPE_BUTT,
};


/*
 * Configure ai isp type.
 */

enum ai_isp_cfg_type {
	AI_ISP_PIPE_LOAD,
	AI_ISP_PIPE_UNLOAD,
	AI_ISP_CFG_INIT,
	AI_ISP_CFG_DEINIT,
	AI_ISP_CFG_ENABLE,
	AI_ISP_CFG_DISABLE,
};

/*
 * Configure ai isp.
 */

typedef struct ai_isp_cfg {
	__u32 vi_pipe;
	__u32 ai_isp_cfg_type;
	__u32 ai_isp_type;
	__u64 param_addr;
	__u32 param_size;
} ai_isp_cfg_t;

#ifdef __cplusplus
	}
#endif

#endif /* __U_VI_UAPI_H__ */
