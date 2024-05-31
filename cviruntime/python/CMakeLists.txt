cmake_minimum_required(VERSION 3.1.0)
project(pyruntime)

if(CMAKE_CROSSCOMPILING)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
  include_directories(${CMAKE_SYSROOT}/include)
  link_directories(${CNPY_PATH})
endif()

add_subdirectory(include/pybind11)
pybind11_add_module(pyruntime pyruntime.cpp)

set(CVI_LIBS ${CVI_LIBS} cviruntime cvikernel)
target_link_libraries(pyruntime PRIVATE ${CVI_LIBS})
install(TARGETS pyruntime DESTINATION python)
