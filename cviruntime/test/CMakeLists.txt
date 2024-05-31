cmake_minimum_required(VERSION 2.8.0)
enable_testing()

include(CTest)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test_utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_SYSROOT}/include)

set(CVI_LIBS ${CVI_LIBS} cviruntime)

set(_TEST_UTILS
  test_utils/test_tf_quant_util.c
  test_utils/test_native_ref.c
  test_utils/test_cvikernel_util.c
)
add_library(cviruntime_test STATIC ${_TEST_UTILS})

if (CHIP STREQUAL "cv183x" OR RUNTIME STREQUAL "CMODEL")
  # SOC BM1880v2 or CMODEL all in one
  file(GLOB_RECURSE TEST_1880v2_CASES 1880v2/*.cpp)
  foreach(TEST_SRC ${TEST_1880v2_CASES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CVI_LIBS} ${EXTRA_LIBS} cviruntime_test)
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
  endforeach()

  # C version test
  file(GLOB TEST_1880v2_NEW_CASES 1880v2/test_*.c)

  foreach(TEST_SRC ${TEST_1880v2_NEW_CASES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CVI_LIBS} ${EXTRA_LIBS}
                          cviruntime_test m)
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
  endforeach()
endif()
 
if(CHIP STREQUAL "cv182x" OR RUNTIME STREQUAL "CMODEL")
  # SOC BM1822 or CMODEL all in one
  file(GLOB_RECURSE TEST_1822_CASES 1822/*.cpp)
  foreach(TEST_SRC ${TEST_1822_CASES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CVI_LIBS} ${EXTRA_LIBS} cviruntime_test)
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
  endforeach()

  # C version test
  file(GLOB TEST_1822_NEW_CASES 1822/test_*.c)

  foreach(TEST_SRC ${TEST_1822_NEW_CASES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CVI_LIBS} ${EXTRA_LIBS}
                          cviruntime_test m)
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
  endforeach()
endif()

if(CHIP STREQUAL "cv181x" OR RUNTIME STREQUAL "CMODEL")
  # SOC BM1810 or CMODEL all in one
  file(GLOB_RECURSE TEST_CV181X_CASES 181x/*.cpp)
  foreach(TEST_SRC ${TEST_CV181X_CASES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CVI_LIBS} ${EXTRA_LIBS} cviruntime_test)
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
  endforeach()

  # C version test
  file(GLOB TEST_CV181X_NEW_CASES 181x/test_*.c)

  foreach(TEST_SRC ${TEST_CV181X_NEW_CASES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CVI_LIBS} ${EXTRA_LIBS}
                          cviruntime_test m)
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
  endforeach()
endif()

if(CHIP STREQUAL "cv180x" OR RUNTIME STREQUAL "CMODEL")
  # SOC BM1810 or CMODEL all in one
  file(GLOB_RECURSE TEST_CV180X_CASES 180x/*.cpp)
  foreach(TEST_SRC ${TEST_CV180X_CASES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CVI_LIBS} ${EXTRA_LIBS} cviruntime_test)
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
  endforeach()

  # C version test
  file(GLOB TEST_CV180X_NEW_CASES 180x/test_*.c)

  foreach(TEST_SRC ${TEST_CV180X_NEW_CASES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CVI_LIBS} ${EXTRA_LIBS}
                          cviruntime_test m)
    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "-Werror -Wall -Wextra")
    install(TARGETS ${TEST_NAME} DESTINATION bin)

    add_test(${TEST_NAME} ${TEST_NAME} ctest_test)
  endforeach()
endif()
