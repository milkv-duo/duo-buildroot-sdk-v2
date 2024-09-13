#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "cityscapes.hpp"
#include "cvi_tdl_log.hpp"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cvi_comm.h"

#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"

#define IMAGE_DIR1 "/frankfurt/"
#define IMAGE_DIR2 "/lindau/"
#define IMAGE_DIR3 "/munster/"

namespace cvitdl {
namespace evaluation {

const int8_t valid_classes[19] = {7,  8,  11, 12, 13, 17, 19, 20, 21, 22,
                                  23, 24, 25, 26, 27, 28, 31, 32, 33};

static void readImage(std::string img_dir, std::vector<std::string> &images) {
  DIR *dirp;
  struct dirent *entry;
  dirp = opendir(img_dir.c_str());

  while ((entry = readdir(dirp)) != NULL) {
    if (entry->d_type != 8 && entry->d_type != 0) continue;

    images.push_back(img_dir + "/" + entry->d_name);
  }
}

cityscapesEval::cityscapesEval(const char *root_dir, const char *output_dir) {
  std::string dir_str = root_dir;

  readImage(dir_str + std::string(IMAGE_DIR1), m_images);
  readImage(dir_str + std::string(IMAGE_DIR2), m_images);
  readImage(dir_str + std::string(IMAGE_DIR3), m_images);

  if (0 != mkdir(output_dir, S_IRWXO) && EEXIST != errno) {
    printf("Create %s failed.\n", output_dir);
  }

  m_output_dir = output_dir;
}

void cityscapesEval::getImage(int index, std::string &image_path) { image_path = m_images[index]; }
void cityscapesEval::getImageNum(uint32_t *num) { *num = m_images.size(); }

void cityscapesEval::writeResult(VIDEO_FRAME_INFO_S *label_frame, const int index) {
  label_frame->stVFrame.pu8VirAddr[0] = (CVI_U8 *)CVI_SYS_Mmap(label_frame->stVFrame.u64PhyAddr[0],
                                                               label_frame->stVFrame.u32Length[0]);

  cv::Mat image(label_frame->stVFrame.u32Height, label_frame->stVFrame.u32Width, CV_8SC1,
                label_frame->stVFrame.pu8VirAddr[0], label_frame->stVFrame.u32Stride[0]);

  for (int i = 0; i < image.rows; ++i) {
    uchar *pixel = image.ptr<uchar>(i);
    for (int j = 0; j < image.cols; ++j) {
      pixel[j] = valid_classes[pixel[j]];
    }
  }

  std::string full_path =
      m_output_dir + "/" + m_images[index].substr(m_images[index].rfind('/') + 1);
  remove(full_path.c_str());
  cv::imwrite(full_path, image);

  CVI_SYS_Munmap((void *)label_frame->stVFrame.pu8VirAddr[0], label_frame->stVFrame.u32Length[0]);
  label_frame->stVFrame.pu8VirAddr[0] = NULL;
}

}  // namespace evaluation
}  // namespace cvitdl
