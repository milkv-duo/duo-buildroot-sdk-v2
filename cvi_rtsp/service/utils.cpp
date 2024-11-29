#include "ctx.h"
#include "vpss_helper.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cvi_isp.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include "utils.h"
#include "cvi_awb.h"

int Bayer_12bit_2_16bit(uint8_t *bayerBuffer, uint16_t *outBuffer,
		uint16_t width, uint16_t height, uint16_t stride)
{
	uint32_t r, c;
	uint32_t au16Temp[2] = {};

	if (bayerBuffer == NULL || outBuffer == NULL) {
		std::cout << "pointer is NULL" << std::endl;
		return 1;
	}

	uint16_t *p = outBuffer;

	for (r = 0; r < height; r++) {
		for (c = 0; c < width; c += 2) {
			au16Temp[0] = (*bayerBuffer << 4) + (*(bayerBuffer + 2) & 0xF);
			au16Temp[1] = (*(bayerBuffer + 1) << 4) + ((*(bayerBuffer + 2) & 0xF0) >> 4);
			*p = au16Temp[0];
			*(p+1) = au16Temp[1];
			bayerBuffer += 3;
			p += 2;
		}
		bayerBuffer += (uint16_t)((stride - width)*1.5);
	}

	return 0;
}

int get_raw_path_vec(std::string raws_path, std::vector<std::string> &raw_path_vec)
{
	int ret;
	std::fstream file(raws_path);

	if (!file.is_open()) {
		std::cout << "open file " << raws_path << " failed" << std::endl;
		return -1;
	}

	std::string line;

	while (getline(file, line)) {
		if (line.empty()) {
			continue;
		}
		raw_path_vec.push_back(line);
	}

	ret = raw_path_vec.size() > 0 ? 0 : -1;

	return ret;
}

int parse_bin_file(VIDEO_FRAME_INFO_S *frame, std::string raw_path)
{
	std::ifstream file(raw_path, std::ios::binary);

	if (!file.is_open()) {
		std::cout << "open file " << raw_path << " failed" << std::endl;
		return -1;
	}

	frame->stVFrame.u32Width = 1440;
	frame->stVFrame.u32Height = 3840;
	frame->stVFrame.u32Length[0] = 1440 * 3840;

	file.read(reinterpret_cast<char*>(frame->stVFrame.pu8VirAddr[0]), frame->stVFrame.u32Length[0]);

	file.close();

	return 0;
}

int read_binary_file(const std::string &raw_path, void *p_buffer, size_t buffer_len)
{
	FILE *fp = fopen(raw_path.c_str(), "rb");
	size_t len;

	if (fp == nullptr) {
		std::cout << "read file failed," << raw_path << std::endl;
		return false;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);

	fseek(fp, 0, SEEK_SET);

	if (len != buffer_len) {
		std::cout << "size not equal,expect:" << buffer_len << ",has " << len << std::endl;
		return false;
	}

	fread(p_buffer, len, 1, fp);
	fclose(fp);

	return 0;
}

int raw_file_12bit_2_16bit(const char *file_path, int width, int height)
{
	int ret;

	FILE *fd = fopen(file_path, "rb");

	if (!fd) {
		printf("can't open the file %s\n", file_path);
		return -1;
	}

	size_t file_size;

	fseek(fd, 0, SEEK_END);
	file_size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	CVI_U8 *bayer_buffer = (CVI_U8 *)calloc(1, file_size);
	CVI_U16 *out_buffer = (CVI_U16 *)calloc(1, height * width * 2);

	fread(bayer_buffer, 1, file_size, fd);

	// convert
	ret = Bayer_12bit_2_16bit(bayer_buffer, out_buffer, width, height, width);

	// save the file
	char out_file_name[128];

	snprintf(out_file_name, sizeof(out_file_name), "%s_16bit.raw", file_path);

	FILE *out_fd = fopen(out_file_name, "wb");

	if (!out_fd) {
		printf("can't open the file %s\n", out_file_name);
		return -1;
	}

	fwrite(out_buffer, 1, height * width * 2, out_fd);

	fclose(fd);
	fclose(out_fd);

	if (bayer_buffer) {
		free(bayer_buffer);
	}

	if (out_buffer) {
		free(out_buffer);
	}

	return ret;
}

