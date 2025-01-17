project(ive)
set(CVIIVE_TAG ${LATEST_TAG})
configure_file(version.hpp.in version.hpp @ONLY)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty
    ${MLIR_INCLUDES}
    ${MIDDLEWARE_INCLUDES}
    ${KERNEL_HEADERS_ROOT}/include
)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/table_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_add.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_add_bf16.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_and.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_block_bf16.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_block.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_const_fill.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_copy_direct.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_copy_interval.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_magandang.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_mask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_max.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_min.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_morph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_mulsum.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_normalize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_or.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_sad.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_sigmoid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_sobel_gradonly.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_sobel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_sub_abs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_sub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_threshold_high_low.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_threshold_slope.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_threshold.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_xor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_alpha_blend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_alpha_blend_pixel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_convert_scale_abs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_blend_pixel_ab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_cmp_sat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_downsample.cpp
)

set(DRAWSRC ${CMAKE_CURRENT_SOURCE_DIR}/ive_draw.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/2ddraw/tpu_draw_rect.cpp)

add_library(cvi_ive_tpu SHARED ${SRC} ${DRAWSRC})
target_link_libraries(cvi_ive_tpu ${MLIR_LIBS})
add_library(cvi_ive_tpu-static STATIC ${SRC} ${DRAWSRC})

set(EX_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu_ive.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/table_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_alpha_blend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_alpha_blend_pixel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_convert_scale_abs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tpu/tpu_blend_pixel_ab.cpp
)
add_library(cvi_ive_tpu_ex SHARED ${EX_SRC})
target_link_libraries(cvi_ive_tpu_ex ${MLIR_LIBS})
add_library(cvi_ive_tpu_ex-static STATIC ${EX_SRC}) ###########
SET_TARGET_PROPERTIES(cvi_ive_tpu_ex-static PROPERTIES OUTPUT_NAME "cvi_ive_tpu_ex") #############

SET_TARGET_PROPERTIES(cvi_ive_tpu_ex-static PROPERTIES SUFFIX ".a")  #############


install(TARGETS cvi_ive_tpu DESTINATION lib)
install(TARGETS cvi_ive_tpu-static DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/cvi_comm_ive.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/cvi_draw_ive.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/ive.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/ive_draw.h DESTINATION include)


install(TARGETS cvi_ive_tpu_ex DESTINATION ex_lib)
install(TARGETS cvi_ive_tpu_ex-static DESTINATION ex_lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/cvi_tpu_ive.h DESTINATION ex_include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/cvi_comm_ive.h DESTINATION ex_include)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/ive_180x/cvi_comm_ive.h DESTINATION include/ive)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../include/ive_180x/ive.h DESTINATION include/ive)