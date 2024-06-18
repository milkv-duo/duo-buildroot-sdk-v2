#include "cvi_base.h"

#include "isp_control.h"
#include "isp_debug.h"
#include "isp_ioctl.h"
#include "isp_3a.h"

#include "isp_comm_inc.h"

CVI_S32 isp_control_set_fd_info(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;
	struct vdev *d;

	d = get_dev_info(VDEV_TYPE_ISP, ViPipe);

	ISP_GET_CTX(ViPipe, pstIspCtx);
	pstIspCtx->ispDevFd = d->fd;

	if (!IS_VDEV_OPEN(d->state)) {
		ISP_LOG_ERR("Pipe(%d) state(%d) incorrect\n", ViPipe, d->state);
		return -EBADF;
	}

	return ret;
}

CVI_S32 isp_control_set_scene_info(VI_PIPE ViPipe)
{
	CVI_U32 scene = 0;
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	G_EXT_CTRLS_VALUE(VI_IOCTL_GET_SCENE_INFO, 0, &scene);

	pstIspCtx->scene = scene;

	return CVI_SUCCESS;
}

CVI_S32 isp_control_get_scene_info(VI_PIPE ViPipe, enum ISP_SCENE_INFO *scene)
{
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	*scene = pstIspCtx->scene;

	return CVI_SUCCESS;
}
