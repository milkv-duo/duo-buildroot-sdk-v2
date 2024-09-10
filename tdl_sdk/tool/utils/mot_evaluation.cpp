#include "mot_evaluation.hpp"
#include <inttypes.h>
#include <cmath>
#include "sys/stat.h"
#include "sys/types.h"

#define DEFAULT_DATA_INFO_NAME "MOT_data_info.txt"

MOT_Evaluation::MOT_Evaluation() {}

MOT_Evaluation::~MOT_Evaluation() {}

CVI_S32 MOT_Evaluation::update(const cvtdl_tracker_t &trackers,
                               const cvtdl_tracker_t &inact_trackers) {
  time_counter += 1;
  bbox_counter += trackers.size;
  std::set<uint64_t> check_list = alive_stable_id;
  for (uint32_t i = 0; i < trackers.size; i++) {
    if (trackers.info[i].state == CVI_TRACKER_NEW) {
      new_id_counter += 1;
      continue;
    }
    if (trackers.info[i].state == CVI_TRACKER_UNSTABLE) {
      continue;
    }
    stable_tracking_counter += 1;
    uint64_t t_id = trackers.info[i].id;
    if (alive_stable_id.find(t_id) == alive_stable_id.end()) {
      alive_stable_id.insert(t_id);
      stable_id.insert(t_id);
      tracking_performance[t_id] = std::pair<uint32_t, uint32_t>(1, time_counter);
    } else {
      tracking_performance[t_id].first += 1;
      check_list.erase(t_id);
    }
  }
  std::set<uint64_t> inact_id;
  for (uint32_t i = 0; i < inact_trackers.size; i++) {
    inact_id.insert(inact_trackers.info[i].id);
  }
  for (std::set<uint64_t>::iterator it = check_list.begin(); it != check_list.end(); it++) {
    if (inact_id.find(*it) == inact_id.end()) {
      alive_stable_id.erase(*it);
      tracking_performance[*it].second = time_counter - tracking_performance[*it].second;
      entropy += -log2((double)tracking_performance[*it].first / tracking_performance[*it].second);
    }
  }

  return CVI_SUCCESS;
}

void MOT_Evaluation::summary(MOT_Performance_t &performance) {
  double total_entropy = entropy;
  for (const auto &id : alive_stable_id) {
    total_entropy += -log2((double)tracking_performance[id].first /
                           (time_counter - tracking_performance[id].second + 1));
  }
  double coverage_rate = (double)stable_tracking_counter / (double)bbox_counter;

  performance.stable_id_num = (uint32_t)stable_id.size();
  performance.total_entropy = total_entropy;
  performance.coverage_rate = coverage_rate;
  performance.score = (1. - coverage_rate) * total_entropy;

#if 0
  printf("\n@@@ MOT Evaluation Summary @@@\n");
  printf("Stable ID Num : %u\n", (uint32_t)stable_id.size());
  printf("Total Entropy : %.4lf\n", total_entropy);
  printf("Coverage Rate : %.4lf ( %u / %u )\n", coverage_rate, stable_tracking_counter,
         bbox_counter);
  printf("Score : %.4lf\n", performance.score);
#endif
}

GridIndexGenerator::GridIndexGenerator(const std::vector<uint32_t> &r) {
  ranges = r;
  idx.resize(r.size());
  std::fill(idx.begin(), idx.end(), 0);
}

GridIndexGenerator::~GridIndexGenerator() {}

CVI_S32 GridIndexGenerator::next(std::vector<uint32_t> &next_idx) {
  if (counter == 0) {
    next_idx = idx;
    counter += 1;
    return CVI_SUCCESS;
  }
  bool carry = true;
  for (size_t i = 0; i < ranges.size(); i++) {
    if (idx[i] + 1 == ranges[i]) {
      idx[i] = 0;
    } else {
      idx[i] += 1;
      carry = false;
    }
    if (!carry) {
      next_idx = idx;
      counter += 1;
      return CVI_SUCCESS;
    }
  }
  return CVI_FAILURE;
}

/********************
 * helper functions *
 ********************/
#define B_SIZE_BBOX 32
#define B_SIZE_FEATURE 64
static void FILL_BBOX(cvtdl_bbox_t *bbox, char text_buffer[][B_SIZE_BBOX]);
static void FILL_FEATURE(cvtdl_feature_t *feature, char text_buffer[][B_SIZE_FEATURE],
                         char *data_path);

