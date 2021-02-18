#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

#include <new>

namespace DEngine::Std
{
	enum class NullVariantT : char;
	constexpr NullVariantT nullVariant = NullVariantT();

	template<typename... Ts>
	class Variant
	{
	public:
		template<typename T>
		static constexpr unsigned int indexOf = Trait::indexOf<T, Ts...>;

		inline ~Variant() noexcept;

		[[nodiscard]] Opt<unsigned int> GetIndex() const noexcept;

		void Set(NullVariantT) noexcept
		{
			Clear();
			tracker = noValue;
		}

		template<typename T>
		void Set(T const& in) noexcept
		{
			constexpr unsigned int newTypeIndex = Trait::indexOf<T, Ts...>;
			if (tracker != noValue)
				Clear();
			if (newTypeIndex != tracker)
			{
				new(data) T(in);
			}
			else
			{
				*reinterpret_cast<T*>(data) = in;
			}

			tracker = newTypeIndex;
		}


		template<typename T>
		void Set(T&& in) noexcept
		{
			constexpr unsigned int newTypeIndex = Trait::indexOf<T, Ts...>;
			if (tracker != noValue)
				Clear();
			if (newTypeIndex != tracker)
			{
				new(data) T(static_cast<T&&>(in));
			}
			else
			{
				*reinterpret_cast<T*>(data) = static_cast<T&&>(in);
			}

			tracker = newTypeIndex;
		}

		template<unsigned int i>
		[[nodiscard]] auto Get() noexcept -> decltype(auto)
		{
			static_assert(i < sizeof...(Ts), "Tried to Get a Variant with an index out of bounds of the types.");
			DENGINE_DETAIL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...>*>(data);
		}

		template<unsigned int i>
		[[nodiscard]] auto Get() const noexcept -> decltype(auto)
		{
			static_assert(i < sizeof...(Ts), "Tried to Get a Variant with an index out of bounds of the types.");
			DENGINE_DETAIL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...> const*>(data);
		}

		template<typename T>
		[[nodiscard]] auto Get() noexcept -> decltype(auto)
		{
			constexpr unsigned int i = Trait::indexOf<T, Ts...>;
			DENGINE_DETAIL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<T*>(data);
		}
		template<typename T>
		[[nodiscard]] auto Get() const noexcept -> decltype(auto)
		{
			constexpr unsigned int i = Trait::indexOf<T, Ts...>;
			DENGINE_DETAIL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<T const*>(data);
		}
		template<typename T>
		[[nodiscard]] bool IsA() const noexcept
		{
			return tracker == Trait::indexOf<T, Ts...>;
		}
		template<typename T>
		[[nodiscard]] T* ToPtr() noexcept
		{
			constexpr unsigned int i = Trait::indexOf<T, Ts...>;
			return tracker == i ? reinterpret_cast<T*>(data) : nullptr;
		}
		template<typename T>
		[[nodiscard]] T const* ToPtr() const noexcept
		{
			constexpr unsigned int i = Trait::indexOf<T, Ts...>;
			return tracker == i ? reinterpret_cast<T const*>(data) : nullptr;
		}

	private:
		static constexpr unsigned int noValue = static_cast<unsigned int>(-1);
		unsigned int tracker = noValue;
		alignas(Trait::max<alignof(Ts)...>) unsigned char data[Trait::max<sizeof(Ts)...>] = {};

		inline void Clear() noexcept;
	};
}

template<typename... Ts>
inline DEngine::Std::Variant<Ts...>::~Variant() noexcept
{
	Clear();
}

template<typename... Ts>
inline DEngine::Std::Opt<unsigned int> DEngine::Std::Variant<Ts...>::GetIndex() const noexcept
{
	if (tracker == noValue)
		return Std::nullOpt;
	else
		return tracker;
}

template<typename... Ts>
void DEngine::Std::Variant<Ts...>::Clear() noexcept
{
	static_assert(sizeof...(Ts) <= 6, "Variant::Clear does not support more than 6 types.");

	if (tracker == noValue)
		return;

	using TriviallyDesctructibleT = char;
	switch (tracker)
	{
	case 0:
	{
		using T = Trait::AtOr<0, TriviallyDesctructibleT, Ts...>;
		reinterpret_cast<T*>(data)->~T();
	}
	break;
	case 1:
	{
		using T = Trait::AtOr<1, TriviallyDesctructibleT, Ts...>;
		reinterpret_cast<T*>(data)->~T();
	}
	break;
	case 2:
	{
		using T = Trait::AtOr<2, TriviallyDesctructibleT, Ts...>;
		reinterpret_cast<T*>(data)->~T();
	}
	break;
	case 3:
	{
		using T = Trait::AtOr<3, TriviallyDesctructibleT, Ts...>;
		reinterpret_cast<T*>(data)->~T();
	}
	break;
	case 4:
	{
		using T = Trait::AtOr<4, TriviallyDesctructibleT, Ts...>;
		reinterpret_cast<T*>(data)->~T();
	}
	break;
	case 5:
	{
		using T = Trait::AtOr<5, TriviallyDesctructibleT, Ts...>;
		reinterpret_cast<T*>(data)->~T();
	}
	break;
	default:
		DENGINE_DETAIL_UNREACHABLE();
		break;
	}
}