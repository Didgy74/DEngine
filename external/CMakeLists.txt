set(LIB_NAME external)

add_library(${LIB_NAME} INTERFACE)
add_library(${LIB_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(
    ${LIB_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/fx-gltf/include>
    $<INSTALL_INTERFACE:include>
)

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(nlohmann-json)
target_link_libraries(${LIB_NAME} INTERFACE nlohmann_json::nlohmann_json)

add_subdirectory(DMath)
target_link_libraries(${LIB_NAME} INTERFACE DMath::DMath)