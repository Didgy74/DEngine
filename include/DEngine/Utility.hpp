#pragma once

namespace DEngine::Utility
{
	template<typename T>
	inline T&& Move(T&& in) noexcept
	{
		return static_cast<T&&>(in);
	}

	template<typename T>
	inline T&& Move(T& in) noexcept
	{
		return static_cast<T&&>(in);
	}
}

namespace DEngine
{
	namespace Util = Utility;
}