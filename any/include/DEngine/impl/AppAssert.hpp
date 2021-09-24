#pragma once

#pragma once

#ifdef DENGINE_APPLICATION_ENABLE_ASSERT

#	include <DEngine/detail/Assert.hpp>

#	define DENGINE_IMPL_APPLICATION_ASSERT(expression) DENGINE_DETAIL_ASSERT(expression)

#	define DENGINE_IMPL_APPLICATION_ASSERT_MSG(expression, msg) DENGINE_DETAIL_ASSERT_MSG(expression, msg)

#	define DENGINE_IMPL_APPLICATION_UNREACHABLE() DENGINE_IMPL_UNREACHABLE()

#else

#	include <DEngine/detail/InnerAssert.hpp>

#	define DENGINE_IMPL_APPLICATION_ASSERT(condition) DENGINE_IMPL_INNER_ASSUME(condition)

#	define DENGINE_IMPL_APPLICATION_ASSERT_MSG(condition, msg) DENGINE_IMPL_INNER_ASSUME(condition)

#	define DENGINE_IMPL_APPLICATION_UNREACHABLE() DENGINE_IMPL_INNER_UNREACHABLE()

#endif