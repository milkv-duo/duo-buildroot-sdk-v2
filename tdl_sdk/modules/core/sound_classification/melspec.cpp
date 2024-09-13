#include "melspec.hpp"
#include <iostream>
#include "cvi_tdl_log.hpp"

using namespace melspec;

static Matrixf melfilter(int sr, int n_fft, int n_mels, int fmin, int fmax, bool htk) {
  int n_f = n_fft / 2 + 1;
  Vectorf fft_freqs = (Vectorf::LinSpaced(n_f, 0.f, static_cast<float>(n_f - 1)) * sr) / n_fft;

  float f_min = 0.f;
  float f_sp = 200.f / 3.f;
  float min_log_hz = 1000.f;
  float min_log_mel = (min_log_hz - f_min) / f_sp;
  float logstep = logf(6.4f) / 27.f;

  auto hz_to_mel = [=](int hz, bool htk) -> float {
    if (htk) {
      return 2595.0f * log10f(1.0f + hz / 700.0f);
    }
    float mel = (hz - f_min) / f_sp;
    if (hz >= min_log_hz) {
      mel = min_log_mel + logf(hz / min_log_hz) / logstep;
    }
    return mel;
  };
  auto mel_to_hz = [=](Vectorf &mels, bool htk) -> Vectorf {
    if (htk) {
      return 700.0f *
             (Vectorf::Constant(n_mels + 2, 10.f).array().pow(mels.array() / 2595.0f) - 1.0f);
    }
    return (mels.array() > min_log_mel)
        .select(((mels.array() - min_log_mel) * logstep).exp() * min_log_hz,
                (mels * f_sp).array() + f_min);
  };

  float min_mel = hz_to_mel(fmin, htk);
  float max_mel = hz_to_mel(fmax, htk);
  Vectorf mels = Vectorf::LinSpaced(n_mels + 2, min_mel, max_mel);
  Vectorf mel_f = mel_to_hz(mels, htk);
  Vectorf fdiff = mel_f.segment(1, mel_f.size() - 1) - mel_f.segment(0, mel_f.size() - 1);
  Matrixf ramps =
      mel_f.replicate(n_f, 1).transpose().array() - fft_freqs.replicate(n_mels + 2, 1).array();

  Matrixf lower = -ramps.topRows(n_mels).array() /
                  fdiff.segment(0, n_mels).transpose().replicate(1, n_f).array();
  Matrixf upper = ramps.bottomRows(n_mels).array() /
                  fdiff.segment(1, n_mels).transpose().replicate(1, n_f).array();
  Matrixf weights = (lower.array() < upper.array()).select(lower, upper).cwiseMax(0);

  auto enorm = (2.0 / (mel_f.segment(2, n_mels) - mel_f.segment(0, n_mels)).array())
                   .transpose()
                   .replicate(1, n_f);
  weights = weights.array() * enorm;

  return weights.transpose();
}

void quant_feat(float *p_feat_src, float q_scale, int len, float logmin_val, int8_t *p_feat_quant) {
  for (int i = 0; i < len; i++) {
    float v = p_feat_src[i];
    if (logmin_val != 0) {
      if (v < logmin_val) v = logmin_val;
      v = 10 * log10f(v);
    }
    int16_t qval = v * q_scale;
    if (qval < -128) {
      // std::cout<<"overflow qval:"<<qval<<std::endl;
      qval = -128;
    } else if (qval > 127) {
      // std::cout<<"overflow qval:"<<qval<<std::endl;
      qval = 127;
    }
    p_feat_quant[i] = qval;
  }
}

MelFeatureExtract::MelFeatureExtract(int num_frames, int sr, int n_fft, int n_hop, int n_mel,
                                     int fmin, int fmax, const std::string &mode, bool htk,
                                     bool center /*=true*/, int power /*=2*/,
                                     bool is_log /*=true*/) {
  num_wav_len_ = num_frames;
  num_fft_ = n_fft;
  num_mel_ = n_mel;
  sample_rate_ = sr;
  num_hop_ = n_hop;
  fmin_ = fmin;
  fmax_ = fmax;
  center_ = center;
  power_ = power;
  mode_ = mode;
  int pad_len = center ? n_fft / 2 : 0;
  pad_len_ = pad_len;

  window_ =
      0.5 *
      (1.f - (Vectorf::LinSpaced(n_fft, 0.f, static_cast<float>(n_fft - 1)) * 2.f * M_PI / n_fft)
                 .array()
                 .cos());
  mel_basis_ = melfilter(sr, n_fft, n_mel, fmin, fmax, htk);
  is_log_ = is_log;
}
MelFeatureExtract::~MelFeatureExtract() {
  if (mp_sft_mag_vec_ != nullptr) {
    delete mp_sft_mag_vec_;
    mp_sft_mag_vec_ = nullptr;
  }
}
void MelFeatureExtract::pad(Vectorf &x, int left, int right, const std::string &mode, float value) {
  // Vectorf x_pad_ = Vectorf::Constant(left+x.size()+right, value);
  if (x_pad_.size() == 0) {
    x_pad_ = Vectorf::Constant(left + right + x.size(), 0);
  }
  x_pad_.segment(left, x.size()) = x;

  if (mode.compare("reflect") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = x[left - i];
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + x.size()] = x[x.size() - 2 - i + left];
    }
  }

  if (mode.compare("symmetric") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = x[left - i - 1];
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + x.size()] = x[x.size() - 1 - i + left];
    }
  }

  if (mode.compare("edge") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = x[0];
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + x.size()] = x[x.size() - 1];
    }
  }
}

