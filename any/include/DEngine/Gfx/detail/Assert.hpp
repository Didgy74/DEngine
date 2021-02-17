#pragma once

#ifndef DENGINE_GFX_ENABLE_ASSERT

#define DENGINE_DETAIL_GFX_ASSERT(expression)

#define DENGINE_DETAIL_GFX_ASSERT_MSG(condition, msg)

#else

#include "DEngine/detail/Assert.hpp"

#define DENGINE_DETAIL_GFX_ASSERT(expression) DENGINE_DETAIL_ASSERT(expression)


#define DENGINE_DETAIL_GFX_ASSERT_MSG(expression, msg) DENGINE_DETAIL_ASSERT_MSG(expression, msg)

#endif