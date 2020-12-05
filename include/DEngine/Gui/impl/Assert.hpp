#pragma once

#ifndef DENGINE_GUI_ENABLE_ASSERT

#define DENGINE_IMPL_GUI_ASSERT(expression)

#define DENGINE_IMPL_GUI_ASSERT_MSG(condition, msg)

#else

#include "DEngine/detail/Assert.hpp"

#define DENGINE_IMPL_GUI_ASSERT(expression) DENGINE_DETAIL_ASSERT(expression)


#define DENGINE_IMPL_GUI_ASSERT_MSG(expression, msg) DENGINE_DETAIL_ASSERT_MSG(expression, msg)

#endif