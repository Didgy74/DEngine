#pragma once

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Assert.hpp"

#include <stdexcept>

namespace DEngine::Containers
{
	template<typename T>
	class Span
	{
	private:
		T* m_dataBuffer = nullptr;
		uSize m_size = 0;

	public:
		inline constexpr Span() noexcept = default;

		inline constexpr Span(T* data, uSize size) noexcept;

		[[nodiscard]] inline constexpr Span<T const> constSpan() const noexcept;
		inline constexpr operator Span<T const>() const noexcept;

		[[nodiscard]] inline T* data() const noexcept;

		[[nodiscard]] inline uSize size() const noexcept;

		[[nodiscard]] inline T& at(uSize i);
		[[nodiscard]] inline T const& at(uSize i) const;

		[[nodiscard]] inline T& operator[](uSize i) noexcept;
		[[nodiscard]] inline T const& operator[](uSize i) const noexcept;

		[[nodiscard]] inline T* begin() noexcept;
		[[nodiscard]] inline T const* begin() const noexcept;
		[[nodiscard]] inline T* end() noexcept;
		[[nodiscard]] inline T const* end() const noexcept;
	};

	template<typename T>
	inline constexpr Span<T>::Span(T* data, uSize size) noexcept :
		m_dataBuffer(data),
		m_size(size)
	{}

	template<typename T>
	inline constexpr Span<T const> Span<T>::constSpan() const noexcept
	{
		return Span<T const>(m_dataBuffer, m_size);
	}

	template<typename T>
	inline constexpr Span<T>::operator Span<T const>() const noexcept
	{
		return constSpan();
	}

	template<typename T>
	inline T* Span<T>::data() const noexcept
	{
		return m_dataBuffer;
	}

	template<typename T>
	inline uSize Span<T>::size() const noexcept
	{
		return m_size;
	}

	template<typename T>
	inline T& Span<T>::at(uSize i)
	{
		if (i >= m_size)
			throw std::out_of_range("Called .at() on Span object with index out of bounds.");
		return m_dataBuffer[i];
	}

	template<typename T>
	inline T const& Span<T>::at(uSize i) const
	{
		if (i >= m_size)
			throw std::out_of_range("Called .at() on Span object with index out of bounds.");
		return m_dataBuffer[i];
	}

	template<typename T>
	inline T& Span<T>::operator[](uSize i) noexcept
	{
		DENGINE_CONTAINERS_ASSERT_MSG(i < m_size, "Span subscript out of range.");
		return m_dataBuffer[i];
	}

	template<typename T>
	inline T const& Span<T>::operator[](uSize i) const noexcept
	{
		DENGINE_CONTAINERS_ASSERT_MSG(i < m_size, "Span subscript out of range.");
		return m_dataBuffer[i];
	}

	template<typename T>
	inline T* Span<T>::begin() noexcept
	{
		return m_dataBuffer;
	}

	template<typename T>
	inline T const* Span<T>::begin() const noexcept
	{
		return m_dataBuffer;
	}

	template<typename T>
	inline T* Span<T>::end() noexcept
	{
		return m_dataBuffer + m_size;
	}

	template<typename T>
	inline T const* Span<T>::end() const noexcept
	{
		return m_dataBuffer + m_size;
	}
}

namespace DEngine
{
	namespace Cont = Containers;
}

