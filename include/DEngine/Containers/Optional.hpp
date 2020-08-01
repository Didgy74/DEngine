#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Containers/detail/Assert.hpp>

#include <stdexcept>
#include <new>

namespace DEngine::Std
{
	enum class NullOpt_T : char;

	constexpr NullOpt_T nullOpt = {};

	template<typename T>
	class Optional
	{	
	public:
		using ValueType = T;

		constexpr Optional();
		constexpr Optional(Optional<T> const&);
		constexpr Optional(Optional<T>&&) noexcept;
		constexpr Optional(NullOpt_T nullOpt) noexcept;
		constexpr Optional(T const&);
		constexpr Optional(T&&) noexcept;
		~Optional();

		Optional<T>& operator=(Optional<T> const&);
		Optional<T>& operator=(Optional<T>&&) noexcept;
		Optional<T>& operator=(T const&);
		Optional<T>& operator=(T&&) noexcept;

		[[nodiscard]] bool HasValue() const;

		[[nodiscard]] T const& Value() const;
		[[nodiscard]] T& Value();

		[[nodiscard]] T const* ToPtr() const;
		[[nodiscard]] T* ToPtr();


	private:
		bool hasValue = false;
		union
		{
			unsigned char unusedChar{};
			T value;
		};
	};

	template<typename T>
	using Opt = Optional<T>;

	template<typename T>
	constexpr Optional<T>::Optional() {}

	template<typename T>
	constexpr Optional<T>::Optional(Optional<T> const& other)
	{
		if (other.hasValue)
		{
			new(&value) T(other.Value());
			hasValue = true;
		}
	}

	template<typename T>
	constexpr Optional<T>::Optional(Optional<T>&& other) noexcept
	{
		if (other.hasValue)
		{
			new(&value) T(static_cast<T&&>(other.Value()));
			hasValue = true;

			other.value.~T();
			other.hasValue = false;
		}
	}

	template<typename T>
	constexpr Optional<T>::Optional(NullOpt_T nullOpt) noexcept {}

	template<typename T>
	constexpr Optional<T>::Optional(T const& other) :
		hasValue(true)
	{
		new(&value) T(other);
	}

	template<typename T>
	constexpr Optional<T>::Optional(T&& other) noexcept :
		hasValue(true)
	{
		new(&value) T(static_cast<T&&>(other));
	}

	template<typename T>
	Optional<T>::~Optional()
	{
		if (hasValue)
			value.~T();
	}

	template<typename T>
	Optional<T>& Optional<T>::operator=(Optional<T> const& other)
	{
		if (this == &other)
			return *this;

		if (hasValue)
			value.~T();

		if (other.hasValue)
		{
			value = other.value;
			hasValue = true;
		}
		else
			hasValue = false;
	}

	template<typename T>
	Optional<T>& Optional<T>::operator=(Optional<T>&& other) noexcept
	{
		if (this == &other)
			return *this;

		if (hasValue)
			value.~T();

		if (other.hasValue)
		{
			value = static_cast<T&&>(other.value);
			hasValue = true;
		}
		else
			hasValue = false;

		return *this;
	}

	template<typename T>
	Optional<T>& Optional<T>::operator=(T const& right)
	{
		if (hasValue)
			value = right;
		else
		{
			new(&value) T(right);
			hasValue = true;
		}
		return *this;
	}

	template<typename T>
	Optional<T>& Optional<T>::operator=(T&& right) noexcept
	{
		if (hasValue)
			value = static_cast<T&&>(right);
		else
		{
			new(&value) T(static_cast<T&&>(right));
			hasValue = true;
		}
		return *this;
	}

	template<typename T>
	bool Optional<T>::HasValue() const
	{
		return hasValue;
	}

	template<typename T>
	T const& Optional<T>::Value() const
	{
		if (!hasValue)
			throw std::runtime_error("Tried to deference Optional-variable without Value.");
		return value;
	}

	template<typename T>
	T& Optional<T>::Value()
	{
		if (!hasValue)
			throw std::runtime_error("Tried to deference Optional-variable without Value.");
		return value;
	}

	template<typename T>
	T const* Optional<T>::ToPtr() const
	{
		if (hasValue)
			return &value;
		else
			return nullptr;
	}

	template<typename T>
	T* Optional<T>::ToPtr()
	{
		if (hasValue)
			return &value;
		else
			return nullptr;
	}
}