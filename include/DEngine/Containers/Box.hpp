#pragma once

#include <DEngine/Containers/detail/Assert.hpp>

namespace DEngine::Std
{
	template<typename T>
	class Box
	{
	public:
		using ValueType = T;

		Box() noexcept = default;
		Box(Box<T> const&) = delete;
		Box(Box<T>&&) noexcept;
		Box(T* ptr) noexcept;
		~Box();

		Box<T>& operator=(Box<T> const&) = delete;
		Box<T>& operator=(Box<T>&&) noexcept;

		T* Get() noexcept;
		T const* Get() const noexcept;
		T* operator->() noexcept;
		T const* operator->() const noexcept;

		operator bool() const noexcept;

		void Release() noexcept;

	private:
		T* data = nullptr;
	};

	template<typename T>
	inline Box<T>::Box(Box<T>&& other) noexcept :
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
	inline Box<T>& Box<T>::operator=(Box<T>&& right) noexcept
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
	inline Box<T>::operator bool() const noexcept
	{
		return data != nullptr;
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