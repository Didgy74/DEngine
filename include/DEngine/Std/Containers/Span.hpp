#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/detail/Assert.hpp>
#include <DEngine/Std/Containers/Range.hpp>

namespace DEngine::Std
{
	template<typename T>
	class Span
	{
	public:
		using ValueType = T;

		constexpr Span() noexcept = default;
		constexpr Span(T* data, uSize size) noexcept;

		[[nodiscard]] constexpr Span<T const> ConstSpan() const noexcept;
		constexpr operator Span<T const>() const noexcept;

		[[nodiscard]] constexpr Range<T*> AsRange() const noexcept;
		constexpr operator Range<T*>() const noexcept;

		[[nodiscard]] T* Data() const noexcept;

		[[nodiscard]] uSize Size() const noexcept;

		[[nodiscard]] T& At(uSize i) const;

		[[nodiscard]] T& operator[](uSize i) const;

		[[nodiscard]] T* begin() const noexcept;
		[[nodiscard]] T* end() const noexcept;

	private:
		T* m_data = nullptr;
		uSize m_size = 0;
	};

	template<typename T>
	constexpr Span<T>::Span(T* data, uSize size) noexcept :
		m_data(data),
		m_size(size)
	{}

	template<typename T>
	constexpr Span<T const> Span<T>::ConstSpan() const noexcept
	{
		return Span<T const>(m_data, m_size);
	}

	template<typename T>
	constexpr Range<T*> Span<T>::AsRange() const noexcept
	{
		Range<T*> returnVal;
		returnVal.begin = begin();
		returnVal.end = end();
		return returnVal;
	}

	template<typename T>
	constexpr Span<T>::operator Span<T const>() const noexcept
	{
		return ConstSpan();
	}

	template<typename T>
	T* Span<T>::Data() const noexcept
	{
		return m_data;
	}

	template<typename T>
	uSize Span<T>::Size() const noexcept
	{
		return m_size;
	}

	template<typename T>
	T& Span<T>::At(uSize i) const
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			m_data != nullptr,
			"Tried to .At() a Span with data pointer set to nullptr.");
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			i < m_size,
			"Tried to .At() a Span with index out of bounds.");
		return m_data[i];
	}

	template<typename T>
	T& Span<T>::operator[](uSize i) const
	{
		return At(i);
	}

	template<typename T>
	T* Span<T>::begin() const noexcept
	{
		return m_data;
	}

	template<typename T>
	T* Span<T>::end() const noexcept
	{
		return m_data + m_size;
	}
}