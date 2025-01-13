include(ExternalProject)
ExternalProject_Add(tracer
                    GIT_REPOSITORY ssh://${DL_SERVER_IP}:29418/cvitek/tracer
                    PREFIX ${BUILD_DOWNLOAD_DIR}/tracer
                    CMAKE_ARGS
                    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                    -DTOOLCHAIN_ROOT_DIR=${TOOLCHAIN_ROOT_DIR}
                    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                    -DCMAKE_INSTALL_MESSAGE=LAZY
                    -DBUILD_SHARED_LIBS=ON
                    -DCMAKE_CXX_FLAGS="-fPIC")
ExternalProject_Get_Property(tracer INSTALL_DIR)
message("Content downloaded to ${BUILD_DOWNLOAD_DIR}/tracer")

set(CVI_TRACER_PATH ${CMAKE_INSTALL_PREFIX}/sample/3rd/tracer)
set(CVI_TRACER_LIBPATH ${INSTALL_DIR}/lib/libcvitracer.so)
set(CVI_TRACER_INCLUDE ${INSTALL_DIR}/include/tracer)

link_directories(${INSTALL_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lcvitracer")
install(FILES ${CVI_TRACER_LIBPATH} DESTINATION ${CMAKE_INSTALL_PREFIX}/sample/3rd/lib)
