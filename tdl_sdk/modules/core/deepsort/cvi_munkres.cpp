/*
 * reference:
 *     https://www.feiyilin.com/munkres.html
 */

#include "cvi_munkres.hpp"
#include "cvi_tdl_log.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>

#define DEBUG 0
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define PADDING_VALUE 0.0

CVIMunkres::CVIMunkres(Eigen::MatrixXf *matrix) {
  m_original_matrix = matrix;
  m_rows = matrix->rows();
  m_cols = matrix->cols();
  m_size = MAX(m_rows, m_cols);

  m_covered_C = new bool[m_size];
  m_covered_R = new bool[m_size];
  m_prime_index[0] = -1;
  m_prime_index[1] = -1;
  m_match_result = new int[m_rows];

  /* Initialization */
  m_state_matrix = new int *[m_size];
  m_cost_matrix = Eigen::MatrixXf(m_size, m_size);

  for (int i = 0; i < m_size; i++) {
    m_state_matrix[i] = new int[m_size];
    for (int j = 0; j < m_size; j++) {
      m_state_matrix[i][j] = NODE_STATE::NONE;
      if (i < m_rows && j < m_cols) {
        m_cost_matrix(i, j) = (*matrix)(i, j);
      } else {
        m_cost_matrix(i, j) = PADDING_VALUE;
      }
    }

    m_covered_R[i] = false;
    m_covered_C[i] = false;
  }

#if DEBUG
  std::cout << "CVIMunkres::cost_matrix" << std::endl;
  std::cout << m_cost_matrix << std::endl;
  std::cout << "CVIMunkres::original_matrix" << std::endl;
  for (int i = 0; i < m_rows; i++) {
    for (int j = 0; j < m_cols; j++) {
      std::cout << m_original_matrix[i][j] << ", ";
    }
    std::cout << std::endl;
  }
#endif /* DEBUG */
}

CVIMunkres::~CVIMunkres() {
  delete[] m_covered_C;
  delete[] m_covered_R;

  for (int i = 0; i < m_size; i++) {
    delete[] m_state_matrix[i];
  }
  delete[] m_state_matrix;

  delete[] m_match_result;
}

MunkresReturn CVIMunkres::solve() {
  try {
    int stage = ALGO_STAGE::ZERO;

    int stage_counter = 0;

    while (true) {
      stage_counter += 1;
      if (stage_counter > 10000) {
        throw std::runtime_error("stage_count exceeds 10000 iterations");
      }

      switch (stage) {
        case ALGO_STAGE::ZERO:
          stage = stage_0();
          break;
        case ALGO_STAGE::ONE:
          stage = stage_1();
          break;
        case ALGO_STAGE::TWO:
          stage = stage_2();
          break;
        case ALGO_STAGE::THREE:
          stage = stage_3();
          break;
        case ALGO_STAGE::FOUR:
          stage = stage_4();
          break;
        case ALGO_STAGE::FINAL:
          stage = stage_final();
          break;
        case ALGO_STAGE::DONE:
          stage = stage_final();
          return MUNKRES_SUCCESS;
        default:
          assert(0);
      }
    }
  } catch (std::runtime_error &e) {
    LOGE("[Munkres] error occurs: %s\n", e.what());
    return MUNKRES_FAILURE;
  }

  return MUNKRES_SUCCESS;
}

int CVIMunkres::stage_0() {
#if DEBUG
  std::cout << "stage 0" << std::endl;
#endif /* DEBUG */
  if (m_rows > m_cols) {
    substract_min_by_col();
  } else if (m_rows < m_cols) {
    substract_min_by_row();
  } else { /* m_rows == m_cols */
    substract_min_by_col();
    substract_min_by_row();
  }
#if DEBUG
  std::cout << "cost_matrix" << std::endl;
  std::cout << m_cost_matrix << std::endl;
#endif /* DEBUG */

  for (int i = 0; i < m_size; i++) {
    for (int j = 0; j < m_size; j++) {
      if (m_cost_matrix(i, j) == 0 && !m_covered_R[i] && !m_covered_C[j]) {
        m_state_matrix[i][j] = NODE_STATE::STAR;
        m_covered_R[i] = true;
        m_covered_C[j] = true;
        break;
      }
    }
  }

  return ALGO_STAGE::ONE;
}

