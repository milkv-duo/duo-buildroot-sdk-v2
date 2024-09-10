#pragma once

#include "core/face/cvtdl_face_helper.h"
#include "core/face/cvtdl_face_types.h"

#include <vector>

namespace cvitdl {
template <int DIM>
class FeatureVector final {
 public:
  std::vector<float> features;

  FeatureVector() : features(DIM, 0) {}
  FeatureVector(const FeatureVector &other) { *this = other; }

  inline size_t size() { return DIM; }

  FeatureVector &operator=(const FeatureVector &other) {
    features = std::vector<float>(other.features.begin(), other.features.end());
    return *this;
  }

  FeatureVector &operator=(FeatureVector &&other) {
    features.swap(other.features);
    return *this;
  }

  FeatureVector &operator+=(const FeatureVector &other) {
    for (size_t i = 0; i < DIM; i++) {
      features[i] += other.features[i];
    }
    return *this;
  }

  FeatureVector &operator-=(const FeatureVector &other) {
    for (size_t i = 0; i < DIM; i++) {
      features[i] -= other.features[i];
    }
    return *this;
  }

  FeatureVector &operator*=(const FeatureVector &other) {
    for (size_t i = 0; i < DIM; i++) {
      features[i] *= other.features[i];
    }
    return *this;
  }

  FeatureVector &operator/=(const float num) {
    for (size_t i = 0; i < DIM; i++) {
      features[i] /= num;
    }
    return *this;
  }

  inline bool operator==(const FeatureVector &rhs) const {
    for (int i = 0; i < DIM; i++) {
      if (this->features[i] != rhs.features[i]) {
        return false;
      }
    }
    return true;
  }

  inline bool operator!=(const FeatureVector &rhs) const { return !(*this == rhs); }
  inline float &operator[](int x) { return features[x]; }

  int max_idx() const {
    float tmp = features[0];
    int res = 0;
    for (int i = 0; i < DIM; i++) {
      if (features[i] > tmp) {
        tmp = features[i];
        res = i;
      }
    }
    return res;
  }
};

#define ATTR_FACE_FEATURE_DIM 512
#define ATTR_EMOTION_FEATURE_DIM 7
#define ATTR_GENDER_FEATURE_DIM 2
#define ATTR_RACE_FEATURE_DIM 3
#define ATTR_AGE_FEATURE_DIM 101

typedef FeatureVector<ATTR_FACE_FEATURE_DIM> FaceFeature;
typedef FeatureVector<ATTR_EMOTION_FEATURE_DIM> EmotionFeature;
typedef FeatureVector<ATTR_GENDER_FEATURE_DIM> GenderFeature;
typedef FeatureVector<ATTR_RACE_FEATURE_DIM> RaceFeature;
typedef FeatureVector<ATTR_AGE_FEATURE_DIM> AgeFeature;

class FaceAttributeInfo final {
 public:
  FaceAttributeInfo() = default;
  FaceAttributeInfo(FaceAttributeInfo &&other) { *this = std::move(other); }

  FaceAttributeInfo(const FaceAttributeInfo &other) { *this = other; }

  FaceAttributeInfo &operator=(const FaceAttributeInfo &other) {
    this->face_feature = other.face_feature;
    this->emotion_prob = other.emotion_prob;
    this->gender_prob = other.gender_prob;
    this->race_prob = other.race_prob;
    this->age_prob = other.age_prob;
    this->emotion = other.emotion;
    this->gender = other.gender;
    this->race = other.race;
    this->age = other.age;
    return *this;
  }

  FaceAttributeInfo &operator=(FaceAttributeInfo &&other) {
    this->face_feature = std::move(other.face_feature);
    this->emotion_prob = std::move(other.emotion_prob);
    this->gender_prob = std::move(other.gender_prob);
    this->race_prob = std::move(other.race_prob);
    this->age_prob = std::move(other.age_prob);
    this->emotion = other.emotion;
    this->gender = other.gender;
    this->race = other.race;
    this->age = other.age;
    return *this;
  }

  inline bool operator==(const FaceAttributeInfo &rhs) const {
    if (this->face_feature != rhs.face_feature || this->emotion_prob != rhs.emotion_prob ||
        this->gender_prob != rhs.gender_prob || this->race_prob != rhs.race_prob ||
        this->age_prob != rhs.age_prob || this->emotion != rhs.emotion ||
        this->gender != rhs.gender || this->race != rhs.race || this->age != rhs.age) {
      return false;
    }
    return true;
  }

  inline bool operator!=(const FaceAttributeInfo &rhs) const { return !(*this == rhs); }

  FaceFeature face_feature;
  EmotionFeature emotion_prob;
  GenderFeature gender_prob;
  RaceFeature race_prob;
  AgeFeature age_prob;
  cvtdl_face_emotion_e emotion = EMOTION_UNKNOWN;
  cvtdl_face_gender_e gender = GENDER_UNKNOWN;
  cvtdl_face_race_e race = RACE_UNKNOWN;
  float age = -1.0;
};
}  // namespace cvitdl