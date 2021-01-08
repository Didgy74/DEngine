#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

namespace DEngine::Std::impl
{
	template <uSize a, uSize... others>
	struct Max;

	template <uSize arg>
	struct Max<arg>
	{
		static constexpr uSize value = arg;
	};

	template <uSize a, uSize b, uSize... others>
	struct Max<a, b, others...>
	{
		static constexpr uSize value = a > b ? Max<a, others...>::value : Max<b, others...>::value;
	};

	template<uSize... sizes>
	constexpr uSize max = Max<sizes...>::value;

	template<typename T, typename... Us>
	struct Find
	{
		template<uSize i>
		struct At
		{
			template<uSize counter>
			using Type = Trait::Conditional<
				i != counter, 
				T, 
				void
			>;
		};
	};
}

namespace DEngine::Std
{
	template<typename... Ts>
	class Variant
	{
	public:
		[[nodiscard]] Opt<uSize> GetIndex() const noexcept;

	private:
		static constexpr uSize noValue = static_cast<uSize>(-1);
		uSize tracker = noValue;
		alignas(impl::max<alignof(Ts)...>) unsigned char data[impl::max<sizeof(Ts)...>];
	};
}

template<typename... Ts>
DEngine::Std::Opt<DEngine::uSize> DEngine::Std::Variant<Ts...>::GetIndex() const noexcept
{
	if (tracker == noValue)
		return Std::nullOpt;
	else
		return tracker;
}