CVI_S32 RUN_MOT_EVALUATION(cvitdl_handle_t tdl_handle, const MOT_EVALUATION_ARGS_t &args,
                           MOT_Performance_t &performance, bool output_result, char *result_path) {
  char text_buffer[256];
  char text_buffer_tmp[256];
  char inFile_data_path[256];
  sprintf(inFile_data_path, "%s/%s", args.mot_data_path, DEFAULT_DATA_INFO_NAME);
  FILE *inFile_data = fopen(inFile_data_path, "r");
  if (inFile_data == NULL) {
    printf("fail to open file: %s\n", inFile_data_path);
    return CVI_FAILURE;
  }

  FILE *outFile_result = NULL;
  if (output_result) {
    struct stat fst;
    if (result_path == NULL || stat(result_path, &fst) != 0) {
      outFile_result = fopen(result_path, "w");
      if (outFile_result == NULL) {
        printf("There is a problem opening the output file: %s.\n", result_path);
        return CVI_FAILURE;
      }
    } else {
      printf("output result file exist. %s\n", result_path);
      return CVI_FAILURE;
    }
  }

  fscanf(inFile_data, "%s %s", text_buffer, text_buffer_tmp);
  int frame_num = atoi(text_buffer);
#if 1
  bool output_features = atoi(text_buffer_tmp) == 1;
  printf("Frame Num: %d ( Output Features: %s )\n", frame_num, output_features ? "true" : "false");
#endif

  if (output_result) {
    fprintf(outFile_result, "%d\n", frame_num);
  }

  cvtdl_object_t obj_meta;
  cvtdl_face_t face_meta;
  cvtdl_tracker_t tracker_meta;
  memset(&obj_meta, 0, sizeof(cvtdl_object_t));
  memset(&face_meta, 0, sizeof(cvtdl_face_t));
  memset(&tracker_meta, 0, sizeof(cvtdl_tracker_t));

  MOT_Evaluation mot_eval_data;

  for (int counter = 1; counter <= frame_num; counter++) {
    // printf("[%d/%d]\n", counter, frame_num);
    fscanf(inFile_data, "%s", text_buffer);
    uint32_t bbox_num = (uint32_t)atoi(text_buffer);

    switch (args.target_type) {
      case PERSON: {
        obj_meta.size = bbox_num;
        obj_meta.info = (cvtdl_object_info_t *)malloc(obj_meta.size * sizeof(cvtdl_object_info_t));
        memset(obj_meta.info, 0, obj_meta.size * sizeof(cvtdl_object_info_t));
        for (uint32_t i = 0; i < bbox_num; i++) {
          // printf("[%u/%u]\n", i, bbox_num);
          char text_buffer_bbox[5][B_SIZE_BBOX];
          /* read bbox info */
          fscanf(inFile_data, "%s %s %s %s %s", text_buffer_bbox[0], text_buffer_bbox[1],
                 text_buffer_bbox[2], text_buffer_bbox[3], text_buffer_bbox[4]);
          obj_meta.info[i].classes = atoi(text_buffer_bbox[0]);
          FILL_BBOX(&obj_meta.info[i].bbox, text_buffer_bbox);
          /* read feature info */
          char text_buffer_feature[2][B_SIZE_FEATURE];  // size, bin data path
          fscanf(inFile_data, "%s %s", text_buffer_feature[0], text_buffer_feature[1]);
          FILL_FEATURE(&obj_meta.info[i].feature, text_buffer_feature, args.mot_data_path);
#if 0
          obj_meta.info[i].feature.size = (uint32_t) atoi(text_buffer_feature[0]);
          if (obj_meta.info[i].feature.size > 0) {
            obj_meta.info[i].feature.type = TYPE_INT8;
            obj_meta.info[i].feature.ptr = (int8_t*) malloc(obj_meta.info[i].feature.size * sizeof(int8_t));
            char inFile_feature_path[256];
            sprintf(inFile_feature_path, "%s/%s", args.mot_data_path, text_buffer_feature[1]);
            FILE *inFile_feature;
            inFile_feature = fopen(inFile_feature_path, "r");
            fread(obj_meta.info[i].feature.ptr, sizeof(int8_t), obj_meta.info[i].feature.size, inFile_feature);
            fclose(inFile_feature);
          }
#endif
        }
        CVI_TDL_DeepSORT_Obj(tdl_handle, &obj_meta, &tracker_meta, args.enable_DeepSORT);
      } break;
      case FACE: {
        face_meta.rescale_type = RESCALE_CENTER;
        face_meta.size = bbox_num;
        face_meta.info = (cvtdl_face_info_t *)malloc(face_meta.size * sizeof(cvtdl_face_info_t));
        memset(face_meta.info, 0, face_meta.size * sizeof(cvtdl_face_info_t));
        for (uint32_t i = 0; i < bbox_num; i++) {
          char text_buffer_bbox[5][B_SIZE_BBOX];
          /* read bbox info */
          fscanf(inFile_data, "%s %s %s %s %s", text_buffer_bbox[0], text_buffer_bbox[1],
                 text_buffer_bbox[2], text_buffer_bbox[3], text_buffer_bbox[4]);
          FILL_BBOX(&face_meta.info[i].bbox, text_buffer_bbox);
#if 0
          printf("face[%u] bbox: x1[%.2f], y1[%.2f], x2[%.2f], y2[%.2f]\n", i,
                 face_meta.info[i].bbox.x1, face_meta.info[i].bbox.y1, face_meta.info[i].bbox.x2,
                 face_meta.info[i].bbox.y2);
#endif
          /* read feature info (ignore now) */
          char text_buffer_feature[2][B_SIZE_FEATURE];  // size, bin data path
          fscanf(inFile_data, "%s %s", text_buffer_feature[0], text_buffer_feature[1]);
          FILL_FEATURE(&face_meta.info[i].feature, text_buffer_feature, args.mot_data_path);
        }
        CVI_TDL_DeepSORT_Face(tdl_handle, &face_meta, &tracker_meta);
      } break;
      default:
        break;
    }

    cvtdl_tracker_t inact_trackers;
    memset(&inact_trackers, 0, sizeof(cvtdl_tracker_t));
    CVI_TDL_DeepSORT_GetTracker_Inactive(tdl_handle, &inact_trackers);
    mot_eval_data.update(tracker_meta, inact_trackers);

    if (output_result) {
      fprintf(outFile_result, "%u\n", tracker_meta.size);
      for (uint32_t i = 0; i < tracker_meta.size; i++) {
        cvtdl_bbox_t *target_bbox =
            (args.target_type == FACE) ? &face_meta.info[i].bbox : &obj_meta.info[i].bbox;
        uint64_t u_id =
            (args.target_type == FACE) ? face_meta.info[i].unique_id : obj_meta.info[i].unique_id;
        fprintf(outFile_result, "%" PRIu64 ",%d,%d,%d,%d,%d,%d,%d,%d,%d\n", u_id,
                (int)target_bbox->x1, (int)target_bbox->y1, (int)target_bbox->x2,
                (int)target_bbox->y2, tracker_meta.info[i].state, (int)tracker_meta.info[i].bbox.x1,
                (int)tracker_meta.info[i].bbox.y1, (int)tracker_meta.info[i].bbox.x2,
                (int)tracker_meta.info[i].bbox.y2);
      }
      fprintf(outFile_result, "%u\n", inact_trackers.size);
      for (uint32_t i = 0; i < inact_trackers.size; i++) {
        fprintf(outFile_result, "%" PRIu64 ",-1,-1,-1,-1,%d,%d,%d,%d,%d\n",
                inact_trackers.info[i].id, inact_trackers.info[i].state,
                (int)inact_trackers.info[i].bbox.x1, (int)inact_trackers.info[i].bbox.y1,
                (int)inact_trackers.info[i].bbox.x2, (int)inact_trackers.info[i].bbox.y2);
      }
    }

    CVI_TDL_Free(&inact_trackers);

    switch (args.target_type) {
      case FACE:
        CVI_TDL_Free(&face_meta);
        break;
      case PERSON:
      case VEHICLE:
      case PET:
        CVI_TDL_Free(&obj_meta);
        break;
      default:
        break;
    }
    CVI_TDL_Free(&tracker_meta);
  }

  mot_eval_data.summary(performance);

  fclose(inFile_data);

  return CVI_SUCCESS;
}

static void FILL_BBOX(cvtdl_bbox_t *bbox, char text_buffer[][B_SIZE_BBOX]) {
  bbox->x1 = atof(text_buffer[1]);
  bbox->y1 = atof(text_buffer[2]);
  bbox->x2 = atof(text_buffer[3]);
  bbox->y2 = atof(text_buffer[4]);
}

static void FILL_FEATURE(cvtdl_feature_t *feature, char text_buffer[][B_SIZE_FEATURE],
                         char *data_path) {
  feature->size = (uint32_t)atoi(text_buffer[0]);
  if (feature->size > 0) {
    feature->type = TYPE_INT8;
    feature->ptr = (int8_t *)malloc(feature->size * sizeof(int8_t));
    char inFile_feature_path[256];
    sprintf(inFile_feature_path, "%s/%s", data_path, text_buffer[1]);
    FILE *inFile_feature;
    inFile_feature = fopen(inFile_feature_path, "r");
    fread(feature->ptr, sizeof(int8_t), feature->size, inFile_feature);
    fclose(inFile_feature);
  }
}
