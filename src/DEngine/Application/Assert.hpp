#pragma once

#include "DEngine/detail/Assert.hpp"

#ifndef DENGINE_APPLICATION_ENABLE_ASSERT

#define DENGINE_DETAIL_APPLICATION_ASSERT(expression) DENGINE_DETAIL_ASSUME(condition)

#define DENGINE_DETAIL_APPLICATION_ASSERT_MSG(condition, msg) DENGINE_DETAIL_ASSUME(condition)

#else

#define DENGINE_DETAIL_APPLICATION_ASSERT(condition) DENGINE_DETAIL_ASSERT(condition)

#define DENGINE_DETAIL_APPLICATION_ASSERT_MSG(condition, msg) DENGINE_DETAIL_ASSERT_MSG(condition, msg)

#endif