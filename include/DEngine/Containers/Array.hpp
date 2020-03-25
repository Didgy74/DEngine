#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Span.hpp"

namespace DEngine::Std
{
	template<typename T, uSize length>
	class Array
	{
	public:
		// Please don't use this field directly
		T m_unused[length] = {};

		

		[[nodiscard]] inline constexpr uSize Size() const noexcept;

		[[nodiscard]] inline constexpr T* Data() noexcept;
		[[nodiscard]] inline constexpr T const* Data() const noexcept;

		[[nodiscard]] inline constexpr Span<T> ToSpan() noexcept;
		[[nodiscard]] inline constexpr Span<T const> ToSpan() const noexcept;

		[[nodiscard]] T& At(uSize i);
		[[nodiscard]] T const& At(uSize i) const;
		[[nodiscard]] inline constexpr T& operator[](uSize i) noexcept;
		[[nodiscard]] inline constexpr T const& operator[](uSize i) const noexcept;

		inline constexpr void Fill(const T& value) noexcept;

		[[nodiscard]] inline constexpr T* begin() noexcept;

		[[nodiscard]] inline constexpr T const* begin() const noexcept;

		[[nodiscard]] inline constexpr T* end() noexcept;

		[[nodiscard]] inline constexpr T const* end() const noexcept;
	};

	template<typename T, uSize length>
	inline constexpr uSize Array<T, length>::Size() const noexcept
	{
		return length;
	}

	template<typename T, uSize length>
	inline constexpr T* Array<T, length>::Data() noexcept
	{
		return m_unused;
	}

	template<typename T, uSize length>
	inline constexpr T const* Array<T, length>::Data() const noexcept
	{
		return m_unused;
	}

	template<typename T, uSize length>
	inline constexpr Span<T> Array<T, length>::ToSpan() noexcept
	{
		return Span<T>(m_unused, length);
	}

	template<typename T, uSize length>
	inline constexpr Span<T const> Array<T, length>::ToSpan() const noexcept
	{
		return Span<T const>(m_unused, length);
	}

	template<typename T, uSize length>
	T& Array<T, length>::At(uSize i)
	{
		if (i >= length)
			throw std::out_of_range("Attempted to .At() an Array with an index out of bounds.");
		return m_unused[i];
	}

	template<typename T, uSize length>
	T const& Array<T, length>::At(uSize i) const
	{
		if (i >= length)
			throw std::out_of_range("Attempted to .At() an Array with an index out of bounds.");
		return m_unused[i];
	}

	template<typename T, uSize length>
	inline constexpr T& Array<T, length>::operator[](uSize i) noexcept
	{
		return At(i);
	}

	template<typename T, uSize length>
	inline constexpr const T& Array<T, length>::operator[](uSize i) const noexcept
	{
		return At(i);
	}


	template<typename T, uSize length>
	inline constexpr void Array<T, length>::Fill(const T& value) noexcept
	{
		for (auto& item : (*this))
			item = value;
	}

	template<typename T, uSize length>
	inline constexpr T* Array<T, length>::begin() noexcept
	{
		return m_unused;
	}

	template<typename T, uSize length>
	inline constexpr const T* Array<T, length>::begin() const noexcept
	{
		return m_unused;
	}

	template<typename T, uSize length>
	inline constexpr T* Array<T, length>::end() noexcept
	{
		return m_unused + length;
	}

	template<typename T, uSize length>
	inline constexpr const T* Array<T, length>::end() const noexcept
	{
		return m_unused + length;
	}
}