static Matrixcf stft(Vectorf &x_paded, Vectorf &window, int n_fft, int n_hop,
                     const std::string &win, bool center, const std::string &mode) {
  // hanning
  // Vectorf window = 0.5*(1.f-(Vectorf::LinSpaced(n_fft, 0.f,
  // static_cast<float>(n_fft-1))*2.f*M_PI/n_fft).array().cos());

  // int pad_len = center ? n_fft / 2 : 0;
  // Vectorf x_paded = pad(x, pad_len, pad_len, mode, 0.f);

  int n_f = n_fft / 2 + 1;
  int n_frames = 1 + (x_paded.size() - n_fft) / n_hop;
  Matrixcf X(n_frames, n_fft);
  Eigen::FFT<float> fft;

  for (int i = 0; i < n_frames; ++i) {
    Vectorf segment = x_paded.segment(i * n_hop, n_fft);
    Vectorf x_frame = window.array() * x_paded.segment(i * n_hop, n_fft).array();
    X.row(i) = fft.fwd(x_frame);
  }
  return X.leftCols(n_f);
}

static Matrixf spectrogram(Matrixcf &X, float power = 1.f) {
  return X.cwiseAbs().array().pow(power);
}

void MelFeatureExtract::update_data(short *p_data, int data_len) {
  if (x_pad_.cols() == 0) {
    x_pad_ = Vectorf::Constant(pad_len_ * 2 + data_len, 0);
  }
  int num_len = int(x_pad_.size()) - 2 * pad_len_;
  if (num_len != data_len) {
    LOGE("size error\n");
  }
  for (int i = 0; i < data_len; i++) {
    x_pad_[i + pad_len_] = p_data[i] / 32768.0;
  }
  int left = pad_len_;
  int right = pad_len_;
  if (mode_.compare("reflect") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = p_data[left - i] / 32768.0;
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + data_len] = p_data[data_len - 2 - i + left] / 32768.0;
    }
  }

  if (mode_.compare("symmetric") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = p_data[left - i - 1] / 32768.0;
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + data_len] = p_data[data_len - 1 - i + left] / 32768.0;
    }
  }

  if (mode_.compare("edge") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = p_data[0] / 32768.0;
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + data_len] = p_data[data_len - 1] / 32768.0;
    }
  }
}
void MelFeatureExtract::update_float_data(float *p_data, int data_len) {
  if (x_pad_.cols() == 0) {
    x_pad_ = Vectorf::Constant(pad_len_ * 2 + data_len, 0);
  }
  int num_len = int(x_pad_.size()) - 2 * pad_len_;
  if (num_len != data_len) {
    LOGE("size error\n");
  }
  for (int i = 0; i < data_len; i++) {
    x_pad_[i + pad_len_] = p_data[i];
  }
  int left = pad_len_;
  int right = pad_len_;
  if (mode_.compare("reflect") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = p_data[left - i];
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + data_len] = p_data[data_len - 2 - i + left];
    }
  }

  if (mode_.compare("symmetric") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = p_data[left - i - 1];
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + data_len] = p_data[data_len - 1 - i + left];
    }
  }

  if (mode_.compare("edge") == 0) {
    for (int i = 0; i < left; ++i) {
      x_pad_[i] = p_data[0];
    }
    for (int i = left; i < left + right; ++i) {
      x_pad_[i + data_len] = p_data[data_len - 1];
    }
  }
}

