#pragma once

#include <DEngine/Std/Containers/impl/Assert.hpp>
#include <DEngine/Std/Trait.hpp>

namespace DEngine::Std::impl { struct OptPlacementNewTag {}; }
constexpr void* operator new(decltype(sizeof(int)) size, void* data, DEngine::Std::impl::OptPlacementNewTag) noexcept { return data; }

namespace DEngine::Std
{
	enum class NullOpt_T : char;

	constexpr NullOpt_T nullOpt = {};

	template<typename T>
	class Opt
	{
	public:
		using ValueType = T;

		Opt(NullOpt_T = nullOpt) noexcept;
		Opt(Opt const&) noexcept;
		Opt(Opt&&) noexcept;
		Opt(T const&) noexcept;
		Opt(Trait::RemoveCVRef<T>&&) noexcept;
		~Opt() noexcept;

		Opt& operator=(Opt const&) noexcept;
		Opt& operator=(Opt&&) noexcept;
		Opt& operator=(T const&) noexcept;
		Opt& operator=(Trait::RemoveCVRef<T>&&) noexcept;

		[[nodiscard]] bool HasValue() const noexcept;

		[[nodiscard]] T const& Value() const noexcept;
		[[nodiscard]] T& Value() noexcept;

		[[nodiscard]] T const* ToPtr() const noexcept;
		[[nodiscard]] T* ToPtr() noexcept;

		void Clear() noexcept;

	protected:
		bool hasValue = false;
		union
		{
			alignas(T) unsigned char unusedChar[sizeof(T)];
			T value;
		};
	};

	template<typename T>
	Opt<T>::Opt(NullOpt_T) noexcept {}

	template<typename T>
	Opt<T>::Opt(Opt const& other) noexcept
	{
		if (other.hasValue)
		{
			new(&value, impl::OptPlacementNewTag{}) T(other.Value());
			hasValue = true;
		}
	}

	template<typename T>
	Opt<T>::Opt(Opt&& other) noexcept
	{
		Clear();
		if (other.hasValue)
		{
			new(&value, impl::OptPlacementNewTag{}) T(static_cast<T&&>(other.Value()));
			hasValue = true;

			other.Clear();
		}
	}

	template<typename T>
	Opt<T>::Opt(T const& other) noexcept :
		hasValue(true)
	{
		new(&value, impl::OptPlacementNewTag{}) T(other);
	}

	template<typename T>
	Opt<T>::Opt(Trait::RemoveCVRef<T>&& other) noexcept :
		hasValue(true)
	{
		new(&value, impl::OptPlacementNewTag{}) T(static_cast<T&&>(other));
	}

	template<typename T>
	Opt<T>::~Opt() noexcept
	{
		Clear();
	}

	template<typename T>
	Opt<T>& Opt<T>::operator=(Opt const& other) noexcept
	{
		if (this == &other)
			return *this;

		if (other.hasValue)
		{
			if (hasValue)
				value = other.value;
			else
				new(&value, impl::OptPlacementNewTag{}) T(other.value);
		}
		else
			Clear();

		hasValue = other.hasValue;
			
		return *this;
	}

	template<typename T>
	Opt<T>& Opt<T>::operator=(Opt&& other) noexcept
	{
		if (this == &other)
			return *this;

		if (other.hasValue)
		{
			if (hasValue)
				value = static_cast<T&&>(other.value);
			else
				new(&value, impl::OptPlacementNewTag{}) T(static_cast<T&&>(other.value));
		}
		else
			Clear();

		hasValue = other.hasValue;

		return *this;
	}

	template<typename T>
	Opt<T>& Opt<T>::operator=(T const& right) noexcept
	{
		if (hasValue)
			value = right;
		else
		{
			new(&value, impl::OptPlacementNewTag{}) T(right);
			hasValue = true;
		}
		return *this;
	}

	template<typename T>
	Opt<T>& Opt<T>::operator=(Trait::RemoveCVRef<T>&& right) noexcept
	{
		if (hasValue)
			value = static_cast<T&&>(right);
		else
		{
			new(&value, impl::OptPlacementNewTag{}) T(static_cast<T&&>(right));
			hasValue = true;
		}
		return *this;
	}

	template<typename T>
	bool Opt<T>::HasValue() const noexcept
	{
		return hasValue;
	}

	template<typename T>
	T const& Opt<T>::Value() const noexcept
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			hasValue,
			"Tried to deference Opt without a value.");
		return value;
	}

	template<typename T>
	T& Opt<T>::Value() noexcept
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			hasValue,
			"Tried to deference Opt without a value.");
		return value;
	}

	template<typename T>
	T const* Opt<T>::ToPtr() const noexcept
	{
		return hasValue ? &value : nullptr;
	}

	template<typename T>
	T* Opt<T>::ToPtr() noexcept
	{
		return hasValue ? &value : nullptr;
	}

	template<typename T>
	void Opt<T>::Clear() noexcept
	{
		if (hasValue)
		{
			value.~T();
			hasValue = false;
		}
	}
}