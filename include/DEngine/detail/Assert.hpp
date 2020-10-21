#pragma once


#ifdef _MSC_VER
#  define DENGINE_DETAIL_ASSUME(condition) __assume(condition)
#  define DENGINE_DETAIL_UNREACHABLE() __assume(0)
#elif defined(__clang__)
#  define DENGINE_DETAIL_ASSUME(condition) __builtin_assume(condition)
#  define DENGINE_DETAIL_UNREACHABLE() __builtin_unreachable()
#else
#  define DENGINE_DETAIL_ASSUME(condition) ((condition) ? static_cast<void>(0) : __builtin_unreachable())
#  define DENGINE_DETAIL_UNREACHABLE() __builtin_unreachable()
#endif


#ifndef DENGINE_ENABLE_ASSERT

#define DENGINE_DETAIL_ASSERT(condition) DENGINE_DETAIL_ASSUME(condition)

#define DENGINE_DETAIL_ASSERT_MSG(condition, msg) DENGINE_DETAIL_ASSUME(condition)

#else

namespace DEngine::detail
{
	[[noreturn]] void Assert(
		char const* conditionString,
		char const* file,
		unsigned long long line,
		char const* msg) noexcept;
}

#define DENGINE_DETAIL_ASSERT(condition) \
	(static_cast<bool>(condition) ? \
		static_cast<void>(0) : \
		::DEngine::detail::Assert(#condition, __FILE__, __LINE__, nullptr))
	

#define DENGINE_DETAIL_ASSERT_MSG(condition, msg) \
	(static_cast<bool>(condition) ? \
		static_cast<void>(0) : \
		::DEngine::detail::Assert(#condition, __FILE__, __LINE__, msg))

#endif