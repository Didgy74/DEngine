# This function sets up the Rust prebuild step that compiles and copies folders correctly into
# mandatory OUTPUT_DIR argument.
function(dengine_setup_asset_prebuild_step target)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs)

    cmake_parse_arguments(ARGS
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if(NOT ARGS_OUTPUT_DIR)
        message(FATAL_ERROR "Called dengine_setup_asset_prebuild_step without any OUTPUT_DIR")
    endif()

    # Set up asset_builder
    # Find Cargo
    find_program(CARGO_COMMAND cargo)
    if (NOT CARGO_COMMAND)
        message(FATAL_ERROR "Couldn't find cargo executable. Make sure Rust is installed.")
    endif()
    find_program(RUSTC_COMMAND rustc)
    if (NOT RUSTC_COMMAND)
        message(FATAL_ERROR "Couldn't find rustc executable.")
    endif()
    find_program(SLANGC_COMMAND slangc)
    if (NOT SLANGC_COMMAND)
        message(FATAL_ERROR "Couldn't find Slang shader compiler. Try installing Vulkan SDK.")
    endif()
    file(GLOB_RECURSE ALL_ASSET_FILES
        CONFIGURE_DEPENDS
        "${DENGINE_ROOT_DIR}/assets/*")
    add_custom_command(
        OUTPUT "${ARGS_OUTPUT_DIR}/.last_build_info.txt"
        COMMENT "Rust: Running asset_builder"
        WORKING_DIRECTORY ${DENGINE_ROOT_DIR}/asset_builder
        COMMAND
            ${CMAKE_COMMAND} -E env
            RUSTC=${RUSTC_COMMAND}
            ${CARGO_COMMAND} run
            --release
            --target-dir ${CMAKE_BINARY_DIR}/asset_builder
            --
            --input "${DENGINE_ROOT_DIR}/assets"
            --output "${ARGS_OUTPUT_DIR}"
            --slangc "${SLANGC_COMMAND}"
        DEPENDS
            ${ALL_ASSET_FILES}
    )

    add_custom_target(run_asset_builder_${target} ALL
        DEPENDS "${ARGS_OUTPUT_DIR}/.last_build_info.txt"
    )
    add_dependencies(${target} run_asset_builder_${target})
endfunction()