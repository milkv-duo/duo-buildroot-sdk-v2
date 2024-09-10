set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
add_subdirectory(face_capture)
add_subdirectory(face_pet_capture)
add_subdirectory(person_capture)
add_subdirectory(personvehicle_capture)
add_subdirectory(vehicle_adas)
add_subdirectory(face_cap_utils)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/../core/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../sample)

set(FRS_SRCS $<TARGET_OBJECTS:face_capture>
             $<TARGET_OBJECTS:face_pet_capture>
             $<TARGET_OBJECTS:person_capture>
             $<TARGET_OBJECTS:personvehicle_capture>
             $<TARGET_OBJECTS:vehicle_adas>
             $<TARGET_OBJECTS:face_cap_utils>
             cvi_tdl_app.c)

project(cvi_tdl_app)
add_library(${PROJECT_NAME} SHARED ${FRS_SRCS})
target_link_libraries(${PROJECT_NAME} cvi_tdl)
install(TARGETS ${PROJECT_NAME} DESTINATION lib)

project(cvi_tdl_app-static)
add_library(${PROJECT_NAME} STATIC ${FRS_SRCS})
target_link_libraries(${PROJECT_NAME} cvi_tdl)
SET_TARGET_PROPERTIES(cvi_tdl_app-static PROPERTIES OUTPUT_NAME "cvi_tdl_app")

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
