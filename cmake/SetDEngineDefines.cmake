# First we need to make a few enums
set(DENGINE_OS_WINDOWS "0")
set(DENGINE_OS_LINUX "1")
set(DENGINE_OS_ANDROID "2")

function(DEngine_SetDefines TARGET)

	add_compile_definitions(${TARGET} PUBLIC DENGINE_OS_VALUE_WINDOWS=${DENGINE_OS_WINDOWS})
	add_compile_definitions(${TARGET} PUBLIC DENGINE_OS_VALUE_LINUX=${DENGINE_OS_LINUX})
	add_compile_definitions(${TARGET} PUBLIC DENGINE_OS_VALUE_ANDROID=${DENGINE_OS_ANDROID})
	if (NOT DEFINED DENGINE_OS)
		message(FATAL_ERROR "Need to set 'DENGINE_OS'")
	endif()
	add_compile_definitions(${TARGET} PUBLIC DENGINE_OS=${DENGINE_OS})

endfunction()