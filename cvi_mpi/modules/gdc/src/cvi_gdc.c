#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/cvi_vi_ctx.h>
#include <linux/vi_uapi.h>
#include <linux/vpss_uapi.h>

#include "cvi_buffer.h"
#include "cvi_sys_base.h"
#include "cvi_sys.h"
#include "cvi_vb.h"

#include "cvi_gdc.h"
#include "gdc_mesh.h"
#include "ldc_ioctl.h"
#include "vi_ioctl.h"
#include "vpss_ioctl.h"
#include "gdc_ctx.h"


// #define LDC_YUV_BLACK 0x808000
// #define LDC_RGB_BLACK 0x0

#define CHECK_GDC_FORMAT(imgIn, imgOut)                                                                                \
	do {                                                                                                           \
		if (imgIn.stVFrame.enPixelFormat != imgOut.stVFrame.enPixelFormat) {                                   \
			CVI_TRACE_GDC(CVI_DBG_ERR, "in/out pixelformat(%d-%d) mismatch\n",                             \
				      imgIn.stVFrame.enPixelFormat, imgOut.stVFrame.enPixelFormat);                    \
			return CVI_ERR_GDC_ILLEGAL_PARAM;                                                              \
		}                                                                                                      \
		if (!GDC_SUPPORT_FMT(imgIn.stVFrame.enPixelFormat)) {                                                  \
			CVI_TRACE_GDC(CVI_DBG_ERR, "pixelformat(%d) unsupported\n", imgIn.stVFrame.enPixelFormat);     \
			return CVI_ERR_GDC_ILLEGAL_PARAM;                                                              \
		}                                                                                                      \
	} while (0)

static CVI_S32 gdc_rotation_check_size(ROTATION_E enRotation, const GDC_TASK_ATTR_S *pstTask)
{
	if (enRotation >= ROTATION_MAX) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "invalid rotation(%d).\n", enRotation);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	if (enRotation == ROTATION_90 || enRotation == ROTATION_270 || enRotation == ROTATION_XY_FLIP) {
		if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
				      enRotation, pstTask->stImgOut.stVFrame.u32Width,
				      pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
				      enRotation, pstTask->stImgOut.stVFrame.u32Height,
				      pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	} else {
		if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
				      enRotation, pstTask->stImgOut.stVFrame.u32Width,
				      pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
				      enRotation, pstTask->stImgOut.stVFrame.u32Height,
				      pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

void gdcq_init(void)
{
}

CVI_S32 CVI_GDC_Suspend(void)
{
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "+\n");

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "-\n");
	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_Resume(void)
{
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "+\n");

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "-\n");

	return CVI_SUCCESS;
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
CVI_S32 CVI_GDC_BeginJob(GDC_HANDLE *phHandle)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, phHandle);

	struct vdev *d = get_dev_info(VDEV_TYPE_DWA, 0);
	if (!IS_VDEV_OPEN(d->state)) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc state(%d) incorrect.", d->state);
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	struct gdc_handle_data cfg;

	memset(&cfg, 0, sizeof(cfg));
	if (gdc_begin_job(d->fd, &cfg))
		return CVI_FAILURE;

	*phHandle = cfg.handle;

	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_EndJob(GDC_HANDLE hHandle)
{
	struct vdev *d = get_dev_info(VDEV_TYPE_DWA, 0);
	if (!IS_VDEV_OPEN(d->state)) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc state(%d) incorrect.", d->state);
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	struct gdc_handle_data cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.handle = hHandle;
	return gdc_end_job(d->fd, &cfg);
}

CVI_S32 CVI_GDC_CancelJob(GDC_HANDLE hHandle)
{
	struct vdev *d = get_dev_info(VDEV_TYPE_DWA, 0);
	if (!IS_VDEV_OPEN(d->state)) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc state(%d) incorrect.", d->state);
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	struct gdc_handle_data cfg;

	memset(&cfg, 0, sizeof(cfg));
	cfg.handle = hHandle;
	return gdc_cancel_job(d->fd, &cfg);
}

