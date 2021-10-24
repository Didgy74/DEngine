function(SetupAssertDefines TARGET VALUE)

	option(DENGINE_ENABLE_ASSERT "Disabling will remove all asserts in engine" ${VALUE})
	if (${DENGINE_ENABLE_ASSERT})
		target_compile_definitions(${TARGET} PUBLIC DENGINE_ENABLE_ASSERT)
	endif()

	option(DENGINE_APPLICATION_ENABLE_ASSERT "Asserts inside the application code" ${VALUE})
	if (${DENGINE_APPLICATION_ENABLE_ASSERT})
		target_compile_definitions(${TARGET} PUBLIC DENGINE_APPLICATION_ENABLE_ASSERT)
	endif()

	option(DENGINE_CONTAINERS_ENABLE_ASSERT "Asserts inside DEngine containers" ${VALUE})
	if (${DENGINE_CONTAINERS_ENABLE_ASSERT})
		target_compile_definitions(${TARGET} PUBLIC DENGINE_CONTAINERS_ENABLE_ASSERT)
	endif()

	option(DENGINE_GFX_ENABLE_ASSERT "Asserts inside the rendering code" ${VALUE})
	if (${DENGINE_GFX_ENABLE_ASSERT})
		target_compile_definitions(${TARGET} PUBLIC DENGINE_GFX_ENABLE_ASSERT)
	endif()

	option(DENGINE_GUI_ENABLE_ASSERT "Asserts inside the GUI toolkit code" ${VALUE})
	if (${DENGINE_GUI_ENABLE_ASSERT})
		target_compile_definitions(${TARGET} PUBLIC DENGINE_GUI_ENABLE_ASSERT)
	endif()

	option(DENGINE_MATH_ENABLE_ASSERT "Asserts inside DEngine math" ${VALUE})
	if (${DENGINE_MATH_ENABLE_ASSERT})
		target_compile_definitions(${TARGET} PUBLIC DENGINE_MATH_ENABLE_ASSERT)
	endif()

endfunction()