void dump_raw(VIDEO_FRAME_INFO_S *p_raw_input, int frm_num, RAW_BITS raw_bits)
{
	struct timeval tv1;
	FILE *output;
	char img_name[128] = {0,}, order_id[8] = {0,};
	static uint64_t frame_id = 0;

	switch (p_raw_input[0].stVFrame.enBayerFormat) {
	case BAYER_FORMAT_BG:
		snprintf(order_id, sizeof(order_id), "BG");
		break;
	case BAYER_FORMAT_GB:
		snprintf(order_id, sizeof(order_id), "GB");
		break;
	case BAYER_FORMAT_GR:
		snprintf(order_id, sizeof(order_id), "GR");
		break;
	case BAYER_FORMAT_RG:
		snprintf(order_id, sizeof(order_id), "RG");
		break;
	default:
		break;
	}

	frame_id++;

	if (frame_id % 5 != 0) {
		return;
	}

	gettimeofday(&tv1, NULL);
	snprintf(img_name, sizeof(img_name),
			"./vi_0_%s_%s_w_%d_h_%d_x_%d_y_%d_tv_%ld_%ld_%ld.raw",
			frm_num == 1 ? "le" : "lese",
			order_id,
			p_raw_input[0].stVFrame.u32Width,
			p_raw_input[0].stVFrame.u32Height,
			p_raw_input[0].stVFrame.s16OffsetLeft,
			p_raw_input[0].stVFrame.s16OffsetTop,
			tv1.tv_sec, tv1.tv_usec,
			frame_id);

	CVI_TRACE_LOG(CVI_DBG_WARN, "dump image %s\n", img_name);

	output = fopen(img_name, "wb");

	switch (raw_bits) {
	case RAW_12_BIT:
		for (int i = 0; i < frm_num; i++) {
			p_raw_input[i].stVFrame.pu8VirAddr[0]
				= (CVI_U8 *)CVI_SYS_Mmap(p_raw_input[i].stVFrame.u64PhyAddr[0] , p_raw_input[i].stVFrame.u32Length[0]);

			fwrite(p_raw_input[i].stVFrame.pu8VirAddr[0], p_raw_input[i].stVFrame.u32Length[0], 1, output);

			CVI_SYS_Munmap((void *)p_raw_input[i].stVFrame.pu8VirAddr[0],
					p_raw_input[i].stVFrame.u32Length[0]);
		}
		break;
	default:
	case RAW_16_BIT:
		size_t u16_image_size = p_raw_input[0].stVFrame.u32Width * p_raw_input[0].stVFrame.u32Height * sizeof(CVI_U16);
		CVI_U16 *pr = (CVI_U16 *)calloc(1, u16_image_size);

		for (int i = 0; i < frm_num; i++) {
			p_raw_input[i].stVFrame.pu8VirAddr[0]
				= (CVI_U8 *)CVI_SYS_Mmap(p_raw_input[i].stVFrame.u64PhyAddr[0] , p_raw_input[i].stVFrame.u32Length[0]);

			Bayer_12bit_2_16bit(p_raw_input[i].stVFrame.pu8VirAddr[0], pr,
						p_raw_input[i].stVFrame.u32Width, p_raw_input[i].stVFrame.u32Height,
						p_raw_input[i].stVFrame.u32Width);

			fwrite(pr, u16_image_size, 1, output);

			CVI_SYS_Munmap((void *)p_raw_input[i].stVFrame.pu8VirAddr[0],
					p_raw_input[i].stVFrame.u32Length[0]);
		}
		break;
	}

	fclose(output);
}

void dump_raw_with_ret(VIDEO_FRAME_INFO_S *p_raw_input, int frm_num, const char *ret_name, RAW_BITS raw_bits)
{
	FILE *output;
	static uint64_t frame_id = 0;

	frame_id++;

	if (frame_id % 5 != 0) {
		return;
	}

	output = fopen(ret_name, "wb");

	switch (raw_bits) {
	case RAW_12_BIT:
		for (int i = 0; i < frm_num; i++) {
			p_raw_input[i].stVFrame.pu8VirAddr[0]
				= (CVI_U8 *)CVI_SYS_Mmap(p_raw_input[i].stVFrame.u64PhyAddr[0] , p_raw_input[i].stVFrame.u32Length[0]);

			fwrite(p_raw_input[i].stVFrame.pu8VirAddr[0], p_raw_input[i].stVFrame.u32Length[0], 1, output);

			CVI_SYS_Munmap((void *)p_raw_input[i].stVFrame.pu8VirAddr[0],
					p_raw_input[i].stVFrame.u32Length[0]);
		}
		break;
	default:
	case RAW_16_BIT:
		size_t u16_image_size = p_raw_input[0].stVFrame.u32Width * p_raw_input[0].stVFrame.u32Height * sizeof(CVI_U16);
		CVI_U16 *pr = (CVI_U16 *)calloc(1, u16_image_size);

		for (int i = 0; i < frm_num; i++) {
			p_raw_input[i].stVFrame.pu8VirAddr[0]
				= (CVI_U8 *)CVI_SYS_Mmap(p_raw_input[i].stVFrame.u64PhyAddr[0] , p_raw_input[i].stVFrame.u32Length[0]);

			Bayer_12bit_2_16bit(p_raw_input[i].stVFrame.pu8VirAddr[0], pr,
						p_raw_input[i].stVFrame.u32Width, p_raw_input[i].stVFrame.u32Height,
						p_raw_input[i].stVFrame.u32Width);

			fwrite(pr, u16_image_size, 1, output);

			CVI_SYS_Munmap((void *)p_raw_input[i].stVFrame.pu8VirAddr[0],
					p_raw_input[i].stVFrame.u32Length[0]);
		}
		break;
	}

	fclose(output);
}

