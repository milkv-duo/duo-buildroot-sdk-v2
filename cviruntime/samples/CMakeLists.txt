cmake_minimum_required(VERSION 2.8.0)

project(cvitek_samples C CXX)

add_subdirectory(utils)
add_subdirectory(runner)
add_subdirectory(classifier)
add_subdirectory(classifier_bf16)
add_subdirectory(classifier_fused_preprocess)
add_subdirectory(classifier_multi_batch)
add_subdirectory(samples_extra)

set(SCRIPT_FILES
    run_classifier.sh
    run_classifier_bf16.sh
    run_classifier_fused_preprocess.sh
    run_classifier_multi_batch.sh
    )

install(FILES ${SCRIPT_FILES}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
    DESTINATION .)

install(DIRECTORY data/ DESTINATION data FILES_MATCHING PATTERN "*")