include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/utils)

add_subdirectory(coco)
add_subdirectory(lfw)
add_subdirectory(wider_face)
add_subdirectory(market1501)
add_subdirectory(wflw)
add_subdirectory(cityscapes)
add_subdirectory(cvi_lpdr)