int CVIMunkres::stage_1() {
#if DEBUG
  std::cout << "stage 1" << std::endl;
#endif /* DEBUG */
  for (int i = 0; i < m_size; i++) {
    m_covered_R[i] = false;
    m_covered_C[i] = false;
  }

  int star_counter = 0;
  for (int i = 0; i < m_size; i++) {
    for (int j = 0; j < m_size; j++) {
      if (m_state_matrix[i][j] == NODE_STATE::PRIME) {
        m_state_matrix[i][j] = NODE_STATE::NONE;
      } else if (m_state_matrix[i][j] == NODE_STATE::STAR) {
        star_counter += 1;
        m_covered_C[j] = true;
      }
    }
  }

  assert(star_counter <= m_size);
  if (star_counter == m_size) {
    return ALGO_STAGE::FINAL;
  } else {
    return ALGO_STAGE::TWO;
  }
}

int CVIMunkres::stage_2() {
#if DEBUG
  std::cout << "stage 2" << std::endl;
  show_state_matrix();
#endif /* DEBUG */
  int _counter = 0;
  while (true) {
    int i, j;
    if (!find_uncovered_zero(i, j)) {
      return ALGO_STAGE::FOUR;
    }

    m_state_matrix[i][j] = NODE_STATE::PRIME;
#if DEBUG
    show_state_matrix();
#endif /* DEBUG */
    if (find_star_by_row(i, j)) {
      m_covered_R[i] = true;
      m_covered_C[j] = false;
    } else {
      m_prime_index[0] = i;
      m_prime_index[1] = j;
      return ALGO_STAGE::THREE;
    }

    _counter += 1;
    if (_counter > 100) {
      throw std::runtime_error("too many times in stage2");
    }
  }
}

int CVIMunkres::stage_3() {
#if DEBUG
  std::cout << "stage 3" << std::endl;
  show_state_matrix();
#endif /* DEBUG */
  int **node_sequence = new int *[2 * m_size + 1];
  node_sequence[0] = new int[2];
  node_sequence[0][0] = m_prime_index[0];
  node_sequence[0][1] = m_prime_index[1];
  int node_counter = 1;

  int r = -1, c = m_prime_index[1];
#if DEBUG
  std::cout << r << ", " << c << std::endl;
#endif /* DEBUG */
  while (find_star_by_col(c, r)) {
    node_sequence[node_counter] = new int[2];
    node_sequence[node_counter][0] = r;
    node_sequence[node_counter][1] = c;
    node_counter += 1;

    if (!find_prime_by_row(r, c)) {
      throw std::runtime_error("Prime Node not found");
    }

    node_sequence[node_counter] = new int[2];
    node_sequence[node_counter][0] = r;
    node_sequence[node_counter][1] = c;
    node_counter += 1;
  }

  for (int i = 0; i < node_counter; i++) {
    r = node_sequence[i][0];
    c = node_sequence[i][1];
#if DEBUG
    std::cout << "(" << i << ") [" << std::setw(2) << r << "," << std::setw(2) << c << "]\n";
#endif /* DEBUG */
    if (i % 2 == 0) {
      assert(m_state_matrix[r][c] == NODE_STATE::PRIME);
      m_state_matrix[r][c] = NODE_STATE::STAR;
    } else {
      assert(m_state_matrix[r][c] == NODE_STATE::STAR);
      m_state_matrix[r][c] = NODE_STATE::NONE;
    }

    delete[] node_sequence[i];
  }

  delete[] node_sequence;

#if DEBUG
  show_state_matrix();
#endif /* DEBUG */
  return ALGO_STAGE::ONE;
}

int CVIMunkres::stage_4() {
#if DEBUG
  std::cout << "stage 4" << std::endl;
  show_state_matrix();
  std::cout << "cost matrix (before)" << std::endl;
  std::cout << m_cost_matrix << std::endl;
#endif /* DEBUG */
  float min = find_uncovered_min();

  for (int i = 0; i < m_size; i++) {
    for (int j = 0; j < m_size; j++) {
      if (m_covered_R[i] && m_covered_C[j]) {
        m_cost_matrix(i, j) += min;
      } else if (!m_covered_R[i] && !m_covered_C[j]) {
        m_cost_matrix(i, j) -= min;
      }
    }
  }

#if DEBUG
  std::cout << "cost matrix (after)" << std::endl;
  std::cout << m_cost_matrix << std::endl;
#endif /* DEBUG */

  return ALGO_STAGE::TWO;
}

