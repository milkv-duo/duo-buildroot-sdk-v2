#ifndef __GRID_INFO_H__
#define __GRID_INFO_H__

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
#if __cplusplus
	extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct _MESH_DATA_ALL_S {
	char grid_name[64];
	bool balloc;
	int num_pairs, imgw, imgh, node_index;
	int *pgrid_src, *pgrid_dst;
	int *pmesh_src, *pmesh_dst;
	int *pnode_src, *pnode_dst;
	int mesh_w;			// unit: pixel
	int mesh_h;			// unit: pixel
	int mesh_horcnt;	// unit: mesh_w
	int mesh_vercnt;	// unit: mesh_h
	int unit_rx;		// unit: mesh_w
	int unit_ry;		// unit: mesh_h
	//int unit_ex;		// = rx + mesh_horcnt - 1
	//int unit_ey;		// = ry + mesh_vercnt - 1
	int _nbr_mesh_x, _nbr_mesh_y;
	bool _bhomo;
	float _homography[10];
	int corners[10];
	float *_pmapx, *_pmapy;
} MESH_DATA_ALL_S;

#define SAFE_FREE_POINTER(ptr)	\
	do {					\
		if (ptr != NULL) {	\
			free(ptr);		\
			ptr = NULL;	\
		}					\
	} while (0)

typedef MESH_DATA_ALL_S meshdata_all;

int load_meshdata(const char *path, MESH_DATA_ALL_S *pmeshdata, const char *bindName);

int free_cur_meshdata(MESH_DATA_ALL_S *pmeshdata);

#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __GRID_INFO_H__ */
