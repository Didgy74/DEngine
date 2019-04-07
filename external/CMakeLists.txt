set(LIB_NAME external)

add_library(${LIB_NAME} INTERFACE)
add_library(${LIB_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})

# fx-gltf
set(FX_GLTF_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/fx-gltf")
target_include_directories(
    ${LIB_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${FX_GLTF_FOLDER}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(
    ${LIB_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${FX_GLTF_FOLDER}/test/thirdparty>
    $<INSTALL_INTERFACE:include>
)

# DMath
add_subdirectory(DMath)
target_link_libraries(${LIB_NAME} INTERFACE DMath::DMath)

# DTex
add_subdirectory(DTex)
target_link_libraries(${LIB_NAME} INTERFACE DTex::DTex)

# Vulkan-headers
add_subdirectory(Vulkan-Headers)
target_link_libraries(${LIB_NAME} INTERFACE Vulkan::Headers)