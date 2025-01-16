project(thirdparty_fetchcontent)

if (NOT IS_DIRECTORY  "${BUILD_DOWNLOAD_DIR}/libeigen-src")
FetchContent_Declare(
  libeigen
  GIT_REPOSITORY https://github.com/milkv-duo/eigen.git
  GIT_TAG 3c4637640b449a17d56ff472d8325c47ac10eba3
)
FetchContent_MakeAvailable(libeigen)
message("Content downloaded to ${libeigen_SOURCE_DIR}")
endif()
include_directories(${BUILD_DOWNLOAD_DIR}/libeigen-src)

set(BUILD_GMOCK OFF CACHE BOOL "Build GMOCK")
set(INSTALL_GTEST OFF CACHE BOOL "Install GMOCK")
if (NOT IS_DIRECTORY "${BUILD_DOWNLOAD_DIR}/googletest-src")
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG  e2239ee6043f73722e7aa812a459f54a28552929 # release-1.11.0
)
FetchContent_MakeAvailable(googletest)
message("Content downloaded to ${googletest_SOURCE_DIR}")
else()
  project(googletest)
    add_subdirectory(${BUILD_DOWNLOAD_DIR}/googletest-src/)
endif()
include_directories(${BUILD_DOWNLOAD_DIR}/googletest-src/googletest/include/gtest)

if(NOT IS_DIRECTORY "${BUILD_DOWNLOAD_DIR}/nlohmannjson-src")
set(nlohmannjson_SOURCE_DIR "${BUILD_DOWNLOAD_DIR}/nlohmannjson-src")
file(MAKE_DIRECTORY ${nlohmannjson_SOURCE_DIR})
file(DOWNLOAD "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp" "${nlohmannjson_SOURCE_DIR}/json.hpp")
message("Content downloaded to ${nlohmannjson_SOURCE_DIR}")
endif()
include_directories(${BUILD_DOWNLOAD_DIR}/nlohmannjson-src)

if (BUILD_WEB_VIEW)
  if(NOT IS_DIRECTORY "${BUILD_DOWNLOAD_DIR}/sophapp-src")
    FetchContent_Declare(
      sophapp
      GIT_REPOSITORY https://github.com/sophgo/sophapp.git
      GIT_TAG origin/ipcamera
    )
    FetchContent_MakeAvailable(sophapp)
    message("Content downloaded to ${sophapp_SOURCE_DIR}")
  endif()
  set(sophapp_SOURCE_DIR ${BUILD_DOWNLOAD_DIR}/sophapp-src)
endif()

if(NOT IS_DIRECTORY "${BUILD_DOWNLOAD_DIR}/stb-src")
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG origin/master
)
  FetchContent_MakeAvailable(stb)
  message("Content downloaded to ${stb_SOURCE_DIR}")
endif()
set(stb_SOURCE_DIR ${BUILD_DOWNLOAD_DIR}/stb-src)
include_directories(${stb_SOURCE_DIR})

install(DIRECTORY  ${stb_SOURCE_DIR}/ DESTINATION sample/3rd/stb/include
    FILES_MATCHING PATTERN "*.h"
    PATTERN ".git" EXCLUDE
    PATTERN ".github" EXCLUDE
    PATTERN "data" EXCLUDE
    PATTERN "deprecated" EXCLUDE
    PATTERN "docs" EXCLUDE
    PATTERN "tests" EXCLUDE
    PATTERN "tools" EXCLUDE)
