#pragma once

#ifndef DENGINE_ENABLE_ASSERT

#define DENGINE_DETAIL_ASSERT(expression) (void*)0

#define DENGINE_DETAIL_ASSERT_MSG(condition, msg) (void*)0

#else

namespace DEngine::detail
{
	[[noreturn]] void Assert(
		char const* conditionString,
		char const* file,
		unsigned long long line,
		char const* msg);
}

#define DENGINE_DETAIL_ASSERT(expression) \
	if (!static_cast<bool>(expression)) \
		::DEngine::detail::Assert(#expression, __FILE__, __LINE__, nullptr)
	

#define DENGINE_DETAIL_ASSERT_MSG(expression, msg) \
	if (!static_cast<bool>(expression))	\
		::DEngine::detail::Assert(#expression,__FILE__, __LINE__, msg)

#endif