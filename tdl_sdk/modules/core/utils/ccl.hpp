#ifndef FILE_CCL_HPP
#define FILE_CCL_HPP

void* create_connect_instance();

int* extract_connected_component(unsigned char* p_fg_mask, int width, int height, int wstride,
                                 int area_thresh, void* p_cc_inst, int* p_num_boxes);
void destroy_connected_component(void* p_cc_inst);

#endif
