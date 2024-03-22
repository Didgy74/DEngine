#pragma once

#ifdef DENGINE_GFX_ENABLE_ASSERT

#	include <DEngine/impl/Assert.hpp>
#	define DENGINE_IMPL_GFX_ASSERT(condition) DENGINE_IMPL_ASSERT(condition)
#	define DENGINE_IMPL_GFX_ASSERT_MSG(condition, msg) DENGINE_IMPL_ASSERT_MSG(condition, msg)
#	define DENGINE_IMPL_GFX_UNREACHABLE() DENGINE_IMPL_UNREACHABLE()

#else

#	include <DEngine/impl/InnerAssert.hpp>
#	define DENGINE_IMPL_GFX_ASSERT(condition) DENGINE_IMPL_INNER_ASSUME(condition)
#	define DENGINE_IMPL_GFX_ASSERT_MSG(condition, msg) DENGINE_IMPL_INNER_ASSUME(condition)
#	define DENGINE_IMPL_GFX_UNREACHABLE() DENGINE_IMPL_INNER_UNREACHABLE()

#endif