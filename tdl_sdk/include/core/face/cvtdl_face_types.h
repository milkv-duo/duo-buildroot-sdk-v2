#ifndef _CVI_FACE_TYPES_H_
#define _CVI_FACE_TYPES_H_
#include "core/core/cvtdl_core_types.h"

/** @enum cvtdl_face_emotion_e
 *  @ingroup core_cvitdlcore
 *  @brief Emotion enum for attribute related TDL models.
 */
typedef enum {
  EMOTION_UNKNOWN = 0,
  EMOTION_HAPPY,
  EMOTION_SURPRISE,
  EMOTION_FEAR,
  EMOTION_DISGUST,
  EMOTION_SAD,
  EMOTION_ANGER,
  EMOTION_NEUTRAL,
  EMOTION_END
} cvtdl_face_emotion_e;

/** @enum cvtdl_face_gender_e
 *  @ingroup core_cvitdlcore
 *  @brief Gender enum for attribute related TDL models.
 */
typedef enum { GENDER_UNKNOWN = 0, GENDER_MALE, GENDER_FEMALE, GENDER_END } cvtdl_face_gender_e;

/** @enum cvtdl_face_race_e
 *  @ingroup core_cvitdlcore
 *  @brief Race enum for attribute related TDL models.
 */
typedef enum {
  RACE_UNKNOWN = 0,
  RACE_CAUCASIAN,
  RACE_BLACK,
  RACE_ASIAN,
  RACE_END
} cvtdl_face_race_e;

/** @enum cvtdl_liveness_ir_position_e
 *  @ingroup core_cvitdlcore
 *  @brief Give liveness TDL inference the hint the physical position of the IR camera is on the
 * left or right side of the RGB camera.
 */
typedef enum { LIVENESS_IR_LEFT = 0, LIVENESS_IR_RIGHT } cvtdl_liveness_ir_position_e;

/** @struct cvtdl_head_pose_t
 *  @ingroup core_cvitdlcore
 *  @brief The data structure for the head pose output.
 *
 *  @var cvtdl_head_pose_t::faacialUnitNormalVector
 *  The Normal vector for the face.
 *  @var cvtdl_head_pose_t::roll
 *  The roll angle of the head pose.
 *  @var cvtdl_head_pose_t::pitch
 *  The pitch angle of the head pose.
 *  @var cvtdl_head_pose_t::yaw
 *  The yaw angle of the head pose.
 */
typedef struct {
  float yaw;
  float pitch;
  float roll;

  // Facial normal means head direction.
  float facialUnitNormalVector[3];  // 0: x-axis, 1: y-axis, 2: z-axis
} cvtdl_head_pose_t;

/** @struct cvtdl_dms_od_info_t
 *  @ingroup core_cvitdlcore
 *  @brief The data structure for the dms object detection output.
 *
 *  @var cvtdl_dms_od_info_t::name
 *  The name for the object.
 *  @var cvtdl_dms_od_info_t::classes
 *  The class for the object.
 *  @var cvtdl_dms_od_info_t::bbox
 *  The bounding box for the object.
 */

typedef struct {
  char name[128];
  int classes;
  cvtdl_bbox_t bbox;
} cvtdl_dms_od_info_t;

/** @struct cvtdl_dms_od_t
 *  @ingroup core_cvitdlcore
 *  @brief The data structure for the dms object detection output.
 *
 *  @var cvtdl_dms_od_t::size
 *  The size for the objects.
 *  @var cvtdl_dms_od_t::width
 *  The frame width for the object detection input.
 *  @var cvtdl_dms_od_t::height
 *  The frame height for the object detection input.
 *  @var cvtdl_dms_od_t::rescale_type
 *  The rescale type for the objects.
 *  @var cvtdl_dms_od_t::info
 *  The info for the objects.
 */

typedef struct {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  meta_rescale_type_e rescale_type;
  cvtdl_dms_od_info_t* info;
} cvtdl_dms_od_t;

