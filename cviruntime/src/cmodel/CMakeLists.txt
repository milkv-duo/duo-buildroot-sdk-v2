cmake_minimum_required(VERSION 2.8.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMODEL_IMPL")

file(GLOB_RECURSE _SOURCES "*.cpp")

set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${_SOURCES})

include_directories(${CMODEL_PATH}/include)
link_directories(${CMODEL_PATH}/lib)
set(CVI_LIBS ${CVI_LIBS} cvicmodel)
set(EXTRA_LIBS ${EXTRA_LIBS} rt dl pthread)

add_library(cviruntime SHARED ${RUNTIME_SOURCES})
add_library(cviruntime-static STATIC ${RUNTIME_SOURCES})

target_link_libraries(cviruntime ${CVI_LIBS} ${EXTRA_LIBS})

install(TARGETS cviruntime DESTINATION lib)
install(TARGETS cviruntime-static DESTINATION lib)