int CVIMunkres::stage_final() {
#if DEBUG
  std::cout << "stage final\n";
  show_state_matrix();
  std::cout << "cost matrix\n";
  std::cout << m_cost_matrix << std::endl;
#endif /* DEBUG */

  for (int i = 0; i < m_rows; i++) {
    bool is_match = false;
    for (int j = 0; j < m_cols; j++) {
      if (m_state_matrix[i][j] == NODE_STATE::STAR) {
        m_match_result[i] = j;
        is_match = true;
        break;
      }
    }
    if (!is_match) {
      m_match_result[i] = -1;
    }
  }

  return ALGO_STAGE::DONE;
}

bool CVIMunkres::find_uncovered_zero(int &r, int &c) {
  for (int i = 0; i < m_size; i++) {
    if (m_covered_R[i]) {
      continue;
    }
    for (int j = 0; j < m_size; j++) {
      if (m_covered_C[j]) {
        continue;
      }
      if (m_cost_matrix(i, j) == 0) {
        r = i;
        c = j;
        return true;
      }
    }
  }
  return false;
}

float CVIMunkres::find_uncovered_min() {
  float min_value = __FLT_MAX__;
  for (int i = 0; i < m_size; i++) {
    if (m_covered_R[i]) {
      continue;
    }
    for (int j = 0; j < m_size; j++) {
      if (m_covered_C[j]) {
        continue;
      }
      if (m_cost_matrix(i, j) < min_value) {
        min_value = m_cost_matrix(i, j);
      }
    }
  }
#if DEBUG
  std::cout << "find uncovered min = " << min_value << std::endl;
#endif /* DEBUG */
  return min_value;
}

bool CVIMunkres::find_star_by_row(const int &row, int &c) {
  for (int j = 0; j < m_size; j++) {
    if (m_state_matrix[row][j] == NODE_STATE::STAR) {
      c = j;
      return true;
    }
  }
  return false;
}

bool CVIMunkres::find_prime_by_row(const int &row, int &c) {
  for (int j = 0; j < m_size; j++) {
    if (m_state_matrix[row][j] == NODE_STATE::PRIME) {
      c = j;
      return true;
    }
  }
  return false;
}

bool CVIMunkres::find_star_by_col(const int &col, int &r) {
  for (int i = 0; i < m_size; i++) {
    if (m_state_matrix[i][col] == NODE_STATE::STAR) {
      r = i;
      return true;
    }
  }
  return false;
}

// bool CVIMunkres::find_prime_by_col(int row, int &c){
//   /* TODO */
//   return false;
// }

void CVIMunkres::substract_min_by_row() {
  for (int i = 0; i < m_size; i++) {
    float min_value = m_cost_matrix(i, 0);
    for (int j = 1; j < m_size; j++) {
      if (min_value > m_cost_matrix(i, j)) {
        min_value = m_cost_matrix(i, j);
      }
    }

    for (int j = 0; j < m_size; j++) {
      m_cost_matrix(i, j) -= min_value;
    }
  }
}

void CVIMunkres::substract_min_by_col() {
  for (int j = 0; j < m_size; j++) {
    float min_value = m_cost_matrix(0, j);
    for (int i = 1; i < m_size; i++) {
      if (min_value > m_cost_matrix(i, j)) {
        min_value = m_cost_matrix(i, j);
      }
    }
    for (int i = 0; i < m_size; i++) {
      m_cost_matrix(i, j) -= min_value;
    }
  }
}

void CVIMunkres::show_state_matrix() {
  std::cout << "==============================================\n";
  std::cout << std::setw(8) << "STATE_M";
  for (int j = 0; j < m_size; j++) {
    std::cout << std::setw(8) << m_covered_C[j];
  }
  std::cout << std::endl;
  for (int i = 0; i < m_size; i++) {
    std::cout << std::setw(8) << m_covered_R[i];
    for (int j = 0; j < m_size; j++) {
      std::cout << std::setw(8) << m_state_matrix[i][j];
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}

void CVIMunkres::show_result() {
  float total_cost = 0.0;
  for (int i = 0; i < m_rows; i++) {
    if (m_match_result[i] != -1) {
      int j = m_match_result[i];
      std::cout << "(" << std::setw(2) << i << "," << std::setw(2) << j << ") -> " << std::setw(8)
                << std::setprecision(4) << (*m_original_matrix)(i, j) << std::endl;
      total_cost += (*m_original_matrix)(i, j);
    }
  }

  std::cout << "total cost = " << std::setw(10) << std::setprecision(4) << total_cost << std::endl;
}