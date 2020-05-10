#pragma once

#ifndef DENGINE_CONTAINERS_ENABLE_ASSERT

#define DENGINE_DETAIL_CONTAINERS_ASSERT(expression)

#define DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(condition, msg)

#else

#include "DEngine/detail/Assert.hpp"

#define DENGINE_DETAIL_CONTAINERS_ASSERT(expression) DENGINE_DETAIL_ASSERT(expression)
	

#define DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(expression, msg) DENGINE_DETAIL_ASSERT_MSG(expression, msg)

#endif