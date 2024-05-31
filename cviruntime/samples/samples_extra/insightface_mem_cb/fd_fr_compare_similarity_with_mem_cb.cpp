#include <iostream>
#include <string>
#include <vector>
#include "face_detector.h"
#include "face_recognizer.h"
#include "mem_alloc.hpp"

static float cal_similarity(cv::Mat feature1, cv::Mat feature2) {
  return feature1.dot(feature2) / (cv::norm(feature1) * cv::norm(feature2));
}

int main(int argc, char *argv[]) {
  // Assume test jpg only has one face per image
  if (argc < 6) {
    std::cout << "Usage: " << argv[0] << " fd.cvimodel fr.cvimodel image1.jpg image2.jpg use_mem_cb"
              << std::endl;
    return -1;
  }

  cv::Mat image1 = cv::imread(argv[3]);
  if (!image1.data) {
    std::cout << "Can not find or open image: " << argv[3] << std::endl;
    return -1;
  }

  cv::Mat image2 = cv::imread(argv[4]);
  if (!image2.data) {
    std::cout << "Can not find or open image: " << argv[4] << std::endl;
    return -1;
  }

  int use_mem_cb = atoi(argv[5]);
  if (1 == use_mem_cb) {
      if (0 != CVI_RT_Global_SetMemAllocCallback(mem_alloc, mem_free)) {
          printf("bind alloc func failed\n");
          return -1;
      }
  }

  FaceDetector fd(argv[1]);
  FaceRecognizer fr(argv[2]);
  print_mem();

  fd.doPreProccess(image1);
  fd.doInference();
  auto det1 = fd.doPostProccess(image1);
  fr.doPreProccess(image1, det1);
  fr.doInference();
  auto feature1 = fr.doPostProccess();

  fd.doPreProccess(image2);
  fd.doInference();
  auto det2 = fd.doPostProccess(image2);
  fr.doPreProccess(image2, det2);
  fr.doInference();
  auto feature2 = fr.doPostProccess();

  float similarity = cal_similarity(feature1, feature2);

  printf("------\n");
  printf("Similarity: %f\n", similarity);
  printf("------\n");

  return 0;
}
