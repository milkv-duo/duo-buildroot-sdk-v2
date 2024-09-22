cmake_minimum_required(VERSION 3.1.0)

project(cvibuilder C CXX)

if (NOT DEFINED FLATBUFFERS_PATH)
  message(FATAL_ERROR "Please set FLATBUF_PATH")
endif()
message(STATUS "FLATBUFFERS_PATH: ${FLATBUFFERS_PATH}")

function(compile_fbs)
  foreach(fbs_file IN LISTS ARGN)
    get_filename_component(name ${fbs_file} NAME_WE)
    set(target_header ${CMAKE_CURRENT_BINARY_DIR}/include/cvibuilder/${name}_generated.h)
    message(STATUS "To compile fbs files ${fbs_file} for ${target_header}")
    add_custom_command(OUTPUT ${target_header}
      DEPENDS ${fbs_file}
      COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/include/cvibuilder
      COMMAND ${FLATBUFFERS_PATH}/flatc -o ${CMAKE_CURRENT_BINARY_DIR}/include/cvibuilder --cpp ${fbs_file}
    )
    set(targets ${targets} ${target_header})
  endforeach()
  add_custom_target(cvibuilder ALL DEPENDS ${targets})
  install(FILES ${targets} DESTINATION include/cvibuilder/)
endfunction()

compile_fbs(
  ${CMAKE_CURRENT_SOURCE_DIR}/proto/cvimodel.fbs
  ${CMAKE_CURRENT_SOURCE_DIR}/proto/parameter.fbs
)