CVI_S32 CVI_GDC_AddRotationTask(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask, ROTATION_E enRotation)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstTask);
	CHECK_GDC_FORMAT(pstTask->stImgIn, pstTask->stImgOut);
	UNUSED(hHandle);

	if (enRotation == ROTATION_180) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "do not support rotation 180\n");
		return CVI_ERR_GDC_NOT_SUPPORT;
	}

	if (gdc_rotation_check_size(enRotation, pstTask) != CVI_SUCCESS) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc_rotation_check_size fail\n");
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	struct vdev *d = get_dev_info(VDEV_TYPE_DWA, 0);
	if (!IS_VDEV_OPEN(d->state)) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc state(%d) incorrect.", d->state);
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	struct gdc_task_attr attr;

	memset(&attr, 0, sizeof(attr));
	attr.handle = hHandle;
	memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
	memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
	//memcpy(attr.au64privateData, pstTask->au64privateData, sizeof(attr.au64privateData));
	//attr.reserved = pstTask->reserved;
	attr.enRotation = enRotation;
	return gdc_add_rotation_task(d->fd, &attr);
}

CVI_S32 CVI_GDC_AddLDCTask(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask
	, const LDC_ATTR_S *pstLDCAttr, ROTATION_E enRotation)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstTask);
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstLDCAttr);
	CHECK_GDC_FORMAT(pstTask->stImgIn, pstTask->stImgOut);
	UNUSED(hHandle);

	if (enRotation == ROTATION_180) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "do not support rotation 180\n");
		return CVI_ERR_GDC_NOT_SUPPORT;
	}

	if (enRotation != ROTATION_0) {
		if (gdc_rotation_check_size(enRotation, pstTask) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "gdc_rotation_check_size fail\n");
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}
	struct vdev *d = get_dev_info(VDEV_TYPE_DWA, 0);

	if (!IS_VDEV_OPEN(d->state)) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc state(%d) incorrect.", d->state);
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	struct gdc_task_attr attr;

	memset(&attr, 0, sizeof(attr));
	attr.handle = hHandle;
	memcpy(&attr.stImgIn, &pstTask->stImgIn, sizeof(attr.stImgIn));
	memcpy(&attr.stImgOut, &pstTask->stImgOut, sizeof(attr.stImgOut));
	memcpy(attr.au64privateData, pstTask->au64privateData, sizeof(attr.au64privateData));
	attr.reserved = pstTask->reserved;
	attr.enRotation = enRotation;
	return gdc_add_ldc_task(d->fd, &attr);
}

CVI_S32 CVI_GDC_LoadLDCMesh(CVI_U32 u32Width, CVI_U32 u32Height, const char *fileNname
	, const char *tskName, CVI_U64 *pu64PhyAddr, CVI_VOID **ppVirAddr)
{
	CVI_U64 paddr;
	CVI_VOID *vaddr;
	SIZE_S in_size, out_size;
	CVI_U32 mesh_1st_size = 0, mesh_2nd_size = 0, mesh_size = 0;

	if (!fileNname) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "Please asign name for LDC mesh\n");
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	if (!tskName) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "Please asign task name for LDC\n");
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	in_size.u32Width = ALIGN(u32Width, DEFAULT_ALIGN);
	in_size.u32Height = ALIGN(u32Height, DEFAULT_ALIGN);
	out_size.u32Width = in_size.u32Width;
	out_size.u32Height = in_size.u32Height;

	mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
	mesh_size = mesh_1st_size + mesh_2nd_size;

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "W=%d, H=%d, mesh_size=%d\n"
		, in_size.u32Width, in_size.u32Height, mesh_size);

	// acquire memory space for mesh.
	if (CVI_SYS_IonAlloc_Cached(&paddr, &vaddr, tskName, mesh_size) != CVI_SUCCESS) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "Can't acquire memory for mesh.\n");
		return CVI_ERR_GDC_NOMEM;
	}
	memset(vaddr, 0 , mesh_size);

	FILE *fp = fopen(fileNname, "rb");
	if (!fp) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "open file:%s failed.\n", fileNname);
		CVI_SYS_IonFree(paddr, vaddr);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	fseek(fp, 0, SEEK_END);
	int fileSize = ftell(fp);

	if (mesh_size != (CVI_U32)fileSize) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "loadmesh file:(%s) size is not match.\n", fileNname);
		CVI_SYS_IonFree(paddr, vaddr);
		fclose(fp);
		return CVI_FAILURE;
	}
	rewind(fp);

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "load mesh size:%d, mesh phy addr:%#llx, vir addr:%p.\n",
		mesh_size, paddr, vaddr);

	fread(vaddr, mesh_size, 1, fp);
	fclose(fp);
	CVI_SYS_IonFlushCache(paddr, vaddr, mesh_size);

	*pu64PhyAddr = paddr;
	*ppVirAddr = vaddr;

	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_SetBufWrapAttr(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask, const DWA_BUF_WRAP_S *pstBufWrap)
{
	struct dwa_buf_wrap_cfg *cfg;
	struct vdev *d;
	CVI_S32 s32Ret;

	d = get_dev_info(VDEV_TYPE_DWA, 0);
	if (!IS_VDEV_OPEN(d->state)) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc state(%d) incorrect.", d->state);
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	cfg = malloc(sizeof(*cfg));
	if (!cfg) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc malloc fails.\n");
		return CVI_FAILURE;
	}

	memset(cfg, 0, sizeof(*cfg));
	cfg->handle = hHandle;
	memcpy(&cfg->stTask.stImgIn, &pstTask->stImgIn, sizeof(cfg->stTask.stImgIn));
	memcpy(&cfg->stTask.stImgOut, &pstTask->stImgOut, sizeof(cfg->stTask.stImgOut));
	memcpy(&cfg->stBufWrap, pstBufWrap, sizeof(cfg->stBufWrap));

	s32Ret = gdc_set_chn_buf_wrap(d->fd, cfg);

	free(cfg);

	return s32Ret;
}

