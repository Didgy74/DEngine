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

# libktx
set(LIBKTX_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/KTX-Software-master")
target_include_directories(
    ${LIB_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${LIBKTX_FOLDER}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(
    ${LIB_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${LIBKTX_FOLDER}/other_include>
    $<INSTALL_INTERFACE:include>
)

set(LIBKTX_LIB_FOLDER "${LIBKTX_FOLDER}/lib")
set(KTX_SOURCE_FILES 
	"${LIBKTX_LIB_FOLDER}/texture.c" 
	"${LIBKTX_LIB_FOLDER}/filestream.c" 
	"${LIBKTX_LIB_FOLDER}/checkheader.c"
	"${LIBKTX_LIB_FOLDER}/hashlist.c"
	"${LIBKTX_LIB_FOLDER}/memstream.c"
	"${LIBKTX_LIB_FOLDER}/swap.c")
target_sources(${LIB_NAME} INTERFACE ${KTX_SOURCE_FILES})