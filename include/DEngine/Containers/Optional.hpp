#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Utility.hpp"
#include "DEngine/Containers/detail/Assert.hpp"

#include <stdexcept>
#include <new>

namespace DEngine::Std
{
	template<typename T>
	class Optional
	{
		bool m_hasValue = false;
		union
		{
			alignas(T) unsigned char m_valueBuffer[sizeof(T)] = {};
			T m_value;
		};
		
	public:
		inline Optional();
		inline Optional(T const& value);
		inline Optional(T&& value);
		inline ~Optional();

		inline Optional<T>& operator=(Optional<T> const& other);

		[[nodiscard]] inline bool HasValue() const;

		[[nodiscard]] inline T const& Value() const;
		[[nodiscard]] inline T& Value();

		[[nodiscard]] inline T const* ToPtr() const;
		[[nodiscard]] inline T* ToPtr();
	};

	template<typename T>
	using Opt = Optional<T>;

	template<typename T>
	inline Optional<T>::Optional() :
		m_hasValue(false)
	{
	}

	template<typename T>
	inline Optional<T>::Optional(T const& value) :
		m_hasValue(true)
	{
		new(m_valueBuffer) T(value);
	}

	template<typename T>
	inline Optional<T>::Optional(T&& value) :
		m_hasValue(true)
	{
		new(m_valueBuffer) T(static_cast<T&&>(value));
	}

	template<typename T>
	inline Optional<T>::~Optional()
	{
		if (HasValue())
			m_value.~T();
	}

	template<typename T>
	inline Optional<T>& Optional<T>::operator=(Optional<T> const& other)
	{
		if (this == &other)
			return *this;

		if (HasValue())
			m_value.~T();

		if (other.HasValue())
			m_value = other.Value();
		else
			m_hasValue = false;
	}

	template<typename T>
	inline bool Optional<T>::HasValue() const
	{
		return m_hasValue;
	}

	template<typename T>
	inline T const& Optional<T>::Value() const
	{
		if (!HasValue())
			throw std::runtime_error("Tried to deference Optional-variable without Value.");
		return m_value;
	}

	template<typename T>
	inline T& Optional<T>::Value()
	{
		if (!HasValue())
			throw std::runtime_error("Tried to deference Optional-variable without Value.");
		return m_value;
	}

	template<typename T>
	inline T const* Optional<T>::ToPtr() const
	{

	}

	template<typename T>
	inline T* Optional<T>::ToPtr()
	{
		
	}
}