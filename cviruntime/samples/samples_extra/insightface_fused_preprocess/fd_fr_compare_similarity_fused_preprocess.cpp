#include <iostream>
#include <string>
#include <vector>
#include "face_detector.h"
#include "face_recognizer.h"

static float cal_similarity(cv::Mat feature1, cv::Mat feature2) {
  return feature1.dot(feature2) / (cv::norm(feature1) * cv::norm(feature2));
}

int main(int argc, char *argv[]) {
  // Assume test jpg only has one face per image
  if (argc < 4) {
    std::cout << "Usage: " << argv[0] << " fd.cvimodel fr.cvimodel image1.jpg image2.jpg"
              << std::endl;
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

  FaceDetector fd(argv[1]);
  FaceRecognizer fr(argv[2]);

  fd.doPreProccess_ResizeOnly(image1);
  fd.doInference();
  auto det1 = fd.doPostProccess();

  fr.doPreProccess_ResizeOnly(image1, det1);
  fr.doInference();
  auto feature1 = fr.doPostProccess();

  fd.doPreProccess_ResizeOnly(image2);
  fd.doInference();
  auto det2 = fd.doPostProccess();
  fr.doPreProccess_ResizeOnly(image2, det2);
  fr.doInference();
  auto feature2 = fr.doPostProccess();

  float similarity = cal_similarity(feature1, feature2);

  printf("------\n");
  printf("Similarity: %f\n", similarity);
  printf("------\n");

  return 0;
}
