cmake_minimum_required(VERSION 2.8.0)

include_directories(./)
include_directories(../common)
include_directories(${CMAKE_SYSROOT}/include)
add_definitions(-DION_CACHE_OPEN)
add_definitions(-DMULTI_PROCESS)

set(RUNTIME_SOURCES ${RUNTIME_SOURCES}
   ${CMAKE_CURRENT_SOURCE_DIR}/../runtime_bmkernel.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/bmruntime_soc.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/bm_dmabuf.c
   ${CMAKE_CURRENT_SOURCE_DIR}/tpu_pmu.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/cvi_rt_183x.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/cvi183x_device_mem.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../common/cvi_device_mem.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../common/cvi_rt_base.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../common/cviruntime_context.cpp)

set(EXTRA_LIBS ${EXTRA_LIBS} rt dl pthread)

add_library(cviruntime-static STATIC ${RUNTIME_SOURCES})
set_property(TARGET cviruntime-static PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(cviruntime SHARED ${RUNTIME_SOURCES})
target_link_libraries(cviruntime cvikernel ${EXTRA_LIBS})

install(TARGETS cviruntime DESTINATION lib)
install(TARGETS cviruntime-static DESTINATION lib)
