#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Utility.hpp"
#include "DEngine/Containers/Assert.hpp"

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

		[[nodiscard]] inline bool HasValue() const;

		[[nodiscard]] inline T const& Value() const;

		[[nodiscard]] inline T& Value();
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
		if (m_hasValue)
			m_value.~T();
	}

	template<typename T>
	inline bool Optional<T>::HasValue() const
	{
		return m_hasValue;
	}

	template<typename T>
	inline T const& Optional<T>::Value() const
	{
		if (m_hasValue == false)
			throw std::runtime_error("Tried to deference Optional-variable without Value.");
		return m_value;
	}

	template<typename T>
	inline T& Optional<T>::Value()
	{
		if (m_hasValue == false)
			throw std::runtime_error("Tried to deference Optional-variable without Value.");
		return m_value;
	}
}