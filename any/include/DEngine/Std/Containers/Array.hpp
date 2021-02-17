#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/detail/Assert.hpp>
#include <DEngine/Std/Containers/Span.hpp>

namespace DEngine::Std
{
	template<typename T, uSize length>
	class Array
	{
	public:
		using ValueType = T;

		// Please don't use this field directly
		T data[length];

		[[nodiscard]] constexpr uSize Size() const noexcept;

		[[nodiscard]] constexpr T* Data() noexcept;
		[[nodiscard]] constexpr T const* Data() const noexcept;

		[[nodiscard]] constexpr Std::Span<T> Span() noexcept;
		[[nodiscard]] constexpr Std::Span<T const> Span() const noexcept;

		[[nodiscard]] T& At(uSize i) noexcept;
		[[nodiscard]] T const& At(uSize i) const noexcept;
		[[nodiscard]] T& operator[](uSize i) noexcept;
		[[nodiscard]] T const& operator[](uSize i) const noexcept;

		constexpr void Fill(T const& value) noexcept;

		[[nodiscard]] constexpr T* begin() noexcept;
		[[nodiscard]] constexpr T const* begin() const noexcept;
		[[nodiscard]] constexpr T* end() noexcept;
		[[nodiscard]] constexpr T const* end() const noexcept;
	};

	template<typename T, uSize length>
	constexpr uSize Array<T, length>::Size() const noexcept
	{
		return length;
	}

	template<typename T, uSize length>
	constexpr T* Array<T, length>::Data() noexcept
	{
		return data;
	}

	template<typename T, uSize length>
	constexpr T const* Array<T, length>::Data() const noexcept
	{
		return data;
	}

	template<typename T, uSize length>
	constexpr Span<T> Array<T, length>::Span() noexcept
	{
		return { data, length };
	}

	template<typename T, uSize length>
	constexpr Span<T const> Array<T, length>::Span() const noexcept
	{
		return { data, length };
	}

	template<typename T, uSize length>
	T& Array<T, length>::At(uSize i) noexcept
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			i < length,
			"Attempted to .At() an Array with an index out of bounds.");
		return data[i];
	}

	template<typename T, uSize length>
	T const& Array<T, length>::At(uSize i) const noexcept
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			i < length,
			"Attempted to .At() an Array with an index out of bounds.");
		return data[i];
	}

	template<typename T, uSize length>
	T& Array<T, length>::operator[](uSize i) noexcept
	{
		return At(i);
	}

	template<typename T, uSize length>
	T const& Array<T, length>::operator[](uSize i) const noexcept
	{
		return At(i);
	}

	template<typename T, uSize length>
	constexpr void Array<T, length>::Fill(T const& value) noexcept
	{
		for (auto& item : (*this))
			item = value;
	}

	template<typename T, uSize length>
	constexpr T* Array<T, length>::begin() noexcept
	{
		return data;
	}

	template<typename T, uSize length>
	constexpr T const* Array<T, length>::begin() const noexcept
	{
		return data;
	}

	template<typename T, uSize length>
	constexpr T* Array<T, length>::end() noexcept
	{
		return data + length;
	}

	template<typename T, uSize length>
	constexpr T const* Array<T, length>::end() const noexcept
	{
		return data + length;
	}
}