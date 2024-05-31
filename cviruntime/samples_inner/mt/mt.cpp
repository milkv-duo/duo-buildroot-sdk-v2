#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "cnpy.h"
#include "mt_model.hpp"

/*
static void load_input(int16_t *seq) {
  memset(seq, 0, INFER_FIX_LEN * sizeof(seq));
  seq[0] = 4;
  seq[1] = 3201;
  seq[2] = 6;
  seq[3] = 150;
  seq[4] = 1121;
  seq[5] = 2;

  printf("src_seq:\n");
  for (int i = 0; i < INFER_FIX_LEN; i++) {
    printf("%d ", seq[i]);
  }
  printf("\n");
}
*/

static int levenshtein_distance(const uint16_t* s, int n, const uint16_t* t, int m) {
   ++n; ++m;
   int* d = new int[n * m];
   memset(d, 0, sizeof(int) * n * m);
   for (int i = 1, im = 0; i < m; ++i, ++im) {
      for (int j = 1, jn = 0; j < n; ++j, ++jn) {
         if (s[jn] == t[im]) {
            d[(i * n) + j] = d[((i - 1) * n) + (j - 1)];
         } else {
            d[(i * n) + j] = std::min(d[(i - 1) * n + j] + 1, /* A deletion. */
                                 std::min(d[i * n + (j - 1)] + 1, /* An insertion. */
                                     d[(i - 1) * n + (j - 1)] + 1)); /* A substitution. */
         }
      }
   }
   int r = d[n * m - 1];
   delete [] d;
   return r;
}

int main(int argc, char **argv) {
  int ret = 0;
  if (argc < 3) {
    printf("Usage:\n");
    printf("   %s mt-cvimodel, input_npz, ref_npz\n", argv[0]);
    exit(1);
  }

  MTrans trans(argv[1]);
  int16_t src_seq[INFER_FIX_LEN];
  //load_input(argv[2], src_seq);

  cnpy::npz_t seq_npz = cnpy::npz_load(argv[2]);
  if (seq_npz.size() == 0) {
    printf("Failed to load input npz\n");
  }
  cnpy::npz_t ref_npz = cnpy::npz_load(argv[3]);
  if (ref_npz.size() == 0) {
    printf("Failed to load ref npz\n");
  }

  int err_sum = 0;
  for (int i = 0; i < seq_npz.size(); i++) {
    auto name = std::to_string(i);
    auto &input_data = seq_npz[name];
    auto in_ptr = input_data.data<uint16_t>();
    memcpy(src_seq, in_ptr, sizeof(uint16_t) * INFER_FIX_LEN);
    printf("src_seq: ");
    for (int i = 0; i < INFER_FIX_LEN; i++) {
      printf("%d ", src_seq[i]);
    }
    printf("\n");


    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    int16_t gen_seq[INFER_FIX_LEN];
    trans.run(src_seq, INFER_FIX_LEN, gen_seq, INFER_FIX_LEN);

    gettimeofday(&t1, NULL);
    long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

    printf("gen_seq: ");
    for (int i = 0; i < INFER_FIX_LEN; i++) {
      printf("%d ", gen_seq[i]);
    }
    printf("\n");

    auto& data = ref_npz[name];
    auto ptr = data.data<uint16_t>();
    printf("ref_seq: ");
    for (int i = 0; i < INFER_FIX_LEN; i++) {
      printf("%d ", ptr[i]);
    }
    printf("\n");

    int err = levenshtein_distance((uint16_t *)gen_seq, 40, ptr, 40);
    // int err = 0;
    // for (int i = 0; i < INFER_FIX_LEN; i++) {
    //   if (gen_seq[i] != ptr[i]) {
    //     err += 1;
    //   }
    // }
    err_sum += err;
    printf("%d => error:%d, sum:%d, performance: %fms\n", i, err, err_sum, elapsed / 1000.0);
  }

  printf("accuracy: %f%\n", 100 - (err_sum * 100.0f / (seq_npz.size() * 40)));
  return 0;
}