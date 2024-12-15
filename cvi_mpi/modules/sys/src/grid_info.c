#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cvi_debug.h"
#include "grid_info.h"

int load_meshdata(const char *path, MESH_DATA_ALL_S *pmeshdata, const char *bindName)
{
	int info[100] = {0};
	FILE *fpGrid;

	if (path == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "file is null\n");
		return -1;
	}

	fpGrid = fopen(path, "rb");
	if (fpGrid == NULL) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "open file fail, %s\n", path);
		return -1;
	}

	//fread(&pmeshdata->mesh_horcnt, sizeof(int), 1, fpGrid);
	//fread(&pmeshdata->mesh_vercnt, sizeof(int), 1, fpGrid);
	//fread(&pmeshdata->num_pairs, sizeof(int), 1, fpGrid);
	//fread(&pmeshdata->imgw, sizeof(int), 1, fpGrid);
	//fread(&pmeshdata->imgh, sizeof(int), 1, fpGrid);
#if USE_OLD
	if (fread(info, sizeof(int), 20, fpGrid) != 20) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "read file fail, %s\n", path);
		return -1;
	}
#else
	if (fread(info, sizeof(int), 100, fpGrid) != 100) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "read file fail, %s\n", path);
		return -1;
	}
#endif

	CVI_TRACE_SYS(CVI_DBG_DEBUG, "head %d %d %d %d %d\n", info[0], info[1], info[2], info[3], info[4]);
	pmeshdata->mesh_horcnt = info[0]; // num of mesh in roi
	pmeshdata->mesh_vercnt = info[1]; // num of mesh in roi
	pmeshdata->num_pairs = info[2];
	pmeshdata->imgw = info[3];
	pmeshdata->imgh = info[4];
	//
	pmeshdata->mesh_w = info[5];   // unit: pixel
	pmeshdata->mesh_h = info[6];   // unit: pixel
	pmeshdata->unit_rx = info[7];  // unit: mesh_w
	pmeshdata->unit_ry = info[8];  // unit: mesh_h
	pmeshdata->_nbr_mesh_x = info[9];	// total meshes in horizontal
	pmeshdata->_nbr_mesh_y = info[10];	// total meshes in vertical
	memcpy(pmeshdata->corners, info + 11, sizeof(int) * 8);

	int _nbr_mesh_y = pmeshdata->mesh_vercnt; // for roi, not for whole image
	int _nbr_mesh_x = pmeshdata->mesh_horcnt;
	int count_grid = pmeshdata->num_pairs;

	pmeshdata->node_index = (_nbr_mesh_x + 1)*(_nbr_mesh_y + 1);

	strcpy(pmeshdata->grid_name, bindName);

	pmeshdata->pgrid_src = (int *)calloc(count_grid * 2, sizeof(int));
	pmeshdata->pgrid_dst = (int *)calloc(count_grid * 2, sizeof(int));
	pmeshdata->pmesh_src = (int *)calloc(count_grid * 8, sizeof(int));
	pmeshdata->pmesh_dst = (int *)calloc(count_grid * 8, sizeof(int));
	pmeshdata->pnode_src = (int *)calloc(pmeshdata->node_index*2, sizeof(int));
	pmeshdata->pnode_dst = (int *)calloc(pmeshdata->node_index*2, sizeof(int));

	CVI_TRACE_SYS(CVI_DBG_DEBUG,
		"mesh_horcnt,mesh_vercnt,_nbr_mesh_x, _nbr_mesh_y, count_grid, num_nodes: %d %d %d %d %d %d\n",
		pmeshdata->mesh_horcnt, pmeshdata->mesh_vercnt, _nbr_mesh_x, _nbr_mesh_y, count_grid,
		pmeshdata->node_index);
	CVI_TRACE_SYS(CVI_DBG_DEBUG, "imgw, imgh, mesh_w, mesh_h ,unit_rx, unit_ry: %d %d %d %d %d %d",
		pmeshdata->imgw, pmeshdata->imgh, pmeshdata->mesh_w, pmeshdata->mesh_h, pmeshdata->unit_rx,
		pmeshdata->unit_ry);

	if (fread(pmeshdata->pgrid_src, sizeof(int), (count_grid * 2), fpGrid) != (size_t)(count_grid * 2)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "read file fail, %s\n", path);
		return -1;
	}

	if (fread(pmeshdata->pgrid_dst, sizeof(int), (count_grid * 2), fpGrid) != (size_t)(count_grid * 2)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "read file fail, %s\n", path);
		return -1;
	}
	// hw mesh
	if (fread(pmeshdata->pmesh_src, sizeof(int), (count_grid * 2 * 4), fpGrid) != (size_t)(count_grid * 2 * 4)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "read file fail, %s\n", path);
		return -1;
	}
	if (fread(pmeshdata->pmesh_dst, sizeof(int), (count_grid * 2 * 4), fpGrid) != (size_t)(count_grid * 2 * 4)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "read file fail, %s\n", path);
		return -1;
	}
	// hw node
	if (fread(pmeshdata->pnode_src, sizeof(int), (pmeshdata->node_index * 2),
		fpGrid) != (size_t)(pmeshdata->node_index * 2)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "read file fail, %s\n", path);
		return -1;
	}
	if (fread(pmeshdata->pnode_dst, sizeof(int), (pmeshdata->node_index * 2),
		fpGrid) != (size_t)(pmeshdata->node_index * 2)) {
		CVI_TRACE_SYS(CVI_DBG_ERR, "read file fail, %s\n", path);
		return -1;
	}

	fclose(fpGrid);

	pmeshdata->balloc = true;
	CVI_TRACE_SYS(CVI_DBG_DEBUG, "read succ\n");

	return 0;
}

int free_cur_meshdata(MESH_DATA_ALL_S *pmeshdata)
{
	SAFE_FREE_POINTER(pmeshdata->pgrid_src);
	SAFE_FREE_POINTER(pmeshdata->pgrid_dst);
	SAFE_FREE_POINTER(pmeshdata->pmesh_src);
	SAFE_FREE_POINTER(pmeshdata->pmesh_dst);
	SAFE_FREE_POINTER(pmeshdata->pnode_src);
	SAFE_FREE_POINTER(pmeshdata->pnode_dst);

	pmeshdata->balloc = false;
	pmeshdata->_bhomo = false;

	return 0;
}