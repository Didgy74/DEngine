#pragma once

#define DENGINE_STD_OS_WINDOWS_VALUE 0
#define DENGINE_STD_OS_LINUX_VALUE 1
#define DENGINE_STD_OS_ANDROID_VALUE 2

#if defined(_WIN32) || defined(_WIN64)
#	define DENGINE_STD_OS DENGINE_STD_OS_WINDOWS_VALUE
#elif defined(__ANDROID__)
#	define DENGINE_OS_ANDROID
#elif defined(__GNUC__)
#	define DENGINE_OS_LINUX
#else
#	error Error. DEngine does not support this platform/compiler
#endif


#define DENGINE_STD_COMPILER_MSVC_VALUE 0
#define DENGINE_STD_COMPILER_GCC_VALUE 1
#if defined(_MSC_VER)

#else
#   define DENGINE_STD_COMPILER DENGINE_STD_COMPILER_GCC_VALUE
#endif

namespace DEngine::Std {
	enum OS {
		Windows = DENGINE_STD_OS_WINDOWS_VALUE,
		Linux = DENGINE_STD_OS_LINUX_VALUE,
		Android = DENGINE_STD_OS_ANDROID_VALUE,
	};

	enum Compiler {
		MSVC = DENGINE_STD_COMPILER_MSVC_VALUE,
		GCC = DENGINE_STD_COMPILER_GCC_VALUE,
	};
}