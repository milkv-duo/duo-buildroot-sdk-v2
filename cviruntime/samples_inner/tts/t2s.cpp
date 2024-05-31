#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "cnpy.h"
#include "t2s_model.hpp"

using namespace t2s;

static void load_input(const char *input_npz, uint16_t *text, uint16_t *lang, uint16_t *speaker, int32_t &text_sz) {
  cnpy::npz_t npz = cnpy::npz_load(input_npz);
  if (npz.size() == 0) {
    printf("Failed to load images npz\n");
  }
  memcpy(text, npz["text"].data<uint16_t>(), 200 * sizeof(uint16_t));
  memcpy(lang, npz["lang"].data<uint16_t>(), 200 * sizeof(uint16_t));
  memcpy(speaker, npz["speaker"].data<uint16_t>(), 200 * sizeof(uint16_t));
  text_sz = npz["text_len"].data<int32_t>()[0];
}

static void saveToNpz(const std::string &file, float *data, size_t size) {
  cnpy::npz_t npz;
  std::vector<size_t> shape = {1, size};
  cnpy::npz_add_array<float>(npz, "mel_out", data, shape);
  cnpy::npz_save_all(file, npz);
}

int main(int argc, char **argv) {
  int ret = 0;
  if (argc < 4) {
    printf("Usage:\n");
    printf("   %s enc-cvimodel, dec-cvimodel input_npz\n", argv[0]);
    exit(1);
  }
  uint16_t text[200];
  uint16_t lang[200];
  uint16_t speaker[200];
  int32_t text_sz;
  load_input(argv[3], text, lang, speaker, text_sz);
  printf("load input\n");

  T2SModel tts(argv[1], argv[2]);

  struct timeval t0, t1;
  gettimeofday(&t0, NULL);

  int32_t duration = 0;
  auto mel_out = tts.run(text, text_sz, lang, speaker, duration);

  gettimeofday(&t1, NULL);
  long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  printf("Performance: %f ms\n", elapsed/1000.0);

  printf("duration:%d\n", duration);
  printf("dump mel_out.npz\n");
  saveToNpz("t2s_mel_out.npz", mel_out,  80 * MAX_DECODE_LEN);

  return 0;
}