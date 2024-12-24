#ifndef __SYS_UTILS_H__
#define __SYS_UTILS_H__
char *get_framelist_path(const char *in_path);
char *get_frame_path(const char *in_path);
char *get_out_path(const char *out_path, const char *frame_name);
char *replace_file_ext(const char *src_file_name, const char *new_file_ext);
char *get_directory_name(const char *path);
char *join_path(const char *path_parent, const char *path_sub);
bool create_directory(const char *dir_path);
bool read_binary_file(const char *filename, void *buffer, int expected_size);
char *split_file_line(char *line, int **boxes, int *box_count);
int count_file_lines(const char *filename);
char **read_file_lines(const char *filename, int *line_count);
int compareFileNames(const void *a, const void *b);
char **getImgList(const char *dir_path, int *line_count);
#endif //__SYS_UTILS_H__