int get_vi_raw(int pipe, VIDEO_FRAME_INFO_S *frame, int *p_frame_num)
{
	// frame is array with two element
	int ret;
	VI_DUMP_ATTR_S attr;

	memset(frame, 0, 2 * sizeof(VIDEO_FRAME_INFO_S));

	attr.bEnable = 1;
	attr.u32Depth = 0;
	attr.enDumpType = VI_DUMP_TYPE_RAW;
	ret = CVI_VI_SetPipeDumpAttr(pipe, &attr);

	if (ret != CVI_SUCCESS) {
		printf("pipe: %d, set vi dump attribute fail!\n", pipe);
		return ret;
	}

	attr.bEnable = 0;
	attr.enDumpType = VI_DUMP_TYPE_IR;
	CVI_VI_GetPipeDumpAttr(pipe, &attr);

	if (attr.bEnable != 1 || attr.enDumpType != VI_DUMP_TYPE_RAW) {
		printf("ERROR: Enable(%d), DumpType(%d)\n", attr.bEnable, attr.enDumpType);
		return CVI_FAILURE;
	}

	if (CVI_VI_GetPipeFrame(pipe, frame, 1000) != CVI_SUCCESS) {
		printf("CVI_VI_GetPipeFrame failed\n");
		return -1;
	}

	if (frame[1].stVFrame.u64PhyAddr[0] != 0) {
		*p_frame_num = 2;
	}

	/*
	std::cout << "frame width: " << frame[0].stVFrame.u32Width << ", height: " << frame[0].stVFrame.u32Height;
	std::cout << ", length: " << frame[0].stVFrame.u32Length[0] << std::endl;
	*/

	return 0;
}

int get_file_raw(std::string &raw_path, VIDEO_FRAME_INFO_S *frame, uint32_t height, uint32_t width, uint32_t buffer_size)
{
	// set frame info
	memset(frame, 0, sizeof(VIDEO_FRAME_INFO_S));
	frame->stVFrame.u32Width = width;
	frame->stVFrame.u32Height = height;
	frame->stVFrame.u32Length[0] = buffer_size;

	uint8_t *buffer = (uint8_t *)calloc(1, buffer_size);

	if (!buffer) {
		std::cout << "alloc memory fail!" << std::endl;
		return -1;
	}

	frame->stVFrame.pu8VirAddr[0] = buffer;  // remember to free this memory

	// parser raw file
	std::ifstream file(raw_path, std::ios::binary);

	if (!file.is_open()) {
		std::cout << "open file " << raw_path << " failed" << std::endl;
		return -1;
	}

	file.read(reinterpret_cast<char*>(frame->stVFrame.pu8VirAddr[0]), frame->stVFrame.u32Length[0]);

	file.close();

	return 0;
}

int get_pq_parameter(int pipe, PQ_PARAMETER_S *p_pq_param)
{
	ISP_WB_Q_INFO_S awb_info;
	int ret = 0;

	ret = CVI_AWB_QueryInfo(pipe, &awb_info);

	if (ret != 0)
		return -1;

	p_pq_param->awb_bgain = awb_info.u16Bgain;
	p_pq_param->awb_rgain = awb_info.u16Rgain;
	p_pq_param->awb_ggain = awb_info.u16Grgain;

	return 0;
}

void CLASS_FREE(cvtdl_class_meta_t *cls_meta)
{
	for (int i = 0; i < 5; i++) {
		cls_meta->cls[i] = 0;
		cls_meta->score[i] = 0.0;
	}
}

