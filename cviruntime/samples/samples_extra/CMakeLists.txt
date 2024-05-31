cmake_minimum_required(VERSION 2.8.0)

project(cvitek_samples C CXX)

add_subdirectory(alphapose_fused_preprocess)
add_subdirectory(detector_yolov3_fused_preprocess)
add_subdirectory(detector_yolov5_fused_preprocess)
add_subdirectory(detector_yolov5-face_fused_preprocess)
add_subdirectory(detector_yolox_s)
add_subdirectory(insightface_fused_preprocess)
add_subdirectory(detector_ppyoloem_fused_preprocess)
add_subdirectory(detector_yolov8n_fused_preprocess)

set(SCRIPT_FILES
    run_alphapose_fused_preprocess.sh
    run_detector_yolov3_fused_preprocess.sh
    run_detector_yolov5_fused_preprocess.sh
    run_detector_yolox_s.sh
    run_insightface_fused_preprocess.sh
    run_detector_ppyoloem_fused_preprocess.sh)

install(FILES ${SCRIPT_FILES}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
    DESTINATION samples_extra)

install(DIRECTORY data DESTINATION samples_extra)