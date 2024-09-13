project(deepsort)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils)

add_library(${PROJECT_NAME} OBJECT cvi_deepsort.cpp
                                   cvi_deepsort.cpp
                                   cvi_deepsort_utils.cpp
                                   cvi_kalman_filter.cpp
                                   cvi_kalman_tracker.cpp
                                   cvi_munkres.cpp
                                   cvi_distance_metric.cpp
                                   pair_track.cpp)
