function(DEngine_SetCompilerSettings TARGET)
	target_compile_features(${TARGET} PUBLIC cxx_std_20)
	#target_compile_options(dengine_any PUBLIC "-ftime-trace")

endfunction()