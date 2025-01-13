project(test_utils)

set(UTILS_SRC ${UTILS_SRC} face_utils.cpp sys_utils.cpp neon_utils.cpp)
add_library(${PROJECT_NAME} OBJECT ${UTILS_SRC})