#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/detail/Assert.hpp"

#include <stdexcept>
#include <new>

namespace DEngine::Std
{
	enum class NullOpt_T : char;

	constexpr NullOpt_T NullOpt = {};

	template<typename T>
	class Optional
	{
		bool m_hasValue = false;
		union
		{
			unsigned char m_unused = {};
			T m_value;
		};
		
	public:
		Optional();
		Optional(Optional<T> const&);
		Optional(Optional<T>&&);
		Optional(NullOpt_T nullOpt);
		Optional(T const&);
		Optional(T&&);
		~Optional();

		Optional<T>& operator=(Optional<T> const&);
		Optional<T>& operator=(Optional<T>&&);

		[[nodiscard]] bool HasValue() const;

		[[nodiscard]] T const& Value() const;
		[[nodiscard]] T& Value();

		[[nodiscard]] T const* ToPtr() const;
		[[nodiscard]] T* ToPtr();
	};

	template<typename T>
	using Opt = Optional<T>;

	template<typename T>
	Optional<T>::Optional() {}

	template<typename T>
	Optional<T>::Optional(Optional<T> const& other)
	{
		if (other.HasValue())
		{
			new(&m_value) T(other.Value());
			m_hasValue = true;
		}
	}

	template<typename T>
	Optional<T>::Optional(Optional<T>&& other)
	{
		if (other.HasValue())
		{
			new(&m_value) T(static_cast<T&&>(other.Value()));
			m_hasValue = true;

			other.m_value.~T();
			other.m_hasValue = false;
		}
	}

	template<typename T>
	Optional<T>::Optional(NullOpt_T nullOpt)
	{
	}

	template<typename T>
	Optional<T>::Optional(T const& value) :
		m_hasValue(true)
	{
		new(&m_value) T(value);
	}

	template<typename T>
	Optional<T>::Optional(T&& value) :
		m_hasValue(true)
	{
		new(&m_value) T(static_cast<T&&>(value));
	}

	template<typename T>
	Optional<T>::~Optional()
	{
		if (HasValue())
			m_value.~T();
	}

	template<typename T>
	Optional<T>& Optional<T>::operator=(Optional<T> const& other)
	{
		if (this == &other)
			return *this;

		if (HasValue())
			m_value.~T();

		if (other.HasValue())
		{
			m_value = other.Value();
			m_hasValue = true;
		}
		else
			m_hasValue = false;
	}

	template<typename T>
	Optional<T>& Optional<T>::operator=(Optional<T>&& other)
	{
		if (this == &other)
			return *this;

		if (HasValue())
			m_value.~T();

		if (other.HasValue())
		{
			m_value = static_cast<T&&>(other.Value());
			m_hasValue = true;
		}
		else
			m_hasValue = false;

		return *this;
	}

	template<typename T>
	bool Optional<T>::HasValue() const
	{
		return m_hasValue;
	}

	template<typename T>
	T const& Optional<T>::Value() const
	{
		if (!HasValue())
			throw std::runtime_error("Tried to deference Optional-variable without Value.");
		return m_value;
	}

	template<typename T>
	T& Optional<T>::Value()
	{
		if (!HasValue())
			throw std::runtime_error("Tried to deference Optional-variable without Value.");
		return m_value;
	}

	template<typename T>
	T const* Optional<T>::ToPtr() const
	{
		if (HasValue())
			return &m_value;
		else
			return nullptr;
	}

	template<typename T>
	T* Optional<T>::ToPtr()
	{
		if (HasValue())
			return &m_value;
		else
			return nullptr;
	}
}