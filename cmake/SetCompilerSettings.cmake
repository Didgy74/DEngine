function(DEngine_SetCompilerSettings TARGET)
	target_compile_features(${TARGET} PUBLIC cxx_std_23)
	set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 23)
	set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)

	#target_compile_options(dengine_any PUBLIC "-ftime-trace")

endfunction()