CVI_S32 CVI_GDC_GetBufWrapAttr(GDC_HANDLE hHandle, const GDC_TASK_ATTR_S *pstTask, DWA_BUF_WRAP_S *pstBufWrap)
{
	struct dwa_buf_wrap_cfg *cfg;
	struct vdev *d;
	CVI_S32 s32Ret;

	d = get_dev_info(VDEV_TYPE_DWA, 0);
	if (!IS_VDEV_OPEN(d->state)) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc state(%d) incorrect.", d->state);
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	cfg = malloc(sizeof(*cfg));
	if (!cfg) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "gdc malloc fails.\n");
		return CVI_FAILURE;
	}

	memset(cfg, 0, sizeof(*cfg));
	cfg->handle = hHandle;
	memcpy(&cfg->stTask, pstTask, sizeof(cfg->stTask));

	s32Ret = gdc_get_chn_buf_wrap(d->fd, cfg);

	free(cfg);

	if (s32Ret == CVI_SUCCESS)
		memcpy(pstBufWrap, &cfg->stBufWrap, sizeof(*pstBufWrap));

	return s32Ret;
}

CVI_S32 CVI_GDC_DumpMesh(MESH_DUMP_ATTR_S *pMeshDumpAttr)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pMeshDumpAttr);

	CVI_U64 phyMesh;
	CVI_VOID *virMesh;
	CVI_U32 u32Width, u32Height, vpssGrp, vpssChn, viChn;
	SIZE_S in_size, out_size;
	CVI_U32 mesh_1st_size, mesh_2nd_size, meshSize;
	CVI_S32 s32Ret;
	CVI_S32 fd;
	struct vpss_chn_attr attr;

	FILE *fp;
	MOD_ID_E mod = pMeshDumpAttr->enModId;
	CVI_CHAR *filePath = pMeshDumpAttr->binFileName;

	switch (mod) {
	case CVI_ID_VI:
		viChn = pMeshDumpAttr->viMeshAttr.chn;
		phyMesh = g_vi_mesh[viChn].paddr;
		virMesh = g_vi_mesh[viChn].vaddr;
		u32Width = gViCtx->chnAttr[viChn].stSize.u32Width;
		u32Height = gViCtx->chnAttr[viChn].stSize.u32Height;
		in_size.u32Width = ALIGN(u32Width, DEFAULT_ALIGN);
		in_size.u32Height = ALIGN(u32Height, DEFAULT_ALIGN);
		out_size.u32Width = in_size.u32Width;
		out_size.u32Height = in_size.u32Height;
		mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
		meshSize = mesh_1st_size + mesh_2nd_size;
		break;
	case CVI_ID_VPSS:
		fd = get_vpss_fd();
		attr.VpssGrp = vpssGrp = pMeshDumpAttr->vpssMeshAttr.grp;
		attr.VpssChn = vpssChn = pMeshDumpAttr->vpssMeshAttr.chn;
		phyMesh = mesh[vpssGrp][vpssChn].paddr;
		virMesh = mesh[vpssGrp][vpssChn].vaddr;

		s32Ret = vpss_get_chn_attr(fd, &attr);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "Grp(%d) Chn(%d) get chn attr fail\n", vpssGrp, vpssChn);
			return s32Ret;
		}
		u32Width = attr.stChnAttr.u32Width;
		u32Height = attr.stChnAttr.u32Height;
		in_size.u32Width = ALIGN(u32Width, DEFAULT_ALIGN);
		in_size.u32Height = ALIGN(u32Height, DEFAULT_ALIGN);
		out_size.u32Width = in_size.u32Width;
		out_size.u32Height = in_size.u32Height;
		mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
		meshSize = mesh_1st_size + mesh_2nd_size;
		break;
	default:
		CVI_TRACE_GDC(CVI_DBG_ERR, "not supported\n");
		return CVI_ERR_GDC_NOT_SUPPORT;
	}

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "dump mesh size:%d, mesh phy addr:%#llx, vir addr:%p.\n",
		meshSize, phyMesh, virMesh);

	fp = fopen(filePath, "wb");
	if (!fp) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "open file:%s failed.\n", filePath);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}
	fwrite(virMesh, meshSize, 1, fp);
	fflush(fp);
	fclose(fp);
	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_LoadMesh(MESH_DUMP_ATTR_S *pMeshDumpAttr, const LDC_ATTR_S *pstLDCAttr)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pMeshDumpAttr);

	CVI_U64 phyMesh;
	CVI_VOID *virMesh;
	CVI_U32 vpssGrp = 0, vpssChn = 0, viChn = 0;
	SIZE_S in_size, out_size;
	CVI_U32 mesh_1st_size, mesh_2nd_size, mesh_size;
	CVI_U32 u32Width, u32Height;
	struct cvi_gdc_mesh *pmesh;
	FILE *fp;
	CVI_S32 fd;
	MOD_ID_E mod = pMeshDumpAttr->enModId;
	CVI_CHAR *filePath = pMeshDumpAttr->binFileName;
	CVI_S32 s32Ret;
	struct vpss_chn_attr attr;

	switch (mod) {
	case CVI_ID_VI:
		viChn = pMeshDumpAttr->viMeshAttr.chn;
		pmesh = &g_vi_mesh[viChn];
		u32Width = gViCtx->chnAttr[viChn].stSize.u32Width;
		u32Height = gViCtx->chnAttr[viChn].stSize.u32Height;
		in_size.u32Width = ALIGN(u32Width, DEFAULT_ALIGN);
		in_size.u32Height = ALIGN(u32Height, DEFAULT_ALIGN);
		break;
	case CVI_ID_VPSS:
		fd = get_vpss_fd();
		attr.VpssGrp = vpssGrp = pMeshDumpAttr->vpssMeshAttr.grp;
		attr.VpssChn = vpssChn = pMeshDumpAttr->vpssMeshAttr.chn;
		pmesh = &mesh[vpssGrp][vpssChn];

		s32Ret = vpss_get_chn_attr(fd, &attr);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "Grp(%d) Chn(%d) get chn attr fail\n", vpssGrp, vpssChn);
			return s32Ret;
		}
		u32Width = attr.stChnAttr.u32Width;
		u32Height = attr.stChnAttr.u32Height;
		in_size.u32Width = ALIGN(u32Width, DEFAULT_ALIGN);
		in_size.u32Height = ALIGN(u32Height, DEFAULT_ALIGN);
		break;
	default:
		CVI_TRACE_GDC(CVI_DBG_ERR, "not supported\n");
		return CVI_ERR_GDC_NOT_SUPPORT;
	}

	out_size.u32Width = in_size.u32Width;
	out_size.u32Height = in_size.u32Height;

	mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
	mesh_size = mesh_1st_size + mesh_2nd_size;

	fp = fopen(filePath, "rb");
	if (!fp) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "open file:%s failed.\n", filePath);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}
	fseek(fp, 0, SEEK_END);
	int fileSize = ftell(fp);

	if (mesh_size != (CVI_U32)fileSize) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "loadmesh file:(%s) size is not match.\n", filePath);
		fclose(fp);
		return CVI_FAILURE;
	}
	rewind(fp);

	// free last time memory
	if (pmesh->vaddr && pmesh->paddr) {
		CVI_SYS_IonFree(pmesh->paddr, pmesh->vaddr);
		pmesh->vaddr = NULL;
		pmesh->paddr = 0;
	}

	// acquire memory space for mesh.
	if (CVI_SYS_IonAlloc_Cached(&phyMesh, &virMesh, "gdc_mesh", mesh_size) != CVI_SUCCESS) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "Can't acquire memory for gdc mesh.\n");
		fclose(fp);
		return CVI_ERR_VPSS_NOMEM;
	}
	memset(virMesh, 0 , mesh_size);

	CVI_TRACE_GDC(CVI_DBG_DEBUG, "load mesh size:%d, mesh phy addr:%#llx, vir addr:%p.\n",
		mesh_size, phyMesh, virMesh);
	pmesh->paddr = phyMesh;
	pmesh->vaddr = virMesh;

	fread(virMesh, mesh_size, 1, fp);
	CVI_SYS_IonFlushCache(phyMesh, virMesh, mesh_size);

	switch (mod) {
	case CVI_ID_VI:
		g_vi_mesh[viChn].meshSize = mesh_size;
		//vi_ctx.stLDCAttr[viChn].bEnable = CVI_TRUE;

		fd = get_vi_fd();
		struct vi_chn_ldc_cfg vi_cfg;

		vi_cfg.ViChn = viChn;
		vi_cfg.enRotation = ROTATION_0;
		vi_cfg.stLDCAttr.stAttr = *pstLDCAttr;
		vi_cfg.stLDCAttr.bEnable = CVI_TRUE;
		vi_cfg.meshHandle = pmesh->paddr;
		if (vi_sdk_set_chn_ldc(fd, &vi_cfg) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "VI Set Chn(%d) LDC fail\n", viChn);
			fclose(fp);
			return CVI_FAILURE;
		}
		break;
	case CVI_ID_VPSS:
		mesh[vpssGrp][vpssChn].meshSize = mesh_size;
		//vpssCtx[vpssGrp].stChnCfgs[vpssChn].stLDCAttr.bEnable = CVI_TRUE;
		fd = get_vpss_fd();
		struct vpss_chn_ldc_cfg vpss_cfg;

		vpss_cfg.VpssGrp = vpssGrp;
		vpss_cfg.VpssChn = vpssChn;
		vpss_cfg.enRotation = ROTATION_0;
		vpss_cfg.stLDCAttr.stAttr = *pstLDCAttr;
		vpss_cfg.stLDCAttr.bEnable = CVI_TRUE;
		vpss_cfg.meshHandle = pmesh->paddr;
		if (vpss_set_chn_ldc(fd, &vpss_cfg) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "VPSS Set Chn(%d) LDC fail\n", vpssChn);
			fclose(fp);
			return CVI_FAILURE;
		}
		break;
	default:
		CVI_TRACE_GDC(CVI_DBG_ERR, "not supported\n");
		fclose(fp);
		return CVI_ERR_GDC_NOT_SUPPORT;
	}
	fclose(fp);
	return CVI_SUCCESS;
}

