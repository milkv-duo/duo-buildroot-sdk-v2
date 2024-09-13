#pragma once

#include <vector>
#include "opencv2/core.hpp"

namespace cvitdl {

void NormalizeAndSplitToU8(cv::Mat &prepared, const std::vector<float> &mean,
                           const std::vector<float> &scales, std::vector<cv::Mat> &channels);

void NormalizeToU8(cv::Mat *src_channels, const std::vector<float> &mean,
                   const std::vector<float> &scales, std::vector<cv::Mat> &channels);

void AverageAndSplitToF32(cv::Mat &prepared, std::vector<cv::Mat> &channels, float MeanR,
                          float MeanG, float MeanB, int width, int height);

void NormalizeAndSplitToF32(cv::Mat &prepared, std::vector<cv::Mat> &channels, float MeanR,
                            float alphaR, float MeanG, float alphaG, float MeanB, float alphaB,
                            int width, int height);

}  // namespace cvitdl
