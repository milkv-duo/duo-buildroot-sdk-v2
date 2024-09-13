/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: module/vpu/include/gdc_mesh.h
 * Description:
 *   GDC's mesh generator for hw.
 */

#ifndef __GDC_MESH_H_
#define __GDC_MESH_H_

#define CVI_GDC_MAGIC 0xbabeface

#define CVI_GDC_MESH_SIZE_ROT 0x60000
#define CVI_GDC_MESH_SIZE_AFFINE 0x20000
#define CVI_GDC_MESH_SIZE_FISHEYE 0xB0000

enum gdc_task_type {
	GDC_TASK_TYPE_ROT = 0,
	GDC_TASK_TYPE_FISHEYE,
	GDC_TASK_TYPE_AFFINE,
	GDC_TASK_TYPE_LDC,
	GDC_TASK_TYPE_MAX,
};

/* gdc_task_param: the gdc task.
 *
 * stTask: define the in/out image info.
 * type: the type of gdc task.
 * param: the parameters for gdc task.
 */
struct gdc_task_param {
	STAILQ_ENTRY(gdc_task_param) stailq;

	GDC_TASK_ATTR_S stTask;
	enum gdc_task_type type;
	union {
		ROTATION_E enRotation;
		LDC_ATTR_S stLDCAttr;
	};
};

/* gdc_job: the handle of gdc.
 *
 * ctx: the list of gdc task in the gdc job.
 * mutex: used if this job is sync-io.
 * cond: used if this job is sync-io.
 * sync_io: CVI_GDC_EndJob() will blocked until done is this is true.
 *          only meaningful if internal module use gdc.
 *          Default true;
 */
struct gdc_job {
	STAILQ_ENTRY(gdc_job) stailq;

	STAILQ_HEAD(gdc_job_ctx, gdc_task_param) ctx;
	pthread_cond_t cond;
	CVI_BOOL sync_io;
};

int get_mesh_size(int *p_mesh_hor, int *p_mesh_ver);
int set_mesh_size(int mesh_hor, int mesh_ver);
void mesh_gen_get_size(SIZE_S in_size, SIZE_S out_size, CVI_U32 *mesh_id_size, CVI_U32 *mesh_tbl_size);
void mesh_gen_rotation(SIZE_S in_size, SIZE_S out_size, ROTATION_E rot, uint64_t mesh_phy_addr, void *mesh_vir_addr);
CVI_S32 mesh_gen_ldc(SIZE_S in_size, SIZE_S out_size, const LDC_ATTR_S *pstLDCAttr,
		     uint64_t mesh_phy_addr, void *mesh_vir_addr, ROTATION_E rot);

#endif // MODULES_VPU_INCLUDE_GDC_MESH_H_