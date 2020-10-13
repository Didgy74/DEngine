#pragma once

#include <DEngine/Std/Containers/detail/Assert.hpp>

namespace DEngine::Std
{
	template<typename T>
	class Box
	{
	public:
		using ValueType = T;

		Box() noexcept;
		Box(Box const&) = delete;
		Box(Box&&) noexcept;
		explicit Box(T* ptr) noexcept;
		~Box();

		Box& operator=(Box const&) = delete;
		Box& operator=(Box&&) noexcept;

		[[nodiscard]] T* Get() noexcept;
		[[nodiscard]] T const* Get() const noexcept;
		[[nodiscard]] T* operator->() noexcept;
		[[nodiscard]] T const* operator->() const noexcept;
		[[nodiscard]] T& operator*() const noexcept;

		[[nodiscard]] operator bool() const noexcept;

		[[nodiscard]] bool operator==(Box const&) const noexcept;
		[[nodiscard]] bool operator==(T const*) const noexcept;

		void Release() noexcept;

	private:
		T* data = nullptr;
	};

	template<typename T>
	inline Box<T>::Box() noexcept :
		data(nullptr)
	{
	}

	template<typename T>
	inline Box<T>::Box(Box&& other) noexcept :
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
		Release();
	}

	template<typename T>
	inline Box<T>& Box<T>::operator=(Box&& right) noexcept
	{
		if (this == &right)
			return *this;
		
		Release();
		data = right.data;
		right.data = nullptr;
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
	inline T& Box<T>::operator*() const noexcept
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
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
	inline void Box<T>::Release() noexcept
	{
		if (data)
		{
			delete data;
			data = nullptr;
		}
	}
}