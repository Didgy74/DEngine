function(DEngine_SetCompilerSettings TARGET)
	target_compile_features(${TARGET} PRIVATE cxx_std_23)
	set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 23)
	set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		target_compile_options(${TARGET} PUBLIC
			$<$<COMPILE_LANGUAGE:CXX>:-Wno-nullability -Wno-nullability-completeness>)
	endif()

	#target_compile_options(dengine_any PUBLIC "-ftime-trace")

endfunction()