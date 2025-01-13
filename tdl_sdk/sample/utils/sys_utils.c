#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int compareFileNames(const void *a, const void *b) {
  const char *str1 = *(const char **)a;
  const char *str2 = *(const char **)b;
  return strcmp(str1, str2);
}

char **read_file_lines(const char *filename, int *line_count) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    *line_count = 0;
    return NULL;
  }

  char **lines = NULL;
  char buffer[1024];
  int count = 0;
  while (fgets(buffer, sizeof(buffer), file)) {
    buffer[strcspn(buffer, "\n")] = 0;  // Remove newline
    lines = realloc(lines, sizeof(char *) * (count + 1));
    lines[count] = strdup(buffer);
    count++;
  }
  fclose(file);
  *line_count = count;
  return lines;
}

int count_file_lines(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    return 0;  // File opening failed, returning 0 lines
  }

  char buffer[1024];  // Assuming that a line does not exceed 1024 characters
  int count = 0;

  while (fgets(buffer, sizeof(buffer), file)) {
    size_t len = strlen(buffer);
    if (buffer[len - 1] == '\n') {
      buffer[len - 1] = '\0';  // Remove line breaks
      len--;
    }
    if (len > 0 && buffer[0] == '#') continue;  // Skip comment line
    count++;
  }

  fclose(file);
  return count;
}

char *replace_file_ext(const char *src_file_name, const char *new_file_ext) {
  const char *last_dot = strrchr(src_file_name, '.');
  const char *last_slash = strrchr(src_file_name, '/');
  if (last_slash && last_dot && last_slash > last_dot) {
    last_dot = NULL;  // The dot is part of a directory name, not the file extension
  }

  size_t new_name_len = last_dot ? (size_t)(last_dot - src_file_name) : strlen(src_file_name);
  char *new_name =
      malloc(new_name_len + strlen(new_file_ext) + 2);  // +2 for dot and null terminator
  if (!new_name) return NULL;

  strncpy(new_name, src_file_name, new_name_len);
  new_name[new_name_len] = '.';
  strcpy(new_name + new_name_len + 1, new_file_ext);

  return new_name;
}

bool create_directory(const char *dir_path) {
  if (mkdir(dir_path, 0777) == -1) {
    return false;
  }
  return true;
}

char *join_path(const char *path_parent, const char *path_sub) {
  size_t parent_len = strlen(path_parent);
  size_t sub_len = strlen(path_sub);
  char *result = malloc(parent_len + sub_len + 2);  // +2 for possible '/' and '\0'
  if (result == NULL) {
    return NULL;  // Memory allocation failed
  }
  strcpy(result, path_parent);
  if (result[parent_len - 1] != '/') {
    result[parent_len] = '/';
    result[parent_len + 1] = '\0';
  }
  strcat(result, path_sub);
  return result;
}

char *get_directory_name(const char *path) {
  char *path_copy = strdup(path);  // Create a modifiable copy of path
  if (path_copy == NULL) {
    return NULL;  // Memory allocation failed
  }
  size_t length = strlen(path_copy);
  if (path_copy[length - 1] == '/') {
    path_copy[length - 1] = '\0';  // Remove trailing '/'
  }
  char *last_slash = strrchr(path_copy, '/');
  char *dirname = NULL;
  if (last_slash != NULL) {
    dirname = strdup(last_slash + 1);
  } else {
    dirname = strdup(path_copy);  // No '/' found, return the whole path
  }
  free(path_copy);
  return dirname;
}

int compare_strings(const void *a, const void *b) {
  const char *str1 = *(const char **)a;
  const char *str2 = *(const char **)b;
  return strcmp(str1, str2);
}

/* Function to read a binary file into a buffer */
bool read_binary_file(const char *filename, void *buffer, int expected_size) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Failed to open file %s\n", filename);
    return false;
  }

  /* Determine the file size */
  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size != expected_size) {
    fprintf(stderr, "File size mismatch: expected %d, got %d\n", expected_size, size);
    fclose(file);
    return false;
  }

  /* Read the file into buffer */
  size_t read_size = fread(buffer, 1, size, file);
  fclose(file);

  /* Check if the read was successful */
  if (read_size != size) {
    fprintf(stderr, "Error reading file: expected %d bytes, read %zu bytes\n", size, read_size);
    return false;
  }

  return true;
}

char *split_file_line(char *line, int **boxes, int *box_count) {
  char *token = strtok(line, ",");
  char *file_name = strdup(token);
  const char *delim = " ,";
  int count = 0;
  int *temp_boxes = NULL;

  while ((token = strtok(NULL, delim))) {
    temp_boxes = realloc(temp_boxes, sizeof(int) * (count + 1));
    temp_boxes[count++] = atoi(token);
  }

  *boxes = temp_boxes;
  *box_count = count;
  return file_name;
}