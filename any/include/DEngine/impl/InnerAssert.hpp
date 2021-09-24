#pragma once

// Define unreachable
// Don't use directly
#ifdef _MSC_VER
#	define DENGINE_IMPL_INNER_UNREACHABLE() __assume(0)
#else
#	define DENGINE_IMPL_INNER_UNREACHABLE() __builtin_unreachable()
#endif

// Define assume
// Don't use directly
#ifdef _MSC_BUILD
#	define DENGINE_IMPL_INNER_ASSUME(condition) __assume(condition)
#else
#	define DENGINE_IMPL_INNER_ASSUME(condition) ((condition) ? static_cast<void>(0) : DENGINE_IMPL_INNER_UNREACHABLE())
#endif