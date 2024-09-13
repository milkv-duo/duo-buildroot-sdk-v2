
#include <fstream>
#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>
#include "sys_utils.hpp"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <sstream>

using namespace std;

std::vector<std::string> read_file_lines(std::string strfile) {
  std::fstream file(strfile);
  std::vector<std::string> lines;
  if (!file.is_open()) {
    return lines;
  }
  std::string line;
  while (getline(file, line)) {
    if (line.length() > 0 && line[0] == '#') continue;
    if (line.at(line.length() - 1) == '\n') {
      line = line.substr(0, line.length() - 1);
    }
    lines.push_back(line);
  }
  return lines;
}
std::string replace_file_ext(const std::string &src_file_name, const std::string &new_file_ext) {
  size_t ext_pos = src_file_name.find_last_of('.');
  size_t seg_pos = src_file_name.find_first_of('/');
  std::string src_name = src_file_name;
  if (seg_pos != src_file_name.npos) {
    src_name.at(seg_pos) = '#';
  }
  std::string str_res;
  if (ext_pos == src_file_name.npos) {
    str_res = src_name + std::string(".") + new_file_ext;
  } else {
    str_res = src_name.substr(0, ext_pos) + std::string(".") + new_file_ext;
  }
  return str_res;
}

bool create_directory(const std::string &str_dir) {
  if (mkdir(str_dir.c_str(), 0777) == -1) return false;
  return true;
}
std::string join_path(const std::string &str_path_parent, const std::string &str_path_sub) {
  std::string str_res = str_path_parent;
  if (str_res.at(str_res.size() - 1) != '/') {
    str_res = str_res + std::string("/");
  }
  str_res = str_res + str_path_sub;
  return str_res;
}
std::string get_directory_name(const std::string &str_path) {
  std::string strname = str_path;
  size_t seg_pos = strname.find_last_of('/');
  if (seg_pos == strname.length() - 1) {  // if '/' is the last pos,drop it then split again
    strname = strname.substr(0, seg_pos);
    seg_pos = strname.find_last_of('/');
  }
  if (seg_pos != strname.npos) {
    strname = strname.substr(seg_pos + 1, strname.length() - 1 - seg_pos);
  }
  return strname;
}

std::vector<std::string> getImgList(std::string dir_path) {
  std::vector<std::string> files;
  DIR *dir;
  struct dirent *ptr;
  // char base[1000];

  if ((dir = opendir(dir_path.c_str())) == NULL) {
    perror("Open dir error...");
    exit(1);
  }

  while ((ptr = readdir(dir)) != NULL) {
    if (strcmp(ptr->d_name, ".") == 0 ||
        strcmp(ptr->d_name, "..") == 0)  /// current dir OR parrent dir
      continue;
    else if (ptr->d_type == 8)  /// file
      // printf("d_name:%s/%s\n",basePath,ptr->d_name);
      files.push_back(ptr->d_name);
    else if (ptr->d_type == 10)  /// link file
      // printf("d_name:%s/%s\n",basePath,ptr->d_name);
      continue;
    else if (ptr->d_type == 4)  /// dir
    {
      files.push_back(ptr->d_name);
      /*
      memset(base,'\0',sizeof(base));
      strcpy(base,basePath);
      strcat(base,"/");
      strcat(base,ptr->d_nSame);
      readFileList(base);
      */
    }
  }
  closedir(dir);
  sort(files.begin(), files.end());
  return files;
}

bool read_binary_file(const std::string &strf, void *p_buffer, int buffer_len) {
  FILE *fp = fopen(strf.c_str(), "rb");
  if (fp == nullptr) {
    std::cout << "read file failed," << strf << std::endl;
    return false;
  }
  fseek(fp, 0, SEEK_END);
  int len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (len != buffer_len) {
    std::cout << "size not equal,expect:" << buffer_len << ",has " << len << std::endl;
    return false;
  }
  fread(p_buffer, len, 1, fp);
  fclose(fp);
  return true;
}

std::string split_file_line(std::string &line, std::vector<std::vector<int>> &boxes) {
  uint32_t pos = line.find(",");
  if (pos == line.npos) {
    return line;
  }
  std::string file_name = line.substr(0, pos);
  std::string res = line.substr(pos + 1);

  istringstream iss(res);
  std::string box;

  std::string val;
  while (getline(iss, box, ',')) {
    istringstream sub_iss(box);
    std::vector<int> vec_box;

    while (getline(sub_iss, val, ' ')) {
      vec_box.push_back(atoi(val.c_str()));
    }
    boxes.push_back(vec_box);
  }

  return file_name;
}