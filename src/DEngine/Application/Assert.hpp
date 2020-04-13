#pragma once

#ifndef DENGINE_APPLICATION_ENABLE_ASSERT

#define DENGINE_DETAIL_APPLICATION_ASSERT(expression)

#define DENGINE_DETAIL_APPLICATION_ASSERT_MSG(condition, msg)

#else

#include "DEngine/detail/Assert.hpp"

#define DENGINE_DETAIL_APPLICATION_ASSERT(expression) DENGINE_DETAIL_ASSERT(expression)


#define DENGINE_DETAIL_APPLICATION_ASSERT_MSG(expression, msg) DENGINE_DETAIL_ASSERT_MSG(expression, msg)

#endif