#pragma once

#undef DENGINE_CONTAINERS_ASSERT

#ifndef DENGINE_CONTAINERS_ENABLE_ASSERT

#define DENGINE_CONTAINERS_ASSERT(expression)

#define DENGINE_CONTAINERS_ASSERT_MSG(condition, msg)

#else

namespace DEngine::Containers
{
	void Assert(const char* conditionString, const char* file, unsigned long long line, const char* msg);
}

#define DENGINE_CONTAINERS_ASSERT(expression) \
{ \
	if (!static_cast<bool>(expression)) \
	{ \
		DEngine::Containers::Assert(#expression,__FILE__, __LINE__, nullptr); \
	} \
} \
	

#define DENGINE_CONTAINERS_ASSERT_MSG(expression, msg) \
{ \
	if (!static_cast<bool>(expression)) \
	{ \
		DEngine::Containers::Assert(#expression,__FILE__, __LINE__, msg); \
	} \
} \

#endif