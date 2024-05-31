#ifndef RUNTIME_SIMILARITY_H
#define RUNTIME_SIMILARITY_H

#include <stddef.h>
#include <math.h>
#include <vector>
#include <iostream>

static float u16_to_bf16(uint16_t val) {
  float ret;
  auto *q = reinterpret_cast<uint16_t *>(&ret);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  q[0] = val;
#else
  q[1] = val;
#endif
  return ret;
}

template <typename U, typename V>
static bool array_convert(U *u, V *v, std::vector<float> &uu, std::vector<float> &vv) {
  size_t equal_cnt = 0;
  for (size_t i = 0; i < uu.size(); i++) {
    uu[i] = (typeid(U) == typeid(uint16_t)) ? u16_to_bf16(u[i]) : static_cast<float>(u[i]);
    vv[i] = (typeid(V) == typeid(uint16_t)) ? u16_to_bf16(v[i]) : static_cast<float>(v[i]);
    if (uu[i] == vv[i])
      equal_cnt++;
  }
  return equal_cnt == uu.size();
}

static float array_average(float *u, float *v, size_t size) {
  double average = 0;
  for (size_t i = 0; i < size; i++) {
    average += u[i] * v[i];
  }
  return average / size;
}

static float array_average(float *u, size_t size, int power = 1) {
  double average = 0;
  for (size_t i = 0; i < size; i++) {
    if (power != 1) {
      average += pow(u[i], power);
    } else {
      average += u[i];
    }
  }
  return average / size;
}

static float euclidean_similiarity(float *u, float *v, size_t size) {
  double distance = 0;
  double root = 0;
  for (size_t i = 0; i < size; i++) {
    distance += pow(u[i] - v[i], 2);
    root += pow((u[i] + v[i]) / 2, 2);
  }
  distance = sqrt(distance);
  root = sqrt(root);
  return (float)(1 - distance / root);
}

static float correlation_similarity(float *u, float *v, size_t size, bool centered) {
  if (centered) {
    float umu = array_average(u, size);
    float vmu = array_average(v, size);
    for (size_t i = 0; i < size; i++) {
      u[i] -= umu;
      v[i] -= vmu;
    }
  }

  float uv = array_average(u, v, size);
  float uu = array_average(u, size, 2);
  float vv = array_average(v, size, 2);
  return uv / sqrt(uu * vv);
}

template <typename U, typename V>
static void array_similarity(U *u, V *v, size_t size, float &euclidean, float &cosine,
                             float &correlation) {
  std::vector<float> uu(size, 0);
  std::vector<float> vv(size, 0);
  if (array_convert(u, v, uu, vv)) {
    euclidean = 1;
    cosine = 1;
    correlation = 1;
    return;
  }
  euclidean = euclidean_similiarity(uu.data(), vv.data(), uu.size());
  cosine = correlation_similarity(uu.data(), vv.data(), uu.size(), false);
  correlation = correlation_similarity(uu.data(), vv.data(), uu.size(), true);
}

#endif