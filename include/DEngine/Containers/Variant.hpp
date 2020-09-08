#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Std::impl
{
	template <uSize a, uSize... others>
	struct Max;

	template <uSize arg>
	struct Max<arg>
	{
		static constexpr uSize value = arg;
	};

	template <uSize arg1, uSize arg2, uSize... others>
	struct Max<arg1, arg2, others...>
	{
		static constexpr uSize value = arg1 >= arg2 ? Max<arg1, others...>::value :
			Max<arg2, others...>::value;
	};

	template<uSize... sizes>
	constexpr uSize max = Max<sizes...>::value;
}

namespace DEngine::Std
{
	template<typename... Ts>
	class Variant
	{
	private:
		uSize tracker = static_cast<uSize>(-1);
		alignas(impl::max<alignof(Ts...)>) unsigned char data[impl::max<sizeof(Ts...)>];
	};
}