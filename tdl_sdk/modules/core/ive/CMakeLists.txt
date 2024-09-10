project(ivewrapper)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
                    ${CMAKE_CURRENT_SOURCE_DIR})

if (USE_TPU_IVE)
set(IMPL_SRC impl_tpu_ive.cpp)
else()
set(IMPL_SRC impl_ive.cpp)
endif()

add_library(${PROJECT_NAME} OBJECT ive.cpp ${IMPL_SRC})
