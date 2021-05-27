#pragma once

#include <DEngine/Std/Trait.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>

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
		Variant(Variant&& in) noexcept : typeTracker{ in.typeTracker }
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(in.typeTracker != typeTracker_noValue);
			in.moveConstrFn(this->data, in.data);
			moveConstrFn = in.moveConstrFn;
			destrFn = in.destrFn;
			in.Clear();
		}

		template<class T> requires isInPack<T>
		constexpr Variant(T const& in) noexcept : typeTracker{ indexOf<T> }
		{
			new(this->data) T(in);
			SetFunctions<T>();
		}

		~Variant() noexcept { Clear(); }

		Variant& operator=(Variant const&) noexcept = delete;
		Variant& operator=(Variant&&) noexcept = delete;
		
		template<class T> requires isInPack<T>
		Variant& operator=(T const& in) noexcept
		{
			constexpr auto newTypeIndex = indexOf<T>;
			if (newTypeIndex == typeTracker)
			{
				*reinterpret_cast<T*>(this->data) = in;
			}
			else
			{
				Clear();
				new(this->data) T(in);
				SetFunctions<T>();
				typeTracker = newTypeIndex;
			}
			return *this;
		}

		[[nodiscard]] constexpr unsigned int GetIndex() const noexcept
		{
			return typeTracker;
		}

		template<unsigned int i> requires (i < sizeof...(Ts))
		decltype(auto) Get() noexcept
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(typeTracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...>*>(this->data);
		}

		template<unsigned int i> requires (i < sizeof...(Ts))
		decltype(auto) Get() const noexcept
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(typeTracker == i);
			return *reinterpret_cast<Trait::At<i, Ts...> const*>(this->data);
		}

		template<typename T> requires isInPack<T>
		decltype(auto) Get() noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			DENGINE_IMPL_CONTAINERS_ASSERT(typeTracker == i);
			return *reinterpret_cast<T*>(this->data);
		}
		template<typename T> requires isInPack<T>
		decltype(auto) Get() const noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			DENGINE_IMPL_CONTAINERS_ASSERT(typeTracker == i);
			return *reinterpret_cast<T const*>(this->data);
		}
		template<typename T> requires isInPack<T>
		bool IsA() const noexcept
		{
			return typeTracker == Trait::indexOf<T, Ts...>;
		}
		template<typename T> requires isInPack<T>
		T* ToPtr() noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			return typeTracker == i ? reinterpret_cast<T*>(this->data) : nullptr;
		}
		template<typename T> requires isInPack<T>
		T const* ToPtr() const noexcept
		{
			constexpr auto i = Trait::indexOf<T, Ts...>;
			return typeTracker == i ? reinterpret_cast<T const*>(this->data) : nullptr;
		}

	protected:
		alignas(Trait::max<alignof(Ts)...>) unsigned char data[Trait::max<sizeof(Ts)...>];
		static constexpr unsigned int typeTracker_noValue = (unsigned int)-1;
		unsigned int typeTracker;

		using DestructorFn_T = void(*)(void*) noexcept;
		DestructorFn_T destrFn = nullptr;
		using MoveConstrFn_T = void(*)(void*, void*) noexcept;
		MoveConstrFn_T moveConstrFn = nullptr;

		void Clear() noexcept 
		{
			if (typeTracker != typeTracker_noValue)
			{
				DENGINE_IMPL_CONTAINERS_ASSERT(destrFn);
				destrFn(this->data);

				typeTracker = typeTracker_noValue;
				destrFn = nullptr;
				moveConstrFn = nullptr;
			}
		}

		template<typename T>
		constexpr void SetFunctions() noexcept
		{
			destrFn = [](void* data) noexcept { reinterpret_cast<T*>(data)->~T(); };
			moveConstrFn = [](void* data, void* in) noexcept { new(data) T(static_cast<T&&>(*static_cast<T*>(in))); };
		}
	};
}