#pragma once

#include <DEngine/Std/Containers/impl/Assert.hpp>
#include <DEngine/Std/Trait.hpp>

namespace DEngine::Std
{
	template<class T>
	class Box
	{
	public:
		template<class U>
		friend class Box;

		using ValueType = T;

		constexpr Box() noexcept;
		constexpr Box(decltype(nullptr)) noexcept;
		Box(Box const&) = delete;
		constexpr Box(Box&&) noexcept;
		template<class U> requires Trait::isBaseOf<T, U>
		constexpr Box(Box<U>&& in) noexcept : data(in.data)
		{
			in.data = nullptr;
		}
		explicit Box(T* ptr) noexcept;

		~Box();

		Box& operator=(Box const&) = delete;
		Box& operator=(Box&&) noexcept;
		template<class U> requires Trait::isBaseOf<T, U>
		constexpr Box& operator=(Box<U>&& in) noexcept
		{
			Clear();
			data = in.data;
			in.data = nullptr;

			return *this;
		}
		Box& operator=(decltype(nullptr)) noexcept;


		[[nodiscard]] T* Get() noexcept;
		[[nodiscard]] T const* Get() const noexcept;
		[[nodiscard]] T* operator->() noexcept;
		[[nodiscard]] T const* operator->() const noexcept;
		[[nodiscard]] T& operator*() noexcept;
		[[nodiscard]] T const& operator*() const noexcept;

		[[nodiscard]] explicit operator bool() const noexcept;

		[[nodiscard]] bool operator==(Box const&) const noexcept;
		[[nodiscard]] bool operator==(T const*) const noexcept;

		// Returns the pointer and releases ownership over it.
		[[nodiscard]] T* Release() noexcept;
		// Destroys the internals if it holds any.
		void Clear() noexcept;

	protected:
		
		T* data = nullptr;
	};

	template<typename T>
	constexpr Box<T>::Box() noexcept :
		data(nullptr)
	{
	}

	template<typename T>
	constexpr Box<T>::Box(decltype(nullptr)) noexcept : data(nullptr)
	{

	}

	template<typename T>
	constexpr Box<T>::Box(Box&& other) noexcept :
		data(other.data)
	{
		other.data = nullptr;
	}

	template<typename T>
	inline Box<T>::Box(T* ptr) noexcept :
		data(ptr)
	{
	}

	template<typename T>
	inline Box<T>::~Box()
	{
		Clear();
	}

	template<typename T>
	inline Box<T>& Box<T>::operator=(
		Box&& right) noexcept
	{
		Clear();
		data = right.data;
		right.data = nullptr;

		return *this;
	}

	template<typename T>
	inline Box<T>& Box<T>::operator=(decltype(nullptr)) noexcept
	{
		Clear();
		data = nullptr;
		return *this;
	}

	template<typename T>
	inline T* Box<T>::Get() noexcept
	{
		return data;
	}

	template<typename T>
	inline T const* Box<T>::Get() const noexcept
	{
		return data;
	}

	template<typename T>
	inline T* Box<T>::operator->() noexcept
	{
		return data;
	}

	template<typename T>
	inline T const* Box<T>::operator->() const noexcept
	{
		return data;
	}

	template<typename T>
	inline T& Box<T>::operator*() noexcept
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			data != nullptr,
			"Attempted to dereference a Std::Box with a nullptr value.");
		return *data;
	}

	template<typename T>
	inline T const& Box<T>::operator*() const noexcept
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			data != nullptr,
			"Attempted to dereference a Std::Box with a nullptr value.");
		return *data;
	}

	template<typename T>
	inline Box<T>::operator bool() const noexcept
	{
		return data != nullptr;
	}

	template<typename T>
	inline bool Box<T>::operator==(T const* other) const noexcept
	{
		return data == other;
	}

	template<typename T>
	inline T* Box<T>::Release() noexcept
	{
		T* returnVal = data;
		data = nullptr;
		return returnVal;
	}

	template<typename T>
	inline void Box<T>::Clear() noexcept
	{
		if (data)
		{
			delete data;
			data = nullptr;
		}
	}
}