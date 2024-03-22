#pragma once

#include <DEngine/Std/Containers/impl/Assert.hpp>
#include <DEngine/Std/Trait.hpp>

#include <new>

namespace DEngine::Std {
	enum class NullOpt_T : char;
	constexpr NullOpt_T nullOpt = {};

	template<typename T>
	class Opt {
	public:
		using ValueType = T;

		Opt([[maybe_unused]] NullOpt_T = nullOpt) noexcept;

		Opt(Opt const& other) noexcept requires (Std::Trait::isCopyConstructible<T>) :
			hasValue{ other.hasValue }
		{
			if (other.hasValue)
				new(&value) T(other.value);
		}

		Opt(Opt&& other) noexcept requires (Std::Trait::isMoveConstructible<T>) :
			hasValue{ other.hasValue }
		{
			if (other.hasValue) {
				new(&value) T(static_cast<T&&>(other.value));
				other.Clear();
			}
		}

		Opt(T const& other) noexcept requires (Std::Trait::isCopyConstructible<T>) : hasValue{ true } {
			new(&value) T(other);
		}

		Opt(Trait::RemoveCVRef<T>&& other) noexcept requires (Std::Trait::isMoveConstructible<T>) : hasValue{ true } {
			new(&value) T(static_cast<T&&>(other));
		}

		~Opt() noexcept;

		template<class... Ts>
		void Emplace(Ts&&... in) noexcept {
			if (hasValue)
				Clear();

			new(&value) T(in...);
			hasValue = true;
		}


		Opt& operator=(Opt const& other) noexcept requires (Std::Trait::isCopyAssignable<T>) {
			if (this == &other)
				return *this;

			if (other.hasValue) {
				if (hasValue)
					value = other.value;
				else
					new(&value) T(other.value);
			}
			else
				Clear();

			hasValue = other.hasValue;

			return *this;
		}

		Opt& operator=(Opt&& other) noexcept requires (Std::Trait::isMoveAssignable<T>)
		{
			if (this == &other)
				return *this;

			if (other.hasValue)
			{
				if (hasValue)
					value = static_cast<T&&>(other.value);
				else
					new(&value) T(static_cast<T&&>(other.value));
			}
			else
				Clear();

			hasValue = other.hasValue;

			other.Clear();

			return *this;
		}

		Opt& operator=(T const& other) noexcept requires (Std::Trait::isCopyAssignable<T>)
		{
			if (hasValue)
				value = other;
			else
			{
				new(&value) T(other);
				hasValue = true;
			}
			return *this;
		}

		Opt& operator=(Trait::RemoveCVRef<T>&& other) noexcept requires (Std::Trait::isMoveAssignable<T>)
		{
			if (hasValue)
				value = static_cast<T&&>(other);
			else
			{
				new(&value) T(static_cast<T&&>(other));
				hasValue = true;
			}
			return *this;
		}

		Opt& operator=([[maybe_unused]] NullOpt_T) noexcept;

		[[nodiscard]] bool HasValue() const noexcept;
		[[nodiscard]] bool Has() const noexcept;

		[[nodiscard]] T const& Value() const& noexcept
		{
			DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
				hasValue,
				"Tried to deference Opt without a value.");
			return value;
		}
		[[nodiscard]] T const& Get() const& noexcept { return Value(); }
		[[nodiscard]] T& Value() & noexcept
		{
			DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
				hasValue,
				"Tried to deference Opt without a value.");
			return value;
		}
		[[nodiscard]] T& Get() & noexcept { return Value(); }
		[[nodiscard]] T&& Value() && noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
				hasValue,
				"Tried to deference Opt without a value.");
			return static_cast<T&&>(value);
		}
		[[nodiscard]] T&& Get() && noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
				hasValue,
				"Tried to deference Opt without a value.");
			return static_cast<T&&>(value);
		}

		[[nodiscard]] T const* ToPtr() const noexcept;
		[[nodiscard]] T* ToPtr() noexcept;

		void Clear() noexcept;

	protected:
		bool hasValue = false;
		union
		{
			[[maybe_unused]] alignas(T) unsigned char unusedChar[sizeof(T)];
			T value;
		};
	};

	template<typename T>
	Opt<T>::Opt([[maybe_unused]] NullOpt_T) noexcept {}

	template<typename T>
	Opt<T>::~Opt() noexcept
	{
		Clear();
	}

	template<typename T>
	Opt<T>& Opt<T>::operator=([[maybe_unused]] NullOpt_T) noexcept
	{
		Clear();
		return *this;
	}

	template<typename T>
	bool Opt<T>::HasValue() const noexcept { return hasValue; }

	template<typename T>
	bool Opt<T>::Has() const noexcept { return hasValue; }

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
			if constexpr (!Trait::isTriviallyDestructible<T>)
				value.~T();

			hasValue = false;
		}
	}
}