project(utils-cv)
add_library(${PROJECT_NAME} OBJECT color.cpp
                                   contours.cpp
                                   imgwarp.cpp
                                   shapedescr.cpp
                                   thresh.cpp)