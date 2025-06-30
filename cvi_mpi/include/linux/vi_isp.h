#ifndef __U_VI_ISP_H__
#define __U_VI_ISP_H__

#include <linux/vi_snsr.h>
#include <linux/cvi_defines.h>

/*Information on ISP scenarios*/
enum ISP_SCENE_INFO {
    PRE_OFF_POST_OFF_SC,      // Pre-processing off, post-processing off.
    PRE_OFF_POST_ON_SC,       // Pre-processing off, post-processing on.
    FE_ON_BE_ON_POST_OFF_SC,  // Front-end on, back-end on, post-processing off.
    FE_ON_BE_ON_POST_ON_SC,   // Front-end on, back-end on, post-processing on.
    FE_ON_BE_OFF_POST_OFF_SC, // Front-end on, back-end off, post-processing off.
    FE_ON_BE_OFF_POST_ON_SC,  // Front-end on, back-end off, post-processing on.
    FE_OFF_BE_ON_POST_OFF_SC, // Front-end off, back-end on, post-processing off.
    FE_OFF_BE_ON_POST_ON_SC,  // Front-end off, back-end on, post-processing on.
};

/*Different sources of ISP*/
enum cvi_isp_source {
    CVI_ISP_SOURCE_DEV = 0,  // Source from device.
    CVI_ISP_SOURCE_FE,        // Source from front-end.
    CVI_ISP_SOURCE_BE,        // Source from back-end.
    CVI_ISP_SOURCE_MAX,       // Maximum source type (used for bounds checking).
};

/*IP information group type*/
enum IP_INFO_GRP {
    IP_INFO_ID_PRE_RAW_FE0 = 0, // ID for pre-raw front-end 0.
    IP_INFO_ID_CSIBDG0,          // ID for CSI bridge 0.
    IP_INFO_ID_DMA_CTL6,         // ID for DMA control 6.
    IP_INFO_ID_DMA_CTL7,         // ID for DMA control 7.
#if !defined(__CV180X__)
    IP_INFO_ID_DMA_CTL8,         // ID for DMA control 8.
    IP_INFO_ID_DMA_CTL9,         // ID for DMA control 9.
#endif
    IP_INFO_ID_BLC0,             // ID for black level correction 0.
    IP_INFO_ID_BLC1,             // ID for black level correction 1.
    IP_INFO_ID_RGBMAP0,          // ID for RGB mapping 0.
    IP_INFO_ID_WBG2,             // ID for white balance gain 2.
    IP_INFO_ID_DMA_CTL10,        // ID for DMA control 10.
#if !defined(__CV180X__)
    IP_INFO_ID_RGBMAP1,          // ID for RGB mapping 1.
    IP_INFO_ID_WBG3,             // ID for white balance gain 3.
    IP_INFO_ID_DMA_CTL11,        // ID for DMA control 11.

    IP_INFO_ID_PRE_RAW_FE1,      // ID for pre-raw front-end 1.
    IP_INFO_ID_CSIBDG1,          // ID for CSI bridge 1.
    IP_INFO_ID_DMA_CTL12,        // ID for DMA control 12.
    IP_INFO_ID_DMA_CTL13,        // ID for DMA control 13.
    IP_INFO_ID_DMA_CTL14,        // ID for DMA control 14.
    IP_INFO_ID_DMA_CTL15,        // ID for DMA control 15.
    IP_INFO_ID_BLC2,             // ID for black level correction 2.
    IP_INFO_ID_BLC3,             // ID for black level correction 3.
    IP_INFO_ID_RGBMAP2,          // ID for RGB mapping 2.
    IP_INFO_ID_WBG4,             // ID for white balance gain 4.
    IP_INFO_ID_DMA_CTL16,        // ID for DMA control 16.
    IP_INFO_ID_RGBMAP3,          // ID for RGB mapping 3.
    IP_INFO_ID_WBG5,             // ID for white balance gain 5.
    IP_INFO_ID_DMA_CTL17,        // ID for DMA control 17.

