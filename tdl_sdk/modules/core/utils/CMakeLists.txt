project(utils)

set(UTILS_SRC core_utils.cpp
              rescale_utils.cpp
              demangle.cpp
              object_utils.cpp
              ccl.cpp
              profiler.cpp
              img_process.cpp
              token.cpp
              clip_postprocess.cpp
              img_warp.cpp)

if(NOT DEFINED NO_OPENCV)
  set(UTILS_SRC ${UTILS_SRC} face_utils.cpp image_utils.cpp neon_utils.cpp)
endif()
add_library(${PROJECT_NAME} OBJECT ${UTILS_SRC})

if ((NOT DEFINED NO_OPENCV) AND ("${ENABLE_CVI_TDL_CV_UTILS}" STREQUAL "ON"))
message("add cv-utils")  
add_subdirectory(cv)
endif()