CVI_S32 CVI_GDC_LoadMeshWithBuf(MESH_DUMP_ATTR_S *pMeshDumpAttr, const LDC_ATTR_S *pstLDCAttr, CVI_VOID *pBuf, CVI_U32 Len)
{
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pMeshDumpAttr);
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pstLDCAttr);
	MOD_CHECK_NULL_PTR(CVI_ID_GDC, pBuf);

	CVI_U64 phyMesh;
	CVI_VOID *virMesh;
	CVI_U32 vpssGrp = 0, vpssChn = 0, viChn = 0;
	SIZE_S in_size, out_size;
	CVI_U32 mesh_1st_size, mesh_2nd_size, mesh_size;
	CVI_U32 u32Width, u32Height;
	CVI_S32 fd;
	struct cvi_gdc_mesh *pmesh;
	MOD_ID_E mod = pMeshDumpAttr->enModId;
	CVI_S32 s32Ret;
	struct vpss_chn_attr attr;

	switch (mod) {
	case CVI_ID_VI:
		viChn = pMeshDumpAttr->viMeshAttr.chn;
		pmesh = &g_vi_mesh[viChn];
		u32Width = gViCtx->chnAttr[viChn].stSize.u32Width;
		u32Height = gViCtx->chnAttr[viChn].stSize.u32Height;
		in_size.u32Width = ALIGN(u32Width, DEFAULT_ALIGN);
		in_size.u32Height = ALIGN(u32Height, DEFAULT_ALIGN);
		break;
	case CVI_ID_VPSS:
		fd = get_vpss_fd();
		attr.VpssGrp = vpssGrp = pMeshDumpAttr->vpssMeshAttr.grp;
		attr.VpssChn = vpssChn = pMeshDumpAttr->vpssMeshAttr.chn;
		pmesh = &mesh[vpssGrp][vpssChn];

		s32Ret = vpss_get_chn_attr(fd, &attr);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "Grp(%d) Chn(%d) get chn attr fail\n", vpssGrp, vpssChn);
			return s32Ret;
		}
		u32Width = attr.stChnAttr.u32Width;
		u32Height = attr.stChnAttr.u32Height;
		in_size.u32Width = ALIGN(u32Width, DEFAULT_ALIGN);
		in_size.u32Height = ALIGN(u32Height, DEFAULT_ALIGN);
		break;
	default:
		CVI_TRACE_GDC(CVI_DBG_ERR, "not supported\n");
		return CVI_ERR_GDC_NOT_SUPPORT;
	}

	out_size.u32Width = in_size.u32Width;
	out_size.u32Height = in_size.u32Height;

	mesh_gen_get_size(in_size, out_size, &mesh_1st_size, &mesh_2nd_size);
	mesh_size = mesh_1st_size + mesh_2nd_size;
	if (mesh_size != Len) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "invalid param, Len[%d] not match with MOD[%d] meshsize[%d]\n", Len, mod, mesh_size);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	// free last time memory
	if (pmesh->vaddr && pmesh->paddr) {
		CVI_SYS_IonFree(pmesh->paddr, pmesh->vaddr);
		pmesh->vaddr = NULL;
		pmesh->paddr = 0;
	}

	// acquire memory space for mesh.
	if (CVI_SYS_IonAlloc_Cached(&phyMesh, &virMesh, "gdc_mesh", mesh_size) != CVI_SUCCESS) {
		CVI_TRACE_GDC(CVI_DBG_ERR, "Can't acquire memory for gdc mesh.\n");
		return CVI_ERR_GDC_NOMEM;
	}
	memset(virMesh, 0 , mesh_size);
	CVI_TRACE_GDC(CVI_DBG_DEBUG, "load mesh size:%d, mesh phy addr:%#llx, vir addr:%p.\n",
		mesh_size, phyMesh, virMesh);
	pmesh->paddr = phyMesh;
	pmesh->vaddr = virMesh;

	memcpy(virMesh, pBuf, Len);
	CVI_SYS_IonFlushCache(phyMesh, virMesh, mesh_size);

	switch (mod) {
	case CVI_ID_VI:
		g_vi_mesh[viChn].meshSize = mesh_size;
		//vi_ctx.stLDCAttr[viChn].bEnable = CVI_TRUE;

		fd = get_vi_fd();
		struct vi_chn_ldc_cfg vi_cfg;

		vi_cfg.ViChn = viChn;
		vi_cfg.enRotation = ROTATION_0;
		vi_cfg.stLDCAttr.stAttr = *pstLDCAttr;
		vi_cfg.stLDCAttr.bEnable = CVI_TRUE;
		vi_cfg.meshHandle = pmesh->paddr;
		if (vi_sdk_set_chn_ldc(fd, &vi_cfg) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "VI Set Chn(%d) LDC fail\n", viChn);
			return CVI_FAILURE;
		}
		break;
	case CVI_ID_VPSS:
		mesh[vpssGrp][vpssChn].meshSize = mesh_size;
		//vpssCtx[vpssGrp].stChnCfgs[vpssChn].stLDCAttr.bEnable = CVI_TRUE;
		fd = get_vpss_fd();
		struct vpss_chn_ldc_cfg vpss_cfg;

		vpss_cfg.VpssGrp = vpssGrp;
		vpss_cfg.VpssChn = vpssChn;
		vpss_cfg.enRotation = ROTATION_0;
		vpss_cfg.stLDCAttr.stAttr = *pstLDCAttr;
		vpss_cfg.stLDCAttr.bEnable = CVI_TRUE;
		vpss_cfg.meshHandle = pmesh->paddr;
		if (vpss_set_chn_ldc(fd, &vpss_cfg) != CVI_SUCCESS) {
			CVI_TRACE_GDC(CVI_DBG_ERR, "VPSS Set Chn(%d) LDC fail\n", vpssChn);
			return CVI_FAILURE;
		}
		break;
	default:
		CVI_TRACE_GDC(CVI_DBG_ERR, "not supported\n");
		return CVI_ERR_GDC_NOT_SUPPORT;
	}
	return CVI_SUCCESS;
}