    IP_INFO_ID_PRE_RAW_FE2,      // ID for pre-raw front-end 2.
    IP_INFO_ID_CSIBDG2,          // ID for CSI bridge 2.
    IP_INFO_ID_DMA_CTL18,        // ID for DMA control 18.
    IP_INFO_ID_DMA_CTL19,        // ID for DMA control 19.
    IP_INFO_ID_BLC4,             // ID for black level correction 4.
    IP_INFO_ID_RGBMAP4,          // ID for RGB mapping 4.
    IP_INFO_ID_WBG6,             // ID for white balance gain 6.
    IP_INFO_ID_DMA_CTL20,        // ID for DMA control 20.
#endif
    IP_INFO_ID_PRE_RAW_BE,       // ID for pre-raw back-end.
    IP_INFO_ID_CROP0,            // ID for crop area 0.
    IP_INFO_ID_CROP1,            // ID for crop area 1.
    IP_INFO_ID_BLC5,             // ID for black level correction 5.
#if !defined(__CV180X__)
    IP_INFO_ID_BLC6,             // ID for black level correction 6.
#endif
    IP_INFO_ID_AF,               // ID for autofocus.
    IP_INFO_ID_DMA_CTL21,        // ID for DMA control 21.
    IP_INFO_ID_DPC0,             // ID for defective pixel correction 0.
#if !defined(__CV180X__)
	IP_INFO_ID_DPC1,
#endif
    IP_INFO_ID_DMA_CTL22,        // ID for DMA control 22.
#if !defined(__CV180X__)
    IP_INFO_ID_DMA_CTL23,        // ID for DMA control 23.
#endif
    IP_INFO_ID_PRE_WDMA,         // ID for pre-write DMA.
    // IP_INFO_ID_PCHK0,          // ID for parameter check 0.
    // IP_INFO_ID_PCHK1,          // ID for parameter check 1.

    IP_INFO_ID_RAWTOP,           // ID for raw top processing.
    IP_INFO_ID_CFA,              // ID for color filter array.
    IP_INFO_ID_LSC,              // ID for lens shading correction.
    IP_INFO_ID_DMA_CTL24,        // ID for DMA control 24.
    IP_INFO_ID_GMS,              // ID for gain mapping structure.
    IP_INFO_ID_DMA_CTL25,        // ID for DMA control 25.
    IP_INFO_ID_AEHIST0,          // ID for auto exposure histogram 0.
    IP_INFO_ID_DMA_CTL26,        // ID for DMA control 26.
#if !defined(__CV180X__)
    IP_INFO_ID_AEHIST1,          // ID for auto exposure histogram 1.
    IP_INFO_ID_DMA_CTL27,        // ID for DMA control 27.
#endif
    IP_INFO_ID_DMA_CTL28,        // ID for DMA control 28.
#if !defined(__CV180X__)
    IP_INFO_ID_DMA_CTL29,        // ID for DMA control 29.
#endif
    IP_INFO_ID_RAW_RDMA,         // ID for raw read DMA.
    IP_INFO_ID_BNR,              // ID for noise reduction.
    IP_INFO_ID_CROP2,            // ID for crop area 2.
    IP_INFO_ID_CROP3,            // ID for crop area 3.
    IP_INFO_ID_LMAP0,            // ID for lens mapping 0.
    IP_INFO_ID_DMA_CTL30,        // ID for DMA control 30.
#if !defined(__CV180X__)
    IP_INFO_ID_LMAP1,            // ID for lens mapping 1.
    IP_INFO_ID_DMA_CTL31,        // ID for DMA control 31.
#endif
    IP_INFO_ID_WBG0,             // ID for white balance gain 0.
#if !defined(__CV180X__)
    IP_INFO_ID_WBG1,             // ID for white balance gain 1.
#endif
    // IP_INFO_ID_PCHK2,          // ID for parameter check 2.
    // IP_INFO_ID_PCHK3,          // ID for parameter check 3.
    IP_INFO_ID_LCAC,             // ID for local contrast adjustment.
    IP_INFO_ID_RGBCAC,           // ID for RGB color adjustment.

    IP_INFO_ID_RGBTOP,           // ID for RGB top processing.
    IP_INFO_ID_CCM0,             // ID for color correction matrix 0.
#if !defined(__CV180X__)
    IP_INFO_ID_CCM1,             // ID for color correction matrix 1.
#endif
    IP_INFO_ID_RGBGAMMA,         // ID for RGB gamma correction.
    IP_INFO_ID_YGAMMA,           // ID for Y gamma correction.
    IP_INFO_ID_MMAP,             // ID for memory mapping.
    IP_INFO_ID_DMA_CTL32,        // ID for DMA control 32.
#if !defined(__CV180X__)
    IP_INFO_ID_DMA_CTL33,        // ID for DMA control 33.
#endif
    IP_INFO_ID_DMA_CTL34,        // ID for DMA control 34.
#if !defined(__CV180X__)
    IP_INFO_ID_DMA_CTL35,        // ID for DMA control 35.
#endif
    IP_INFO_ID_DMA_CTL36,        // ID for DMA control 36.
#if !defined(__CV180X__)
    IP_INFO_ID_DMA_CTL37,        // ID for DMA control 37.
#endif
    IP_INFO_ID_CLUT,             // ID for color lookup table.
    IP_INFO_ID_DHZ,              // ID for de-haze processing.
    IP_INFO_ID_CSC,              // ID for color space conversion.
    IP_INFO_ID_RGBDITHER,        // ID for RGB dithering.
    // IP_INFO_ID_PCHK4,          // ID for parameter check 4.
    // IP_INFO_ID_PCHK5,          // ID for parameter check 5.
    IP_INFO_ID_HIST_V,           // ID for vertical histogram.
    IP_INFO_ID_DMA_CTL38,        // ID for DMA control 38.
    IP_INFO_ID_HDRFUSION,        // ID for HDR fusion processing.
    IP_INFO_ID_HDRLTM,           // ID for HDR local tone mapping.
    IP_INFO_ID_DMA_CTL39,        // ID for DMA control 39.
#if !defined(__CV180X__)
    IP_INFO_ID_DMA_CTL40,        // ID for DMA control 40.
#endif

