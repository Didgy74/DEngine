#pragma once

#include <DEngine/impl/InnerAssert.hpp>

#ifdef DENGINE_ENABLE_ASSERT
namespace DEngine::impl
{
	[[noreturn]] void Assert(
		char const* conditionString,
		char const* file,
		unsigned long long line,
		char const* msg) noexcept;
}
#endif

// Define Unreachable
#ifdef DENGINE_ENABLE_ASSERT

#	define DENGINE_IMPL_UNREACHABLE() \
		::DEngine::impl::Assert( \
			nullptr, \
			__FILE__, \
			__LINE__, \
			"Hit an unreachable.")

#else

#	define DENGINE_IMPL_UNREACHABLE() DENGINE_IMPL_INNER_UNREACHABLE()

#endif

// Define Assert and Assert_Msg
#ifdef DENGINE_ENABLE_ASSERT
# 	define DENGINE_IMPL_ASSERT(condition) \
		(static_cast<bool>(condition) ? \
			static_cast<void>(0) : \
			::DEngine::impl::Assert(#condition, __FILE__, __LINE__, nullptr))
#	define DENGINE_IMPL_ASSERT_MSG(condition, msg) \
		(static_cast<bool>(condition) ? \
			static_cast<void>(0) : \
			::DEngine::impl::Assert(#condition, __FILE__, __LINE__, msg))

#else

#	define DENGINE_IMPL_ASSERT(condition) DENGINE_IMPL_INNER_ASSUME(condition)
#	define DENGINE_IMPL_ASSERT_MSG(condition, msg) DENGINE_IMPL_INNER_ASSUME(condition)

#endif