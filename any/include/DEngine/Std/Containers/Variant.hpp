#pragma once

#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>

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

		~Variant() noexcept;

		[[nodiscard]] Opt<unsigned int> GetIndex() const noexcept;

		void Set(NullVariantT) noexcept
		{
			Clear();
			tracker = noValue;
			destructorPFN = nullptr;
		}

		template<typename T>
		void Set(T const& in) noexcept
		{
			constexpr auto newTypeIndex = Trait::indexOf<T, Ts...>;
			if (newTypeIndex != tracker)
			{
				Clear();
				new(data) T(in);
			}
			else
			{
				*reinterpret_cast<T*>(data) = in;
			}
			SetDestructorPfn<T>();

			tracker = newTypeIndex;
		}

		template<typename T>
		void Set(Trait::RemoveCVRef<T>&& in) noexcept
		{
			using Type = typename Trait::RemoveCVRef<T>;
			constexpr auto newTypeIndex = Trait::indexOf<Type, Ts...>;
			if (newTypeIndex != tracker)
			{
				Clear();
				new(data) Type(static_cast<Type&&>(in));
			}
			else
			{
				*reinterpret_cast<Type*>(data) = static_cast<Type&&>(in);
			}
			SetDestructorPfn<T>();

			tracker = newTypeIndex;
		}

		template<unsigned int i>
		[[nodiscard]] auto Get() noexcept -> decltype(auto)
		{
			static_assert(i < sizeof...(Ts), "Tried to Get a Variant with an index out of bounds of the types.");
			DENGINE_IMPL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...>*>(data);
		}

		template<unsigned int i>
		[[nodiscard]] auto Get() const noexcept -> decltype(auto)
		{
			static_assert(i < sizeof...(Ts), "Tried to Get a Variant with an index out of bounds of the types.");
			DENGINE_IMPL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...> const*>(data);
		}

		template<typename T>
		[[nodiscard]] auto Get() noexcept -> decltype(auto)
		{
			constexpr unsigned int i = Trait::indexOf<T, Ts...>;
			DENGINE_IMPL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<T*>(data);
		}
		template<typename T>
		[[nodiscard]] auto Get() const noexcept -> decltype(auto)
		{
			constexpr unsigned int i = Trait::indexOf<T, Ts...>;
			DENGINE_IMPL_CONTAINERS_ASSERT(tracker == i);
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
		alignas(Trait::max<alignof(Ts)...>) unsigned char data[Trait::max<sizeof(Ts)...>];
		static constexpr unsigned int noValue = static_cast<unsigned int>(-1);
		unsigned int tracker = noValue;
		using DestructorPFN = void(*)(void* data) noexcept;
		DestructorPFN destructorPFN = nullptr;

		void Clear() noexcept;
		template<typename U>
		void SetDestructorPfn() noexcept;
	};
}

template<typename... Ts>
DEngine::Std::Variant<Ts...>::~Variant() noexcept
{
	Clear();
}

template<typename... Ts>
DEngine::Std::Opt<unsigned int> DEngine::Std::Variant<Ts...>::GetIndex() const noexcept
{
	if (tracker == noValue)
		return Std::nullOpt;
	else
		return tracker;
}

template<typename... Ts>
void DEngine::Std::Variant<Ts...>::Clear() noexcept
{
	if (tracker == noValue)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT(!destructorPFN);
		return;
	}
	
	DENGINE_IMPL_CONTAINERS_ASSERT(destructorPFN);
	destructorPFN(data);
}

template<typename... Ts>
template<typename U>
void DEngine::Std::Variant<Ts...>::SetDestructorPfn() noexcept
{
	destructorPFN = [](void* data) noexcept 
	{ 
		reinterpret_cast<U*>(data)->~U(); 
	};
}