    IP_INFO_ID_YUVTOP,           // ID for YUV top processing.
    IP_INFO_ID_TNR,              // ID for temporal noise reduction.
    IP_INFO_ID_DMA_CTL41,        // ID for DMA control 41.
    IP_INFO_ID_DMA_CTL42,        // ID for DMA control 42.
    IP_INFO_ID_FBCE,             // ID for frame buffer compression engine.
    IP_INFO_ID_DMA_CTL43,        // ID for DMA control 43.
    IP_INFO_ID_DMA_CTL44,        // ID for DMA control 44.
    IP_INFO_ID_FBCD,             // ID for frame buffer color depth.
    IP_INFO_ID_YUVDITHER,        // ID for YUV dithering.
    IP_INFO_ID_CA,               // ID for contrast adjustment.
    IP_INFO_ID_CA_LITE,          // ID for lightweight contrast adjustment.
    IP_INFO_ID_YNR,              // ID for Y noise reduction.
    IP_INFO_ID_CNR,              // ID for chroma noise reduction.
    IP_INFO_ID_EE,               // ID for edge enhancement.
    IP_INFO_ID_YCURVE,           // ID for Y curve adjustment.
    IP_INFO_ID_DCI,              // ID for dynamic contrast improvement.
    IP_INFO_ID_DMA_CTL45,        // ID for DMA control 45.
    // IP_INFO_ID_DCI_GAMMA,     // ID for dynamic contrast improvement gamma.
    IP_INFO_ID_CROP4,            // ID for crop area 4.
    IP_INFO_ID_DMA_CTL46,        // ID for DMA control 46.
    IP_INFO_ID_CROP5,            // ID for crop area 5.
    IP_INFO_ID_DMA_CTL47,        // ID for DMA control 47.
    IP_INFO_ID_LDCI,             // ID for lens distortion correction.
    IP_INFO_ID_DMA_CTL48,        // ID for DMA control 48.
    IP_INFO_ID_DMA_CTL49,        // ID for DMA control 49.
    IP_INFO_ID_PRE_EE,           // ID for pre-edge enhancement.
    // IP_INFO_ID_PCHK6,          // ID for parameter check 6.
    // IP_INFO_ID_PCHK7,          // ID for parameter check 7.

    IP_INFO_ID_ISPTOP,           // ID for ISP top processing.
    IP_INFO_ID_WDMA_CORE0,       // ID for write DMA core 0.
    IP_INFO_ID_RDMA_CORE,        // ID for read DMA core.
    IP_INFO_ID_CSIBDG_LITE,      // ID for lightweight CSI bridge.
    IP_INFO_ID_DMA_CTL0,         // ID for DMA control 0.
    IP_INFO_ID_DMA_CTL1,         // ID for DMA control 1.
    IP_INFO_ID_DMA_CTL2,         // ID for DMA control 2.
    IP_INFO_ID_DMA_CTL3,         // ID for DMA control 3.
    IP_INFO_ID_WDMA_CORE1,       // ID for write DMA core 1.
    IP_INFO_ID_PRE_RAW_VI_SEL,   // ID for pre-raw VI selection.
    IP_INFO_ID_DMA_CTL4,         // ID for DMA control 4.
#if !defined(__CV180X__)
    IP_INFO_ID_DMA_CTL5,         // ID for DMA control 5.
#endif
    // IP_INFO_ID_CMDQ,           // ID for command queue.

    IP_INFO_ID_MAX,              // Maximum ID for IP information groups (used for bounds checking).
};

/* Structure representing IP information with start address and size. */
struct ip_info {
	__u32 str_addr; //IP start address
	__u32 size; //IP total registers size
};

/* struct cvi_vip_memblock
 * @base: the address of the memory allocated.
 * @size: Size in bytes of the memblock.
 */
struct cvi_vip_memblock {
	__u8  raw_num;
	__u64 phy_addr;
	void *vir_addr;
#ifdef __arm__
	__u32 padding;
#endif
	__u32 size;
};

