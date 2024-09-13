#ifndef _CVI_FACE_HELPER_H_
#define _CVI_FACE_HELPER_H_
#include "cvtdl_face_types.h"

/**
 * @brief Convert cvtdl_face_emotion_e to human readable string.
 * @ingroup core_cvitdlcore
 *
 * @param emotion Input cvtdl_face_emotion_e enum.
 * @return const char* Emotion string.
 */
inline const char* getEmotionString(cvtdl_face_emotion_e emotion) {
  switch (emotion) {
    case EMOTION_HAPPY:
      return "Happy";
    case EMOTION_SURPRISE:
      return "Surprise";
    case EMOTION_FEAR:
      return "Fear";
    case EMOTION_DISGUST:
      return "Disgust";
    case EMOTION_SAD:
      return "Sad";
    case EMOTION_ANGER:
      return "Anger";
    case EMOTION_NEUTRAL:
      return "Neutral";
    default:
      return "Unknown";
  }
  return "";
}

/**
 * @brief Convert cvtdl_face_gender_e to human readable string.
 * @ingroup core_cvitdlcore
 *
 * @param gender Input cvtdl_face_gender_e enum.
 * @return const char* Gender string.
 */
inline const char* getGenderString(cvtdl_face_gender_e gender) {
  switch (gender) {
    case GENDER_MALE:
      return "Male";
    case GENDER_FEMALE:
      return "Female";
    default:
      return "Unknown";
  }
  return "";
}

/**
 * @brief Convert cvtdl_face_race_e to human readable string.
 * @ingroup core_cvitdlcore
 *
 * @param race Input cvtdl_face_race_e enum.
 * @return const char* Race string.
 */
inline const char* getRaceString(cvtdl_face_race_e race) {
  switch (race) {
    case RACE_CAUCASIAN:
      return "Caucasian";
    case RACE_BLACK:
      return "Black";
    case RACE_ASIAN:
      return "Asian";
    default:
      return "Unknown";
  }
  return "";
}

#endif