cmake_minimum_required(VERSION 2.8.0)
project(transpose CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
include_directories(${TPU_SDK_PATH}/include)
link_directories(${TPU_SDK_PATH}/lib)

set(CVI_LIBS cviruntime cvikernel)
add_executable(add add.cpp)
target_link_libraries(add ${CVI_LIBS})
install(TARGETS add DESTINATION bin)