void MelFeatureExtract::melspectrogram_optimze(short *p_data, int data_len, int8_t *p_dst,
                                               int dst_len, float q_scale, bool fix, float eps,
                                               float s, float alpha, float delta, float r) {
  int pad_len = center_ ? num_fft_ / 2 : 0;

  int n_f = num_fft_ / 2 + 1;
  int padded_len = data_len + 2 * pad_len;
  int n_frames = 1 + (padded_len - num_fft_) / num_hop_;

  Eigen::FFT<float> fft;
  melspec::Vectorf segment(num_fft_);
  melspec::Vectorf specmag(n_f);
  melspec::Vectorf last_state(num_mel_);

  const float scale = 1.0 / 32768.0;
  for (int i = 0; i < n_frames; ++i) {
    int start_idx = i * num_hop_;
    for (int j = 0; j < num_fft_; j++) {
      int srcidx = start_idx + j - pad_len;
      if (srcidx < 0) {
        srcidx = -srcidx;
      } else if (srcidx >= data_len) {
        int over = srcidx - data_len;
        srcidx = data_len - over - 2;
      }
      segment[j] = p_data[srcidx] * scale;  // TODO:fuquan.ke this could be optimized
    }
    melspec::Vectorf x_frame = window_.array() * segment.array();
    melspec::Vectorcf spec_ri = fft.fwd(x_frame);
    // compute mag
    melspec::Vectorf specmag = spec_ri.leftCols(n_f).cwiseAbs().array().pow(2);

    int8_t *pdst_r = p_dst + i * num_mel_;
    melspec::Vectorf rowv = specmag * mel_basis_;

    if (fix) {  // use pcen

      if (i == 0) {
        last_state = rowv;
      } else {
        last_state = (1 - s) * last_state + s * rowv;
      }

      melspec::Vectorf pcen_data =
          (rowv.array() / (last_state.array() + eps).pow(alpha) + delta).pow(r) - pow(delta, r);

      for (int n = 0; n < num_mel_; n++) {
        int16_t qval = pcen_data[n] * q_scale;

        if (qval < -128) {
          qval = -128;
        } else if (qval > 127) {
          qval = 127;
        }
        pdst_r[n] = qval;
      }
    } else {
      for (int n = 0; n < num_mel_; n++) {
        float v = rowv[n];
        if (v < min_val_) v = min_val_;
        if (is_log_) {
          v = 10 * log10f(v);
        }
        int16_t qval = v * q_scale;
        if (qval < -128) {
          // std::cout<<"overflow qval:"<<qval<<std::endl;
          qval = -128;
        } else if (qval > 127) {
          // std::cout<<"overflow qval:"<<qval<<std::endl;
          qval = 127;
        }
        pdst_r[n] = qval;
      }
    }
  }
}

std::map<float, int> MelFeatureExtract::generate_seg_pack_idx(int pack_idx, int pack_len,
                                                              int hop_len, int wav_len) {
  std::map<float, int> pack_seg_map;
  if (pack_idx == -1) {
    return pack_seg_map;
  }
  int pad_len = center_ ? num_fft_ / 2 : 0;
  int padded_len = num_wav_len_ + 2 * pad_len;
  int n_frames = 1 + (padded_len - num_fft_) / num_hop_;
  float packf = pack_len;  // to process the case num_hop less than pack_len
  // std::cout<<"pack:"<<pack_idx<<",";
  for (int i = 0; i < n_frames; ++i) {
    int start_idx = i * num_hop_ - pad_len;
    if (start_idx < 0 || start_idx + num_fft_ >= num_wav_len_) continue;
    float seg_pack_idx = start_idx / packf + pack_idx;

    pack_seg_map[seg_pack_idx] = i;
    // std::cout<<i<<":"<<seg_pack_idx<<",";
  }
  // std::cout<<"\n";
  return pack_seg_map;
}

