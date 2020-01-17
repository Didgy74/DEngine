#pragma once

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Assert.hpp"

namespace DEngine::Containers
{
	template<typename T>
	class Span
	{
	private:
		T* m_data = nullptr;
		uSize m_size = 0;

	public:
		[[nodiscard]] inline Span() noexcept {}

		[[nodiscard]] inline Span(T* data, uSize size) noexcept :
			m_data(data),
			m_size(size)
		{}

		inline operator Span<const T>() const noexcept;

		[[nodiscard]] inline T* data() const noexcept;

		[[nodiscard]] inline uSize size() const noexcept;

		[[nodiscard]] inline T& operator[](uSize i) noexcept;

		[[nodiscard]] inline const T& operator[](uSize i) const noexcept;

		[[nodiscard]] inline T* begin() noexcept;

		[[nodiscard]] inline const T* begin() const noexcept;

		[[nodiscard]] inline T* end() noexcept;

		[[nodiscard]] inline const T* end() const noexcept;
	};

	template<typename T>
	inline Span<T>::operator Span<const T>() const noexcept
	{
		return Span<const T>(m_data, m_size);
	}

	template<typename T>
	inline T* Span<T>::data() const noexcept
	{
		return m_data;
	}

	template<typename T>
	inline uSize Span<T>::size() const noexcept
	{
		return m_size;
	}

	template<typename T>
	inline T& Span<T>::operator[](uSize i) noexcept
	{
		DENGINE_CONTAINERS_ASSERT_MSG(i < m_size, "DEngine Span subscript out of range.");
		return *(m_data + i);
	}

	template<typename T>
	inline const T& Span<T>::operator[](uSize i) const noexcept
	{
		DENGINE_CONTAINERS_ASSERT_MSG(i < m_size, "DEngine Span subscript out of range.");
		return *(m_data + i);
	}

	template<typename T>
	inline T* Span<T>::begin() noexcept
	{
		return m_data;
	}

	template<typename T>
	inline const T* Span<T>::begin() const noexcept
	{
		return m_data;
	}

	template<typename T>
	inline T* Span<T>::end() noexcept
	{
		return m_data + m_size;
	}

	template<typename T>
	inline const T* Span<T>::end() const noexcept
	{
		return m_data + m_size;
	}
}

namespace DEngine
{
	namespace Cont = Containers;
}

