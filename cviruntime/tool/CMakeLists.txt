cmake_minimum_required(VERSION 2.8.0)

include_directories(${PROJECT_SOURCE_DIR}/../../third_party/cnpy)
if(DEFINED CNPY_PATH)
  include_directories(${CNPY_PATH}/include)
  link_directories(${CNPY_PATH}/lib)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
if(CMAKE_CROSSCOMPILING)
  include_directories(${CMAKE_SYSROOT}/include)
  link_directories(${CNPY_PATH})
endif()

set(CVI_LIBS ${CVI_LIBS} cviruntime)

set(CVI_LIBS ${CVI_LIBS} cnpy z)
add_executable(model_runner model_runner.cpp)
target_link_libraries(model_runner ${CVI_LIBS} ${EXTRA_LIBS})

add_executable(multi_model_tester multi_model_tester.cpp)
target_link_libraries(multi_model_tester ${CVI_LIBS} ${EXTRA_LIBS})

add_executable(multi_thread_tester multi_thread_tester.cpp)
target_link_libraries(multi_thread_tester ${CVI_LIBS} ${EXTRA_LIBS})

add_executable(model_interface_tester model_interface_tester.cpp)
target_link_libraries(model_interface_tester ${CVI_LIBS} ${EXTRA_LIBS})

add_executable(stress_tester stress_tester.cpp)
target_link_libraries(stress_tester ${CVI_LIBS} ${EXTRA_LIBS})

add_executable(cvimodel_tool cvimodel_tool.cpp md5.cpp)
target_link_libraries(cvimodel_tool cviruntime)

install(TARGETS model_runner 
        multi_model_tester cvimodel_tool 
        model_interface_tester stress_tester
        multi_thread_tester
        DESTINATION bin)