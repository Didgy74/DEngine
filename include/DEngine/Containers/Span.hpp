#pragma once

#include "DEngine/FixedWidthTypes.hpp"
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

		inline constexpr Span(T* Data, uSize size) noexcept;

		[[nodiscard]] inline constexpr Span<T const> ConstSpan() const noexcept;
		inline constexpr operator Span<T const>() const noexcept;

		[[nodiscard]] inline T* Data() const noexcept;

		[[nodiscard]] inline uSize Size() const noexcept;

		[[nodiscard]] inline T& At(uSize i);
		[[nodiscard]] inline T const& At(uSize i) const;

		[[nodiscard]] inline T& operator[](uSize i) noexcept;
		[[nodiscard]] inline T const& operator[](uSize i) const noexcept;

		[[nodiscard]] inline T* begin() noexcept;
		[[nodiscard]] inline T const* begin() const noexcept;
		[[nodiscard]] inline T* end() noexcept;
		[[nodiscard]] inline T const* end() const noexcept;
	};

	template<typename T>
	inline constexpr Span<T>::Span(T* Data, uSize size) noexcept :
		m_dataBuffer(Data),
		m_size(size)
	{}

	template<typename T>
	inline constexpr Span<T const> Span<T>::ConstSpan() const noexcept
	{
		return Span<T const>(m_dataBuffer, m_size);
	}

	template<typename T>
	inline constexpr Span<T>::operator Span<T const>() const noexcept
	{
		return ConstSpan();
	}

	template<typename T>
	inline T* Span<T>::Data() const noexcept
	{
		return m_dataBuffer;
	}

	template<typename T>
	inline uSize Span<T>::Size() const noexcept
	{
		return m_size;
	}

	template<typename T>
	inline T& Span<T>::At(uSize i)
	{
		if (i >= m_size)
			throw std::out_of_range("Tried to .At() a Span with index out of bounds.");
		return m_dataBuffer[i];
	}

	template<typename T>
	inline T const& Span<T>::At(uSize i) const
	{
		if (i >= m_size)
			throw std::out_of_range("Tried to .At() a Span with index out of bounds.");
		return m_dataBuffer[i];
	}

	template<typename T>
	inline T& Span<T>::operator[](uSize i) noexcept
	{
		return At(i);
	}

	template<typename T>
	inline T const& Span<T>::operator[](uSize i) const noexcept
	{
		return At(i);
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