int MelFeatureExtract::melspectrogram_pack_optimize(short *p_data, int data_len, int pack_len,
                                                    int start_pack_idx, int8_t *p_dst, int dst_len,
                                                    float q_scale, bool fix, float eps, float s,
                                                    float alpha, float delta, float r) {
  if (data_len < num_wav_len_) {
    printf("error,input wavlen:%d,expected wavlen:%d\n", data_len, num_wav_len_);
    return -1;
  }
  if (last_pack_len_ != -1 && last_pack_len_ != pack_len) {
    printf("error,packlen not equal,last:%d,current:%d\n", last_pack_len_, pack_len);
    return -1;
  }
  if (pack_len % num_hop_ != 0) {
    printf("error,packlen(%d) mod hoplen(%d) !=0\n", pack_len, num_hop_);
    return -1;
  }

  std::map<float, int> last_seg_pack =
      generate_seg_pack_idx(last_pack_idx_, pack_len, num_hop_, num_wav_len_);
  std::map<float, int> cur_seg_pack =
      generate_seg_pack_idx(start_pack_idx, pack_len, num_hop_, num_wav_len_);

  std::map<int, int> seg_map;
  for (auto iter = cur_seg_pack.begin(); iter != cur_seg_pack.end(); ++iter) {
    float pack_idx = iter->first;
    int seg_idx = iter->second;
    if (last_seg_pack.count(pack_idx)) {
      seg_map[seg_idx] = last_seg_pack[pack_idx];
    }
  }
  int num_copy_frame = 0;
  // copy segment feature from previous frame
  for (auto iter = seg_map.begin(); iter != seg_map.end(); ++iter) {
    int cur_seg = iter->first;
    int prev_seg = iter->second;
    if (cur_seg > prev_seg) {
      printf("error, current seg:%d,prev seg:%d\n", cur_seg, prev_seg);
      return -1;
    }

    if (fix) {  // update sft mag vec
      float *p_cur_mag = mp_sft_mag_vec_ + cur_seg * num_mel_;
      float *p_prev_mag = mp_sft_mag_vec_ + prev_seg * num_mel_;
      memcpy(p_cur_mag, p_prev_mag, num_mel_ * sizeof(p_prev_mag[0]));
    } else {
      int8_t *p_cur_seg = p_dst + cur_seg * num_mel_;
      int8_t *p_prev_seg = p_dst + prev_seg * num_mel_;
      memcpy(p_cur_seg, p_prev_seg, num_mel_ * sizeof(p_dst[0]));
      num_copy_frame += 1;
    }
  }

  int pad_len = center_ ? num_fft_ / 2 : 0;
  int n_f = num_fft_ / 2 + 1;
  int padded_len = num_wav_len_ + 2 * pad_len;
  int n_frames = 1 + (padded_len - num_fft_) / num_hop_;
  if (fix && mp_sft_mag_vec_ == nullptr) {
    mp_sft_mag_vec_ = new float[n_frames * num_mel_];
  }

  melspec::Vectorf segment(num_fft_);
  melspec::Vectorf specmag(n_f);
  const float scale = 1.0 / 32768.0;
  melspec::Vectorf last_state(num_mel_);
  float pcen_bias = pow(delta, r);

  int num_skip = 0;
  for (int i = 0; i < n_frames; ++i) {
    int8_t *pdst_r = p_dst + i * num_mel_;
    if (seg_map.count(i)) {
      if (fix) {
        Eigen::Map<Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor>> rowv(
            mp_sft_mag_vec_ + i * num_mel_, 1, num_mel_);
        last_state = (1 - s) * last_state + s * rowv;
        melspec::Vectorf pcen_data =
            (rowv.array() / (last_state.array() + eps).pow(alpha) + delta).pow(r) - pcen_bias;
        quant_feat(pcen_data.data(), q_scale, num_mel_, 0, pdst_r);
      }

      num_skip += 1;
      continue;
    }
    int start_idx = i * num_hop_;
    for (int j = 0; j < num_fft_; j++) {
      int srcidx = start_idx + j - pad_len;
      if (srcidx < 0) {
        srcidx = -srcidx;
      } else if (srcidx >= num_wav_len_) {
        int over = srcidx - num_wav_len_;
        srcidx = num_wav_len_ - over - 2;
      }
      segment[j] = p_data[srcidx] * scale;  // TODO:fuquan.ke this could be optimized
    }

    melspec::Vectorf x_frame = window_.array() * segment.array();
    melspec::Vectorcf spec_ri = fft_.fwd(x_frame);
    // compute mag
    melspec::Vectorf specmag = spec_ri.leftCols(n_f).cwiseAbs().array().pow(2);

    if (fix) {
      Eigen::Map<Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor>> rowv(
          mp_sft_mag_vec_ + i * num_mel_, 1, num_mel_);
      rowv = specmag * mel_basis_;
      // memcpy(mp_sft_mag_vec_ + i * num_mel_, rowv.data(), sizeof(mp_sft_mag_vec_[0]) * num_mel_);
      if (i == 0) {
        last_state = rowv;
      } else {
        last_state = (1 - s) * last_state + s * rowv;
      }

      melspec::Vectorf pcen_data =
          (rowv.array() / (last_state.array() + eps).pow(alpha) + delta).pow(r) - pcen_bias;

      quant_feat(pcen_data.data(), q_scale, num_mel_, 0, pdst_r);
    } else {
      melspec::Vectorf rowv = specmag * mel_basis_;
      quant_feat(rowv.data(), q_scale, num_mel_, min_val_, pdst_r);
    }
  }
  last_pack_idx_ = start_pack_idx;
  last_pack_len_ = pack_len;

  return 0;
}