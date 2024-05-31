CMAKE_MINIMUM_REQUIRED(VERSION 3.0 FATAL_ERROR)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if (NOT DEFINED LLVM_MAIN_SRC_DIR)
  project(CNPY)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  message(STATUS "CMAKE_SYSROOT ${CMAKE_SYSROOT}")
  include_directories(${CMAKE_SYSROOT}/include)

  include_directories(${CMAKE_INSTALL_PREFIX}/include)
  link_directories(${CMAKE_INSTALL_PREFIX}/lib)

  set(ENV{PKG_CONFIG_DIR} "")
  set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")
  set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
  find_package(ZLIB)

  option(ENABLE_STATIC "Build static (.a) library" ON)
  add_library(cnpy SHARED "cnpy.cpp")
  target_link_libraries(cnpy z)
  install(TARGETS "cnpy" LIBRARY DESTINATION lib PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  if(ENABLE_STATIC)
    add_library(cnpy-static STATIC "cnpy.cpp")
    set_target_properties(cnpy-static PROPERTIES OUTPUT_NAME "cnpy")
    install(TARGETS "cnpy-static" ARCHIVE DESTINATION lib)
  endif(ENABLE_STATIC)

  install(FILES "cnpy.h" DESTINATION include)
  install(FILES "mat2npz" "npy2mat" "npz2mat" DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  add_executable(example1 example1.cpp)
  target_link_libraries(example1 cnpy)

else()
  message(STATUS "CNPY LLVM Tree Build")

  add_custom_target(CNPY)
  set_target_properties(CNPY PROPERTIES FOLDER Third_party)
  add_dependencies(CNPY
    cnpy
    cnpy_example1
  )

  set(LLVM_OPTIONAL_SOURCES
    cnpy.cpp
    example1.cpp
  )

  find_package(ZLIB REQUIRED)

  add_llvm_library(cnpy SHARED
    cnpy.cpp)
  target_link_libraries(cnpy ${ZLIB_LIBRARIES})

  install(FILES "cnpy.h" DESTINATION include)
  add_llvm_executable(cnpy_example1
    example1.cpp)
  target_link_libraries(cnpy_example1 PRIVATE
    cnpy)
endif()
