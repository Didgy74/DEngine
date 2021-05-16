#pragma once

#include <DEngine/Std/Trait.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>

namespace DEngine::Std
{
	template<class ...Ts>
	class Variant
	{
	public:
		template<class T>
		static constexpr bool isInPack = (Trait::isSame<T, Ts> || ...);
		template<class T>
		static constexpr unsigned int indexOf = Trait::indexOf<T, Ts...>;

		constexpr Variant() noexcept = delete;
		template<class T> requires isInPack<T>
		constexpr Variant(T const& in) noexcept : tracker{ indexOf<T> }
		{
			new(data) T(in);
			SetDestructorPfn<T>();
		}
		~Variant() noexcept;

		template<class T> requires isInPack<T>
		Variant& operator=(T const& in) noexcept
		{
			constexpr auto newTypeIndex = indexOf<T>;
			if (newTypeIndex == tracker)
			{
				*reinterpret_cast<T*>(data) = in;
			}
			else
			{
				Clear();
				new(data) T(in);
				SetDestructorPfn<T>();
				tracker = newTypeIndex;
			}
			return *this;
		}

		[[nodiscard]] constexpr unsigned int GetIndex() const noexcept;

		/*
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
		*/

		template<unsigned int i>
		[[nodiscard]] decltype(auto) Get() noexcept
		{
			static_assert(i < sizeof...(Ts), "Tried to Get a Variant with an index out of bounds of the types.");
			DENGINE_IMPL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...>*>(data);
		}

		template<unsigned int i>
		[[nodiscard]] decltype(auto) Get() const noexcept
		{
			static_assert(i < sizeof...(Ts), "Tried to Get a Variant with an index out of bounds of the types.");
			DENGINE_IMPL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...> const*>(data);
		}

		template<typename T>
		[[nodiscard]] decltype(auto) Get() noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			DENGINE_IMPL_CONTAINERS_ASSERT(tracker == i);
			return *reinterpret_cast<T*>(data);
		}
		template<typename T>
		[[nodiscard]] decltype(auto) Get() const noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
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
			constexpr auto i = Trait::indexOf<T, Ts...>;
			return tracker == i ? reinterpret_cast<T*>(data) : nullptr;
		}
		template<typename T>
		[[nodiscard]] T const* ToPtr() const noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			return tracker == i ? reinterpret_cast<T const*>(data) : nullptr;
		}

	private:
		alignas(Trait::max<alignof(Ts)...>) unsigned char data[Trait::max<sizeof(Ts)...>];
		unsigned int tracker;
		using DestructorPFN = void(*)(void*) noexcept;
		DestructorPFN destructorPFN;

		void Clear() noexcept;
		template<typename T>
		constexpr void SetDestructorPfn() noexcept;
	};
}

template<typename... Ts>
DEngine::Std::Variant<Ts...>::~Variant() noexcept
{
	Clear();
}

template<typename... Ts>
constexpr unsigned int DEngine::Std::Variant<Ts...>::GetIndex() const noexcept
{
	return tracker;
}

template<typename... Ts>
void DEngine::Std::Variant<Ts...>::Clear() noexcept
{
	DENGINE_IMPL_CONTAINERS_ASSERT(destructorPFN);
	destructorPFN(data);
}

template<typename ...Ts>
template<typename T>
constexpr void DEngine::Std::Variant<Ts...>::SetDestructorPfn() noexcept
{
	destructorPFN = [](void* data) noexcept 
	{
		reinterpret_cast<T*>(data)->~T(); 
	};
}