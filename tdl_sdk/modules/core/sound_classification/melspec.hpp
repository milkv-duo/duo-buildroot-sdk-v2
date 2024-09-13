
#include <map>
#include <string>
#include <vector>
#include "Eigen/Core"
#include "unsupported/Eigen/FFT"
namespace melspec {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif  // !M_PI

typedef Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor> Vectorf;
typedef Eigen::Matrix<std::complex<float>, 1, Eigen::Dynamic, Eigen::RowMajor> Vectorcf;
typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> Matrixf;
typedef Eigen::Matrix<std::complex<float>, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
    Matrixcf;
class MelFeatureExtract {
 public:
  MelFeatureExtract(int num_frames, int sr, int n_fft, int n_hop, int n_mel, int fmin, int fmax,
                    const std::string &mode, bool htk, bool center = true, int power = 2,
                    bool is_log = true);
  ~MelFeatureExtract();

  void update_data(short *p_data, int data_len);
  void update_float_data(float *p_data, int data_len);
  void melspectrogram_impl(int8_t *p_dst, int dst_len, float q_scale);
  Matrixf melspectrogram(std::vector<float> &wav);
  void melspectrogram_optimze(short *p_data, int data_len, int8_t *p_dst, int dst_len,
                              float q_scale, bool fixed = false, float eps = 1E-6, float s = 0.025,
                              float alpha = 0.98, float delta = 2, float r = 0.5);
  void pad(Vectorf &x, int left, int right, const std::string &mode, float value);

  /**
   * @brief generate pack index for each segment of this frame
   * @param pack_idx start pack index of this frame
   * @return std::map<float,int>
   */
  std::map<float, int> generate_seg_pack_idx(int pack_idx, int pack_len, int hop_len, int wav_len);

  /**
   * @brief opitmize implementation for pack version
   * @param pack_len the input data(p_data) is combined with many packs,each with size equal
   * pack_len
   * @param start_pack_idx the pack index of the first pack of the input data
   * @param q_scale quant scale of the input tensor of the KWS model
   */
  int melspectrogram_pack_optimize(short *p_data, int data_len, int pack_len, int start_pack_idx,
                                   int8_t *p_dst, int dst_len, float q_scale, bool fixed = false,
                                   float eps = 1E-6, float s = 0.025, float alpha = 0.98,
                                   float delta = 2, float r = 0.5);

 private:
  // float *mp_buffer;
  Matrixf mel_basis_;
  Vectorf x_pad_;
  Vectorf window_;

  int num_wav_len_;  // data length used to extract mel feature
  int last_pack_idx_ = -1;
  int last_pack_len_ = -1;
  float *mp_sft_mag_vec_ = nullptr;  // num_frame x num_mel_
  Eigen::FFT<float> fft_;

  int num_fft_;
  int win_len_;
  int num_hop_;
  int num_mel_;
  int sample_rate_;
  int fmin_;
  int fmax_;
  bool center_ = true;
  int power_ = 2;
  std::string mode_;
  int pad_len_ = 0;
  const float min_val_ = 1.0e-6;
  bool is_log_;
};

}  // namespace melspec
