#pragma once

#include <DEngine/Std/Trait.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>

// This implements a placement-new operator without having to include <new> or <cstddef>
namespace DEngine::Std::impl { struct VariantPlacementNewTag {}; }
constexpr void* operator new(decltype(sizeof(int)) size, void* ptr, DEngine::Std::impl::VariantPlacementNewTag) noexcept
{
	return ptr; 
}

constexpr void operator delete(void* ptr, DEngine::Std::impl::VariantPlacementNewTag) noexcept {}

namespace DEngine::Std
{
	/*
	template<class T, unsigned int length>
	struct TestArray { T data[length] = {}; };
	using DestructorFn_T = void(*)(void*) noexcept;

	template<class... Ts>
	consteval void Test(TestArray<DestructorFn_T, sizeof...(Ts)>& in);

	template<class T, class... Ts>
	consteval void Test<T, Ts...>(TestArray<DestructorFn_T, sizeof...(Ts)>& in)
	{
		[](void* data) noexcept { reinterpret_cast<T*>(data)->~T(); };
	}

	template<class... Ts>
	[[nodiscard]] consteval TestArray<DestructorFn_T, sizeof...(Ts)> Function(unsigned int index) noexcept
	{
		TestArray<DestructorFn_T, sizeof...(Ts)> var = {};
	}
	*/

	// Using a moved-from Variant is UB.
	template<class ...Ts>
	class Variant
	{
	public:
		template<class T>
		static constexpr bool isInPack = (Trait::isSame<T, Ts> || ...);
		template<class T> requires isInPack<T>
		static constexpr unsigned int indexOf = Trait::indexOf<T, Ts...>;

		constexpr Variant() noexcept = delete;
		Variant(Variant const&) noexcept = delete;
		constexpr Variant(Variant&& in) noexcept;
		template<class T> requires isInPack<T>
		constexpr Variant(T const& in) noexcept : typeTracker{ indexOf<T> }
		{
			new(data, impl::VariantPlacementNewTag{}) T(in);
			SetFunctions<T>();
		}
		template<class T> requires isInPack<T>
		constexpr Variant(T&& in) noexcept : typeTracker{ indexOf<T> }
		{
			new(data, impl::VariantPlacementNewTag{}) T(static_cast<T&&>(in));
			SetFunctions<T>();
		}

		~Variant() noexcept;

		Variant& operator=(Variant const&) noexcept = delete;
		Variant& operator=(Variant&&) noexcept;
		
		template<class T> requires isInPack<T>
		Variant& operator=(T const& in) noexcept
		{
			constexpr auto newTypeIndex = indexOf<T>;
			if (newTypeIndex == typeTracker)
			{
				(*reinterpret_cast<T*>(data)) = in;
			}
			else
			{
				Clear();
				new(data, impl::VariantPlacementNewTag{}) T(in);
				SetFunctions<T>();
				typeTracker = newTypeIndex;
			}
			return *this;
		}

		[[nodiscard]] constexpr unsigned int GetIndex() const noexcept;

		template<unsigned int i> requires (i < sizeof...(Ts))
		decltype(auto) Get() noexcept
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(typeTracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...>*>(data);
		}

		template<unsigned int i> requires (i < sizeof...(Ts))
		decltype(auto) Get() const noexcept
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(typeTracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...> const*>(data);
		}

		template<typename T> requires isInPack<T>
		decltype(auto) Get() noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			DENGINE_IMPL_CONTAINERS_ASSERT(typeTracker == i);
			return *reinterpret_cast<T*>(data);
		}
		template<typename T> requires isInPack<T>
		decltype(auto) Get() const noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			DENGINE_IMPL_CONTAINERS_ASSERT(typeTracker == i);
			return *reinterpret_cast<T const*>(data);
		}
		template<typename T> requires isInPack<T>
		[[nodiscard]] bool IsA() const noexcept
		{
			return typeTracker == Trait::indexOf<T, Ts...>;
		}
		template<typename T> requires isInPack<T>
		[[nodiscard]] T* ToPtr() noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			return typeTracker == i ? reinterpret_cast<T*>(data) : nullptr;
		}
		template<typename T> requires isInPack<T>
		[[nodiscard]] T const* ToPtr() const noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			return typeTracker == i ? reinterpret_cast<T const*>(data) : nullptr;
		}

	protected:
		alignas(Trait::max<alignof(Ts)...>) unsigned char data[Trait::max<sizeof(Ts)...>];
		static constexpr unsigned int typeTracker_noValue = (unsigned int)-1;
		unsigned int typeTracker;

		using DestructorFn_T = void(*)(void*) noexcept;
		DestructorFn_T destrFn = nullptr;
		using MoveConstrFn_T = void(*)(void*, void*) noexcept;
		MoveConstrFn_T moveConstrFn = nullptr;
		using MoveAssignFn_T = void(*)(void*, void*) noexcept;
		MoveConstrFn_T moveAssignFn = nullptr;

		void Clear() noexcept;

		template<typename T>
		constexpr void SetFunctions() noexcept
		{
			destrFn = [](void* data) noexcept { reinterpret_cast<T*>(data)->~T(); };
			moveConstrFn = [](void* data, void* other) noexcept { new(data, impl::VariantPlacementNewTag{}) T(static_cast<T&&>(*static_cast<T*>(other))); };
			moveAssignFn = [](void* data, void* other) noexcept { *static_cast<T*>(data) = static_cast<T&&>(*static_cast<T*>(other)); };
		}
	};

	template<class ...Ts>
	constexpr Variant<Ts...>::Variant(Variant&& in) noexcept : typeTracker{ in.typeTracker }
	{
		DENGINE_IMPL_CONTAINERS_ASSERT(in.typeTracker != typeTracker_noValue);
		in.moveConstrFn(this->data, in.data);
		destrFn = in.destrFn;
		moveConstrFn = in.moveConstrFn;
		moveAssignFn = in.moveAssignFn;
		in.Clear();
	}

	template<class ...Ts>
	Variant<Ts...>::~Variant() noexcept { Clear(); }

	template<class ...Ts>
	Variant<Ts...>& Variant<Ts...>::operator=(Variant&& other) noexcept
	{
		DENGINE_IMPL_CONTAINERS_ASSERT(other.typeTracker != typeTracker_noValue);

		if (other.typeTracker == typeTracker)
		{
			moveAssignFn(data, &other);
		}
		else
		{
			Clear();

			destrFn = other.destrFn;
			moveConstrFn = other.moveConstrFn;
			moveAssignFn = other.moveAssignFn;

			moveConstrFn(data, &other);
			typeTracker = other.typeTracker;
		}

		other.Clear();
		return *this;
	}

	template<class ...Ts>
	constexpr unsigned int Variant<Ts...>::GetIndex() const noexcept
	{
		return typeTracker;
	}

	template<class ...Ts>
	void Variant<Ts...>::Clear() noexcept
	{
		if (typeTracker != typeTracker_noValue)
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(destrFn);
			destrFn(data);

			typeTracker = typeTracker_noValue;
			destrFn = nullptr;
			moveConstrFn = nullptr;
			moveAssignFn = nullptr;
		}
	}
}