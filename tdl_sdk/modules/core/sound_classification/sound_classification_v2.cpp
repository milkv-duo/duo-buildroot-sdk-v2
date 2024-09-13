#include "sound_classification_v2.hpp"
#include <iostream>
#include <numeric>
#include "cvi_tdl_log.hpp"
using namespace melspec;
using namespace cvitdl;
using namespace std;

SoundClassification::SoundClassification() : Core(CVI_MEM_SYSTEM) {
  audio_param_.win_len = 1024;
  audio_param_.num_fft = 1024;
  audio_param_.hop_len = 256;
  audio_param_.sample_rate = 16000;
  audio_param_.time_len = 3;  // 3 second
  audio_param_.num_mel = 40;
  audio_param_.fmin = 0;
  audio_param_.fmax = audio_param_.sample_rate / 2;
  audio_param_.fix = false;
}

SoundClassification::~SoundClassification() { delete mp_extractor_; }

int SoundClassification::onModelOpened() {
  CVI_SHAPE input_shape = getInputShape(0);
  int32_t image_width = input_shape.dim[2];
  bool htk = false;

  if (image_width == 251 && audio_param_.hop_len == 128) {  // sr16k * 2s, hop_len = 128
    audio_param_.sample_rate = 16000;
    audio_param_.time_len = 2;
  } else if (image_width == 63 || audio_param_.hop_len == 128) {  // sr8k * 2s
    audio_param_.sample_rate = 8000;
    audio_param_.time_len = 2;
  } else if (image_width == 94) {  // sr8k * 3s
    audio_param_.sample_rate = 8000;
    audio_param_.time_len = 3;
  } else if (image_width == 126) {  // sr16k * 2s
    audio_param_.sample_rate = 16000;
    audio_param_.time_len = 2;
  } else if (image_width == 188) {  // sr16k * 3s
    audio_param_.sample_rate = 16000;
    audio_param_.time_len = 3;
  }

  audio_param_.fmax = audio_param_.sample_rate / 2;
  int num_frames = audio_param_.time_len * audio_param_.sample_rate;

  mp_extractor_ = new MelFeatureExtract(num_frames, audio_param_.sample_rate, audio_param_.num_fft,
                                        audio_param_.hop_len, audio_param_.num_mel,
                                        audio_param_.fmin, audio_param_.fmax, "reflect", htk);
  LOGI("model input width:%d,height:%d,sample_rate:%d,time_len:%d\n", image_width,
       input_shape.dim[3], audio_param_.sample_rate, audio_param_.time_len);
  return CVI_SUCCESS;
}

cvitdl_sound_param SoundClassification::get_algparam() { return audio_param_; }
void SoundClassification::set_algparam(cvitdl_sound_param audio_param) {
  audio_param_.win_len = audio_param.win_len;
  audio_param_.num_fft = audio_param.num_fft;
  audio_param_.hop_len = audio_param.hop_len;
  audio_param_.sample_rate = audio_param.sample_rate;
  audio_param_.time_len = audio_param.time_len;
  audio_param_.num_mel = audio_param.num_mel;
  audio_param_.fmin = audio_param.fmin;
  audio_param_.fmax = audio_param.fmax;
  audio_param_.fix = audio_param.fix;
}

int SoundClassification::inference(VIDEO_FRAME_INFO_S *stOutFrame, int *index) {
  int img_width = stOutFrame->stVFrame.u32Width / 2;  // unit: 16 bits
  int img_height = stOutFrame->stVFrame.u32Height;

  // save audio to image array
  short *temp_buffer = reinterpret_cast<short *>(stOutFrame->stVFrame.pu8VirAddr[0]);
  normal_sound(temp_buffer, img_width * img_height);
  mp_extractor_->update_data(temp_buffer, img_width * img_height);

  model_timer_.TicToc("start");

  const TensorInfo &tinfo = getInputTensorInfo(0);
  int8_t *input_ptr = tinfo.get<int8_t>();

  mp_extractor_->melspectrogram_optimze(temp_buffer, img_width * img_height, input_ptr,
                                        int(tinfo.tensor_elem), tinfo.qscale, audio_param_.fix);

  std::vector<VIDEO_FRAME_INFO_S *> frames = {stOutFrame};
  run(frames);

  const TensorInfo &info = getOutputTensorInfo(0);

  // get top k
  *index = get_top_k(info.get<float>(), info.tensor_elem);
  model_timer_.TicToc("post");
  // std::cout<<"output index:"<<*index<<std::endl;
  return CVI_SUCCESS;
}