/*Image data block information*/
struct cvi_vip_isp_raw_blk {
    struct cvi_vip_memblock raw_dump; // Memory block for raw dump.
    __u32 frm_num;                  // Number of frames.
    __u32 time_out;                 // Timeout in milliseconds.
    __u16 src_w;                    // Source width.
    __u16 src_h;                    // Source height.
    __u16 crop_x;                   // Crop area X-coordinate.
    __u16 crop_y;                   // Crop area Y-coordinate.
    __u8  is_b_not_rls;             // Flag indicating if buffer is not released.
    __u8  is_timeout;                // Flag indicating if a timeout occurred.
    __u8  is_sig_int;               // Flag indicating if a signal interrupt occurred.
};

/*isp smooth data block information*/
struct cvi_vip_isp_smooth_raw_param {
    struct cvi_vip_isp_raw_blk *raw_blk; // Pointer to raw block information.
    __u8 raw_num;                        // Number of raw blocks.
    __u8 frm_num;                        // Number of frames.
};

/*ISP parameter related state memory*/
struct cvi_isp_sts_mem {
    __u8 raw_num;                     // Number of raw data entries.
    __u8 mem_sts_in_use;             // Memory status in use flag.
    struct cvi_vip_memblock af;      // Memory block for autofocus.
    struct cvi_vip_memblock gms;     // Memory block for gain mapping structure.
    struct cvi_vip_memblock ae_le;   // Memory block for auto exposure low end.
    struct cvi_vip_memblock ae_se;   // Memory block for auto exposure high end.
    struct cvi_vip_memblock awb;     // Memory block for auto white balance.
    struct cvi_vip_memblock awb_post; // Memory block for post auto white balance.
    struct cvi_vip_memblock dci;     // Memory block for dynamic contrast improvement.
    struct cvi_vip_memblock hist_edge_v; // Memory block for histogram edge vertical.
    struct cvi_vip_memblock mmap;    // Memory block for memory mapping.
};

/*Media Bus Frame Format*/
struct cvi_isp_mbus_framefmt {
    __u32 width;  // Width of the frame.
    __u32 height; // Height of the frame.
    __u32 code;   // Format code of the frame.
};

/*Rectangular area*/
struct cvi_isp_rect {
    __s32 left;   // Left coordinate of the rectangle.
    __s32 top;    // Top coordinate of the rectangle.
    __u32 width;  // Width of the rectangle.
    __u32 height; // Height of the rectangle.
};

/*Configure the frame format and cropping area of user images*/
struct cvi_isp_usr_pic_cfg {
    struct cvi_isp_mbus_framefmt fmt; // Frame format configuration.
    struct cvi_isp_rect crop;          // Crop rectangle configuration.
};

/*Information from image sensors*/
struct cvi_isp_snr_info {
    __u8   raw_num;      // Number of raw data entries.
    __u16  color_mode;   // Color mode of the sensor.
    __u32  pixel_rate;   // Pixel rate of the sensor.
    struct wdr_size_s snr_fmt; // Sensor format structure.
};

/*Update information of image sensor*/
struct cvi_isp_snr_update {
    __u8  raw_num;                // Number of raw data entries.
    struct snsr_cfg_node_s snr_cfg_node; // Sensor configuration node.
};

/*YUV processing related parameters*/
struct cvi_vip_isp_yuv_param {
    __u8   raw_num;           // Number of raw data entries.
    __u32  yuv_bypass_path;   // YUV bypass path configuration.
};

/*Parameters for memory mapped grid size*/
struct cvi_isp_mmap_grid_size {
    __u8  raw_num;    // Number of raw data entries.
    __u8  grid_size;  // Size of the grid for memory mapping.
};

/*proc data buffer*/
struct isp_proc_cfg {
    void *buffer;       // Pointer to the buffer for processing.
#ifdef __arm__
    __u32 padding;      // Padding for alignment on ARM architecture.
#endif
    size_t buffer_size; // Size of the buffer.
};

/*Parameters related to ISP AWB status*/
struct cvi_vip_isp_awb_sts {
    __u8  raw_num;  // Number of raw data entries.
    __u8  is_se;    // Flag indicating if SE is enabled.
    __u8  buf_idx;  // Buffer index for AWB data.
};

/*DMA buffer information*/
struct cvi_vi_dma_buf_info {
    __u64  paddr;  // Physical address of the DMA buffer.
    __u32  size;   // Size of the DMA buffer.
};

/*scaler online information*/
struct cvi_isp_sc_online {
    __u8   raw_num;       // Number of raw data entries.
    __u8   is_sc_online;  // Flag indicating if scene configuration is online.
};

#endif // __U_VI_ISP_H__
