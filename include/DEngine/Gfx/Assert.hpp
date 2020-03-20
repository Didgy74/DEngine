#pragma once

#undef DENGINE_GFX_ASSERT

#ifndef DENGINE_GFX_ENABLE_ASSERT

#define DENGINE_GFX_ASSERT(expression)

#define DENGINE_GFX_ASSERT_MSG(condition, msg)

#else

namespace DEngine::Gfx
{
	void Assert(const char* conditionString, const char* file, unsigned long long line, const char* msg);
}

#define DENGINE_GFX_ASSERT(expression) \
{ \
	if (!static_cast<bool>(expression)) \
	{ \
		DEngine::Gfx::Assert(#expression,__FILE__, __LINE__, nullptr); \
	} \
} \
	

#define DENGINE_GFX_ASSERT_MSG(expression, msg) \
{ \
	if (!static_cast<bool>(expression)) \
	{ \
		DEngine::Gfx::Assert(#expression,__FILE__, __LINE__, msg); \
	} \
} \

#endif