/** @struct cvtdl_dms_t
 *  @ingroup core_cvitdlcore
 *  @brief The data structure for storing face meta.
 *
 *  @var cvtdl_dms_t::r_eye_score
 *  The right eye score.
 *  @var cvtdl_dms_t::l_eye_score
 *  The left eye score.
 *  @var cvtdl_dms_t::yawn_score
 *  The yawn score.
 *  @var cvtdl_dms_t::phone_score
 *  The phone score.
 *  @var cvtdl_dms_t::smoke_score
 *  The smoke score.
 *  @var cvtdl_dms_t::landmarks_106
 *  The face 106 landmarks.
 *  @var cvtdl_dms_t::landmarks_5
 *  The face 5 landmarks which is the same as retinaface.
 *  @var cvtdl_dms_t::head_pose
 *  The head pose.
 *  @var cvtdl_dms_t::dms_od
 *  The dms od info.
 *
 *  @see cvtdl_face_info_t
 */

typedef struct {
  float reye_score;
  float leye_score;
  float yawn_score;
  float phone_score;
  float smoke_score;
  cvtdl_pts_t landmarks_106;
  cvtdl_pts_t landmarks_5;
  cvtdl_head_pose_t head_pose;
  cvtdl_dms_od_t dms_od;
} cvtdl_dms_t;

/** @struct cvtdl_face_info_t
 *  @ingroup core_cvitdlcore
 *  @brief The data structure for storing a single face information.
 *
 *  @var cvtdl_face_info_t::name
 *  A human readable name.
 *  @var cvtdl_face_info_t::unique_id
 *  The unique id of a face.
 *  @var cvtdl_face_info_t::bbox
 *  The bounding box of a face. Refers to the width, height from cvtdl_face_t.
 *  @var cvtdl_face_info_t::pts
 *  The point to describe the point on the face.
 *  @var cvtdl_face_info_t::feature
 *  The feature to describe a face.
 *  @var cvtdl_face_info_t::emotion
 *  The emotion from attribute.
 *  @var cvtdl_face_info_t::gender
 *  The gender from attribute.
 *  @var cvtdl_face_info_t::race
 *  The race from attribute.
 *  @var cvtdl_face_info_t::age
 *  The age.
 *  @var cvtdl_face_info_t::liveness_score
 *  The liveness score.
 *  @var cvtdl_face_info_t::hardhat_score
 *  The hardhat score.
 *  @var cvtdl_face_info_t::mask_score
 *  The mask score.
 *  @var cvtdl_face_info_t::face_quality
 *  The face quality.
 *  @var cvtdl_face_info_t::head_pose;
 *  The head pose.
 *
 *  @see cvtdl_face_t
 */

typedef struct {
  char name[128];
  uint64_t unique_id;
  cvtdl_bbox_t bbox;
  cvtdl_pts_t pts;
  cvtdl_feature_t feature;
  cvtdl_face_emotion_e emotion;
  cvtdl_face_gender_e gender;
  cvtdl_face_race_e race;
  float score;
  float gender_score;
  float glass;
  float age;
  float liveness_score;
  float hardhat_score;
  float mask_score;
  float recog_score;
  float face_quality;
  float pose_score;
  float pose_score1;
  float sharpness_score;
  float blurness;
  cvtdl_head_pose_t head_pose;
  int track_state;
  float velx;
  float vely;
} cvtdl_face_info_t;

/** @struct cvtdl_face_t
 *  @ingroup core_cvitdlcore
 *  @brief The data structure for storing face meta.
 *
 *  @var cvtdl_face_t::size
 *  The size of the info.
 *  @var cvtdl_face_t::width
 *  The current width. Affects the coordinate recovery of bbox and pts.
 *  @var cvtdl_face_t::height
 *  The current height. Affects the coordinate recovery of bbox and pts.
 *  @var cvtdl_face_t::info
 *  The information of each face.
 *  @var cvtdl_face_t::dms
 *  The dms of face.
 *
 *  @see cvtdl_face_info_t
 */
typedef struct {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  meta_rescale_type_e rescale_type;
  cvtdl_face_info_t* info;
  cvtdl_dms_t* dms;
} cvtdl_face_t;

#endif
