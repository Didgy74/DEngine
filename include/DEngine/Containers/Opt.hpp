#pragma once

#include <DEngine/Containers/detail/Assert.hpp>

#include <new>

namespace DEngine::Std
{
	enum class NullOpt_T : char;

	constexpr NullOpt_T nullOpt = {};

	template<typename T>
	class Opt
	{	
	public:
		using ValueType = T;

		constexpr Opt(NullOpt_T = nullOpt) noexcept;
		constexpr Opt(Opt const&);
		constexpr Opt(Opt&&) noexcept;
		explicit constexpr Opt(T const&);
		explicit constexpr Opt(T&&) noexcept;
		~Opt();

		Opt& operator=(Opt const&);
		Opt& operator=(Opt&&) noexcept;
		Opt& operator=(T const&);
		Opt& operator=(T&&) noexcept;

		[[nodiscard]] bool HasValue() const;

		[[nodiscard]] T const& Value() const;
		[[nodiscard]] T& Value();

		[[nodiscard]] T const* ToPtr() const;
		[[nodiscard]] T* ToPtr();

	private:
		bool hasValue = false;
		union
		{
			unsigned char unusedChar = 0;
			T value;
		};
	};

	template<typename T>
	constexpr Opt<T>::Opt(NullOpt_T) noexcept {}

	template<typename T>
	constexpr Opt<T>::Opt(Opt const& other)
	{
		if (other.hasValue)
		{
			new(&value) T(other.Value());
			hasValue = true;
		}
	}

	template<typename T>
	constexpr Opt<T>::Opt(Opt&& other) noexcept
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
	constexpr Opt<T>::Opt(T const& other) :
		hasValue(true)
	{
		new(&value) T(other);
	}

	template<typename T>
	constexpr Opt<T>::Opt(T&& other) noexcept :
		hasValue(true)
	{
		new(&value) T(static_cast<T&&>(other));
	}

	template<typename T>
	Opt<T>::~Opt()
	{
		if (hasValue)
			value.~T();
	}

	template<typename T>
	Opt<T>& Opt<T>::operator=(Opt const& other)
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
	Opt<T>& Opt<T>::operator=(Opt&& other) noexcept
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
	Opt<T>& Opt<T>::operator=(T const& right)
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
	Opt<T>& Opt<T>::operator=(T&& right) noexcept
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
	bool Opt<T>::HasValue() const
	{
		return hasValue;
	}

	template<typename T>
	T const& Opt<T>::Value() const
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			hasValue,
			"Tried to deference Opt without a value.");
		return value;
	}

	template<typename T>
	T& Opt<T>::Value()
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			hasValue,
			"Tried to deference Opt without a value.");
		return value;
	}

	template<typename T>
	T const* Opt<T>::ToPtr() const
	{
		if (hasValue)
			return &value;
		else
			return nullptr;
	}

	template<typename T>
	T* Opt<T>::ToPtr()
	{
		if (hasValue)
			return &value;
		else
			return nullptr;
	}
}