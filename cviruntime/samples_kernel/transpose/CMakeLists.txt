cmake_minimum_required(VERSION 2.8.0)
project(transpose CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

include_directories(${TPU_BASE}/cvitek_mlir/include)
link_directories(${TPU_BASE}/cvitek_mlir/lib)

set(CVI_LIBS cvicmodel cviruntime cvikernel)
add_executable(transpose transpose.cpp)
target_link_libraries(transpose ${CVI_LIBS})


