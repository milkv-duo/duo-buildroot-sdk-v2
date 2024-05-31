cmake_minimum_required(VERSION 2.8.0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/kernel_function)

set(RUNTIME_SOURCES
   ${CMAKE_CURRENT_SOURCE_DIR}/common/model.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/neuron.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/program.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/stream.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/section.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/runtime.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/debug.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/taskpool.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/shared_mem.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/alloc.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/kernel_function/kernelFunc.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/kernel_function/euclideanDist.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/kernel_function/matrixMul.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/kernel_function/grayImageLight.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/common/kernel_function/tdmaCopy.cpp)

if (${ENABLE_CPU_FUNC})
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/deformableconv.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/deform_im2col.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/instancenorm.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/interpolation.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/quant.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/proposal.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/roi_pooling.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/frcn_detection.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/reducel2.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/reducemean.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/reducemax.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/retinaface_detection.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/softmax.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/preprocess.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/transpose.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/ssd_detection.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/argmax.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/argmax_v2.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/argmax_v3.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/pixelshuffle.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/yolo_detection.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/embedding.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/gathernd.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/grid_sampler.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/cumsum.cpp)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/cpu_function/gatherelements_pt.cpp)
endif()

if (${ENABLE_COMPRESS_CMDBUF})
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/lz4/lz4_all.c)
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/lz4/xxhash.c)
endif()

if (RUNTIME STREQUAL "SOC")
  if (CHIP STREQUAL cv183x)
    add_subdirectory(soc/183x)
  elseif (CHIP STREQUAL cv182x)
    add_subdirectory(soc/182x)
  elseif (CHIP STREQUAL cv181x)
    add_subdirectory(soc/181x)
  elseif (CHIP STREQUAL cv180x)
    add_subdirectory(soc/180x)
  endif()
elseif (RUNTIME STREQUAL "CMODEL")
  set(RUNTIME_SOURCES ${RUNTIME_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/common/mmpool.cpp)
  add_subdirectory(cmodel)
else()
  message(FATAL_ERROR "Unknown RUNTIME: " ${RUNTIME})
endif()

