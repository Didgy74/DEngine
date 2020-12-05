#pragma once

#ifdef DENGINE_ENABLE_ASSERT
namespace DEngine::detail
{
	[[noreturn]] void Assert(
		char const* conditionString,
		char const* file,
		unsigned long long line,
		char const* msg) noexcept;
}
#endif

#ifdef DENGINE_ENABLE_ASSERT

#  define DENGINE_DETAIL_UNREACHABLE() ::DEngine::detail::Assert("Hit an unreachable.", __FILE__, __LINE__, "Hit an unreachable.")

#elif

#	ifdef _MSC_VER
#  define DENGINE_DETAIL_UNREACHABLE() __assume(0)
# elif defined(__clang__)
#  define DENGINE_DETAIL_UNREACHABLE() __builtin_unreachable()
# else
#  define DENGINE_DETAIL_UNREACHABLE() __builtin_unreachable()
# endif

#endif


#ifdef DENGINE_ENABLE_ASSERT

#define DENGINE_DETAIL_ASSERT(condition) \
	(static_cast<bool>(condition) ? \
		static_cast<void>(0) : \
		::DEngine::detail::Assert(#condition, __FILE__, __LINE__, nullptr))

#define DENGINE_DETAIL_ASSERT_MSG(condition, msg) \
	(static_cast<bool>(condition) ? \
		static_cast<void>(0) : \
		::DEngine::detail::Assert(#condition, __FILE__, __LINE__, msg))

#else

#	ifdef _MSC_VER
#	 define DENGINE_DETAIL_ASSERT(condition) __assume(condition)
#	 define DENGINE_DETAIL_ASSERT_MSG(condition) __assume(condition)
# elif defined(__clang__)
#  define DENGINE_DETAIL_ASSERT(condition) __builtin_assume(condition)
#  define DENGINE_DETAIL_ASSERT_MSG(condition) __builtin_assume(condition)
# else
#  define DENGINE_DETAIL_ASSERT(condition) ((condition) ? static_cast<void>(0) : __builtin_unreachable())
#  define DENGINE_DETAIL_ASSERT_MSG(condition) ((condition) ? static_cast<void>(0) : __builtin_unreachable())
# endif

#endif