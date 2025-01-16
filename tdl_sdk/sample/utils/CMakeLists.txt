project(test_utils)

set(UTILS_SRC ${UTILS_SRC} sys_utils.c)
add_library(${PROJECT_NAME} OBJECT ${UTILS_SRC})