cmake_minimum_required(VERSION 3.1.0)

project(cviruntime C CXX)

execute_process(
  COMMAND git describe --always --tags --dirty
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE GIT_EXEC_RESULT
  OUTPUT_VARIABLE GIT_SHORT_HASH)

string(STRIP ${GIT_SHORT_HASH} GIT_SHORT_HASH)
string(TIMESTAMP BUILD_TIME "%Y%m%d")
set(RUNTIME_VERSION "${GIT_SHORT_HASH}@${BUILD_TIME}")
message(STATUS "runtime version: ${RUNTIME_VERSION}")
add_definitions(-DRUNTIME_VERSION="${RUNTIME_VERSION}")

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "\${ORIGIN}/../lib;\${ORIGIN}/")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

option(ENABLE_COMPRESS_CMDBUF "enable compressed cmdbuf" ON)
option(ENABLE_CPU_FUNC "enable cpu functions" ON)
option(ENABLE_PMU "enable tpu PMU" ON)

set(SAFETY_FLAGS "-Werror -Wall -Wextra -fno-strict-aliasing -Wno-missing-field-initializers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SAFETY_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SAFETY_FLAGS}")

if(CMAKE_CROSSCOMPILING)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftree-vectorize -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize -Wno-unused-parameter")
endif()

if (NOT DEFINED CHIP)
  message(FATAL_ERROR "no CHIP specified")
endif()
if (NOT DEFINED RUNTIME)
  message(FATAL_ERROR "no RUNTIME specified")
endif()
if(NOT DEFINED CVIKERNEL_PATH)
  message(FATAL_ERROR "Please set CVIKERNEL_PATH to point to the cvikernel installation")
endif()
if (NOT DEFINED FLATBUFFERS_PATH)
  message(FATAL_ERROR "Please set FLATBUF_PATH")
endif()
if(NOT DEFINED CVIBUILDER_PATH)
  message(FATAL_ERROR "Please set CVIBUILDER_PATH to point to the CVIBUILDER installation")
endif()
if(RUNTIME STREQUAL CMODEL)
  if(NOT DEFINED CMODEL_PATH)
    message(FATAL_ERROR "Please set CMODEL_PATH to point to the cmodel source installation")
  endif()
endif()

message(STATUS "CHIP: ${CHIP}")
message(STATUS "RUNTIME: ${RUNTIME}")
message(STATUS "CMODEL_PATH: ${CMODEL_PATH}")
message(STATUS "CVIKERNEL_PATH: ${CVIKERNEL_PATH}")
message(STATUS "FLATBUFFERS_PATH: ${FLATBUFFERS_PATH}")
message(STATUS "CVIBUILDER_PATH: ${CVIBUILDER_PATH}")


message(STATUS "CHIP: ${CHIP}")

if (CHIP STREQUAL cv183x)
  add_definitions(-DCHIPID=0x1)
elseif (CHIP STREQUAL cv182x)
  add_definitions(-DCHIPID=0x2)
elseif (CHIP STREQUAL cv181x)
  add_definitions(-DCHIPID=0x3)
  set(ENABLE_COMPRESS_CMDBUF OFF CACHE BOOL "" FORCE)
elseif (CHIP STREQUAL cv180x)
  add_definitions(-DCHIPID=0x4)
  set(ENABLE_COMPRESS_CMDBUF OFF CACHE BOOL "" FORCE)
  set(ENABLE_CPU_FUNC OFF CACHE BOOL "" FORCE)
endif()
add_definitions(-DCHIP=${CHIP})

if (${ENABLE_COMPRESS_CMDBUF})
  add_definitions(-DENABLE_COMPRESS_CMDBUF)
endif()

if (${ENABLE_CPU_FUNC})
  add_definitions(-DENABLE_CPU_FUNC)
endif()

if (${ENABLE_PMU})
  add_definitions(-DENABLE_PMU)
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src/common
  ${CVIBUILDER_PATH}/include
  ${FLATBUFFERS_PATH}/include
  ${CVIKERNEL_PATH}/include
  ${CMAKE_CURRENT_BINARY_DIR})

if (${ENABLE_COMPRESS_CMDBUF}) 
  include_directories(${PROJECT_SOURCE_DIR}/include/lz4)
endif()

link_directories(${CVIKERNEL_PATH}/lib)
set(CVI_LIBS ${CVI_LIBS} cvikernel)

if(RUNTIME STREQUAL CMODEL)
  include_directories(${CMODEL_PATH}/include)
  link_directories(${CMODEL_PATH}/lib)
  set(CVI_LIBS ${CVI_LIBS} cvicmodel)
endif()

add_subdirectory(src)
add_subdirectory(tool)
if (ENABLE_PYRUNTIME STREQUAL "ON")
  add_subdirectory(python)
endif()

if (ENABLE_TEST STREQUAL "ON")
  add_subdirectory(test)
endif()

if (NOT CMAKE_CROSSCOMPILING)
  if (ENABLE_TEST STREQUAL "ON")
    enable_testing()
  endif()
endif()

file(GLOB HEADERS
  include/cviruntime.h
  include/bmruntime.h
  include/bmruntime_bmkernel.h
  include/cviruntime_context.h
  include/cviruntime_extra.h
  include/cvitpu_debug.h)
install(FILES ${HEADERS} DESTINATION include)

file(GLOB RUNTIME_HEADERS
  include/runtime/cpu_function.hpp
  include/runtime/neuron.hpp
  include/runtime/op_param.hpp)
install(FILES ${RUNTIME_HEADERS} DESTINATION include/runtime)

if(NOT CMAKE_CROSSCOMPILING)
  # install the whole sample dir as source code
  install(DIRECTORY samples DESTINATION .)
endif()

install(FILES scripts/envs_tpu_sdk.sh
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
    DESTINATION .)

if (CHIP STREQUAL cv183x)
  install(FILES scripts/regression_new_models_cv183x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_models.sh)
  install(FILES scripts/regression_models_e2e_cv183x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_models_e2e.sh)
  install(FILES scripts/regression_samples_cv183x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_samples.sh)
elseif (CHIP STREQUAL cv182x)
  install(FILES scripts/regression_new_models_cv182x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_models.sh)
  install(FILES scripts/regression_models_e2e_cv182x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_models_e2e.sh)
  install(FILES scripts/regression_samples_cv182x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_samples.sh)
elseif (CHIP STREQUAL cv181x)
  install(FILES scripts/regression_new_models_cv181x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_models.sh)
  install(FILES scripts/regression_models_e2e_cv181x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_models_e2e.sh)
  install(FILES scripts/regression_samples_cv181x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_samples.sh)
elseif (CHIP STREQUAL cv180x)
  install(FILES scripts/regression_new_models_cv180x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_models.sh)
  install(FILES scripts/regression_models_e2e_cv180x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_models_e2e.sh)
  install(FILES scripts/regression_samples_cv180x.sh
      PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
      DESTINATION . RENAME regression_samples.sh)
endif()
