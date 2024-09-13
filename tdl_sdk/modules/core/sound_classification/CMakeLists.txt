project(sound_classification)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/
	            ${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../utils)
add_library(${PROJECT_NAME} OBJECT melspec.cpp sound_classification_v2.cpp)
