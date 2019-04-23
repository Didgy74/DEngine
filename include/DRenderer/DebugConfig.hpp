#pragma once

namespace DRenderer
{
#ifdef NDEBUG
	constexpr bool debugConfig = false;
#else
	constexpr bool debugConfig = true;
#endif
}