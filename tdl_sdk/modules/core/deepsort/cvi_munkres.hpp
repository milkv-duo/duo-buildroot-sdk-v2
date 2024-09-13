#pragma once

#include <Eigen/Eigen>

enum NODE_STATE { NONE = 0, STAR, PRIME };

enum ALGO_STAGE { ZERO = 0, ONE, TWO, THREE, FOUR, FINAL, DONE };

enum MunkresReturn {
  MUNKRES_SUCCESS,
  MUNKRES_FAILURE,
};

class CVIMunkres {
 public:
  CVIMunkres(Eigen::MatrixXf *matrix);
  ~CVIMunkres();

  MunkresReturn solve();
  void show_result();
  // std::vector<std::pair<int, int>> compute(cv::Mat &cost_matrix);
  int *m_match_result;

 private:
  // variables
  int m_rows, m_cols, m_size;
  bool *m_covered_R, *m_covered_C;
  Eigen::MatrixXf *m_original_matrix;
  Eigen::MatrixXf m_cost_matrix;
  int **m_state_matrix;
  int m_prime_index[2];

  void substract_min_by_row();
  void substract_min_by_col();

  int stage_0();
  int stage_1();
  int stage_2();
  int stage_3();
  int stage_4();
  int stage_final();
  bool find_uncovered_zero(int &r, int &c);
  bool find_star_by_row(const int &row, int &c);
  bool find_star_by_col(const int &col, int &r);
  bool find_prime_by_row(const int &row, int &c);
  float find_uncovered_min();

  void show_state_matrix();

  // bool find_prime_by_col(int row, int &c);
};
