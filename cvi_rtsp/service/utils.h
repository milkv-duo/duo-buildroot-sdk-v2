#pragma once

typedef enum {
	RAW_12_BIT,
	RAW_16_BIT
} RAW_BITS;


int Bayer_12bit_2_16bit(uint8_t *bayerBuffer, uint16_t *outBuffer,
		uint16_t width, uint16_t height, uint16_t stride);

int get_raw_path_vec(std::string raws_path, std::vector<std::string> &raw_path_vec);

int parse_bin_file(VIDEO_FRAME_INFO_S *frame, std::string raw_path);

int read_binary_file(const std::string &raw_path, void *p_buffer, size_t buffer_len);

int raw_file_12bit_2_16bit(const char *file_path, int width, int height);

void dump_raw(VIDEO_FRAME_INFO_S *p_raw_input, int frm_num, RAW_BITS raw_bit);

void dump_raw_with_ret(VIDEO_FRAME_INFO_S *p_raw_input, int frm_num, const char *ret_name, RAW_BITS raw_bits);

void CLASS_FREE(cvtdl_class_meta_t *cls_meta);

int get_vi_raw(int pipe, VIDEO_FRAME_INFO_S *frame, int *p_frame_num);

int get_file_raw(std::string &raw_path, VIDEO_FRAME_INFO_S *frame, uint32_t height, uint32_t width, uint32_t buffer_size);

