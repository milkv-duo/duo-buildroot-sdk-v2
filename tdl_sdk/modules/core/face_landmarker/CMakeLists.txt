project(face_landmarker)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils)

add_library(${PROJECT_NAME} OBJECT face_landmarker.cpp face_landmark_det3.cpp face_landmarker_det2.cpp dms_landmark.cpp)

