#pragma once

#include "DEngine/Int.hpp"
#include "DEngine/Utility.hpp"
#include "DEngine/Containers/Assert.hpp"

#include <stdexcept>
#include <new>

namespace DEngine::Containers
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
		inline Optional() = default;

		inline Optional(const T& value);

		inline Optional(T&& value);

		[[nodiscard]] inline bool hasValue() const;

		[[nodiscard]] inline const T& value() const;

		[[nodiscard]] inline T& value();
	};

	template<typename T>
	using Opt = Optional<T>;

	template<typename T>
	inline Optional<T>::Optional(const T& value) :
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
	inline bool Optional<T>::hasValue() const
	{
		return m_hasValue;
	}

	template<typename T>
	inline const T& Optional<T>::value() const
	{
		if (m_hasValue == false)
			throw std::runtime_error("Tried to deference Optional-variable without value.");
		return *reinterpret_cast<T const*>(m_valueBuffer);
	}

	template<typename T>
	inline T& Optional<T>::value()
	{
		if (m_hasValue == false)
			throw std::runtime_error("Tried to deference Optional-variable without value.");
		return *reinterpret_cast<T*>(m_valueBuffer);
	}
}

namespace DEngine
{
	namespace Cont = Containers;
}