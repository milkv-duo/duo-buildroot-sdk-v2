#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include "yolo_v3_detector.h"
#include "pose_detector.h"

#define MEASURE_TIME
#ifdef MEASURE_TIME
#include <sys/time.h>
static long total_elapsed = 0;
#endif

static void usage(char **argv) {
  printf("Usage:\n");
  printf("   %s yolo_cvimodel pose_cvimodel image.jpg output_image_pose.jpg [repeat] "
         "[max_pose]\n",
         argv[0]);
}

int main(int argc, char **argv) {
  int ret = 0;
  if (argc != 5 && argc != 6 && argc != 7) {
    usage(argv);
    exit(-1);
  }
  int repeat = 1;
  if (argc >= 6) {
    repeat = atoi(argv[5]);
  }
  int max_pose = -1;
  if (argc >= 7) {
    max_pose = atoi(argv[6]);
  }

  // imread
  cv::Mat image;
  image = cv::imread(argv[3]);
  if (!image.data) {
    printf("Could not open or find the image\n");
    return -1;
  }

  // register model
  YoloV3Detector yolo_detector(argv[1]);
  PoseDetector pose_detector(argv[2]);

  while (repeat) {
#ifdef MEASURE_TIME
    struct timeval t0, t1;
    long elapsed, gross_total_elapsed = 0;
    gettimeofday(&t0, NULL);
#endif

    detection dets[MAX_DET];
    yolo_detector.doPreProccess(image);
    yolo_detector.doInference();
    int num_det = yolo_detector.doPostProccess(image.rows, image.cols, dets, MAX_DET);

    std::vector<pose_t> pose_list;
    std::vector<bbox_t> align_bbox_list;
    std::vector<cv::Mat> pose_pred_list;

    for (int i = 0; i < num_det; i++) {
      if (i == max_pose)
        break;
      if (dets[i].cls != 0) { // 0 is person
        continue;
      }

      pose_detector.doPreProccess_ResizeOnly(image, dets[i], align_bbox_list);
      pose_detector.doInference();

      // post process
      auto output_tensor = pose_detector.output;
      auto output_shape = CVI_NN_TensorShape(output_tensor);
      cv::Mat pose_pred({output_shape.dim[0], output_shape.dim[1], output_shape.dim[2],
                         output_shape.dim[3]},
                        CV_32FC1, cv::Scalar(0));
      memcpy(pose_pred.data, CVI_NN_TensorPtr(output_tensor),
             CVI_NN_TensorSize(output_tensor));

      pose_pred_list.push_back(pose_pred);
    }

    pose_list.resize(pose_pred_list.size());
    simple_postprocess(pose_pred_list, align_bbox_list, pose_list);

#ifdef MEASURE_TIME
    gettimeofday(&t1, NULL);
    elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
    printf("TIMER: one frame total time %ld us (with %zu pose)\n", elapsed,
           pose_list.size());
    gettimeofday(&t0, NULL);
#endif

    cv::Mat draw_img = draw_pose(image, pose_list);

#ifdef MEASURE_TIME
    gettimeofday(&t1, NULL);
    elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
    printf("TIMER: draw %ld us\n", elapsed);
    t0 = t1;
#endif

    if (repeat == 1) {
      cv::imwrite(argv[4], draw_img);
    }

    printf("------\n");
    printf(" %d poses are detected\n", pose_list.size());
    printf("------\n");

    repeat--;
  }

  return 0;
}