int SoundClassification::inference_pack(VIDEO_FRAME_INFO_S *stOutFrame, int pack_idx, int pack_len,
                                        int *index) {
  int img_width = stOutFrame->stVFrame.u32Width / 2;  // unit: 16 bits
  int img_height = stOutFrame->stVFrame.u32Height;

  // save audio to image array
  short *temp_buffer = reinterpret_cast<short *>(stOutFrame->stVFrame.pu8VirAddr[0]);
  // normal_sound(temp_buffer, img_width * img_height);
  model_timer_.TicToc("start");

  const TensorInfo &tinfo = getInputTensorInfo(0);
  int8_t *input_ptr = tinfo.get<int8_t>();

  mp_extractor_->melspectrogram_pack_optimize(temp_buffer, img_width * img_height, pack_len,
                                              pack_idx, input_ptr, int(tinfo.tensor_elem),
                                              tinfo.qscale, audio_param_.fix);

  std::vector<VIDEO_FRAME_INFO_S *> frames = {stOutFrame};
  run(frames);

  const TensorInfo &info = getOutputTensorInfo(0);

  // get top k
  *index = get_top_k(info.get<float>(), info.tensor_elem);
  model_timer_.TicToc("post");

  return CVI_SUCCESS;
}
int SoundClassification::get_top_k(float *result, size_t count) {
  int idx = -1;
  float max_e = -10000;
  float cur_e;

  float sum_e = 0.;
  for (size_t i = 0; i < count; i++) {
    cur_e = std::exp(result[i]);
    if (i != 0 && cur_e > max_e) {
      max_e = cur_e;
      idx = i;
    }
    sum_e = float(sum_e) + float(cur_e);
    // std::cout << i << ": " << cur_e << "\t";
  }
  // std::cout << "\n";
  // for (size_t i = 0; i < count; i++) {
  //   cur_e = std::exp(result[i]) / sum_e;
  //   std::cout << "  i:" << i << ", score:" << cur_e;
  // }

  float max = max_e / sum_e;
  if (idx != 0 && max < m_model_threshold) {
    idx = 0;
  }
  return idx;
}

void SoundClassification::normal_sound(short *audio_data, int n) {
  // std::cout << "before:" << audio_data[0];
  std::vector<double> audio_abs(n);
  for (int i = 0; i < n; i++) {
    audio_abs[i] = std::abs(static_cast<double>(audio_data[i]));
  }
  std::vector<double> top_data;
  std::make_heap(audio_abs.begin(), audio_abs.end());
  if (top_num <= 0) {
    std::cerr << "When top_num<=0, the volume adaptive algorithm will fail. Current top_num="
              << top_num << std::endl;
    return;
  }
  for (int i = 0; i < top_num; i++) {
    top_data.push_back(audio_abs.front());
    std::pop_heap(audio_abs.begin(), audio_abs.end());
    audio_abs.pop_back();
  }
  double top_mean = std::accumulate(top_data.begin(), top_data.end(), 0.0) / top_num;
  if (top_mean == 0) {
    std::cout << "The average of the top data is zero, cannot scale the audio data." << std::endl;
  } else {
    double r = max_rate * SCALE_FACTOR_FOR_INT16 / double(top_mean);
    double tmp = 0;
    for (int i = 0; i < n; i++) {
      tmp = audio_data[i] * r;
      audio_data[i] = short(tmp);
    }
  }
  // std::cout << ", after:" << audio_data[0];
}

int SoundClassification::getClassesNum() {
  const TensorInfo &info = getOutputTensorInfo(0);
  return info.tensor_elem;
}