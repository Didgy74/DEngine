#pragma once

#include "DEngine/Int.hpp"

#include "DEngine/Containers/Assert.hpp"

namespace DEngine::Containers
{
	template<typename T, uSize length>
	class Array
	{
	public:
		// Please don't use this field directly
		T m_dataBuffer[length] = {};

		inline constexpr void fill(const T& value) noexcept;

		[[nodiscard]] inline constexpr uSize size() const noexcept;

		[[nodiscard]] inline constexpr T* data() noexcept;

		[[nodiscard]] inline constexpr const T* data() const noexcept;

		[[nodiscard]] inline constexpr T& operator[](uSize i) noexcept;

		[[nodiscard]] inline constexpr const T& operator[](uSize i) const noexcept;

		[[nodiscard]] inline constexpr T* begin() noexcept;

		[[nodiscard]] inline constexpr const T* begin() const noexcept;

		[[nodiscard]] inline constexpr T* end() noexcept;

		[[nodiscard]] inline constexpr const T* end() const noexcept;
	};

	template<typename T, uSize length>
	inline constexpr uSize Array<T, length>::size() const noexcept
	{
		return length;
	}

	template<typename T, uSize length>
	inline constexpr T* Array<T, length>::data() noexcept
	{
		return m_dataBuffer;
	}

	template<typename T, uSize length>
	inline constexpr const T* Array<T, length>::data() const noexcept
	{
		return m_dataBuffer;
	}

	template<typename T, uSize length>
	inline constexpr void Array<T, length>::fill(const T& value) noexcept
	{
		for (auto& item : (*this))
		{
			item = value;
		}
	}

	template<typename T, uSize length>
	inline constexpr T& Array<T, length>::operator[](uSize i) noexcept
	{
		DENGINE_CONTAINERS_ASSERT_MSG(i < length, "DEngine Array subscript out of range.");
		return m_dataBuffer[i];
	}

	template<typename T, uSize length>
	inline constexpr const T& Array<T, length>::operator[](uSize i) const noexcept
	{
		DENGINE_CONTAINERS_ASSERT_MSG(i < length, "DEngine Array subscript out of range.");
		return m_dataBuffer[i];
	}

	template<typename T, uSize length>
	inline constexpr T* Array<T, length>::begin() noexcept
	{
		return m_dataBuffer;
	}

	template<typename T, uSize length>
	inline constexpr const T* Array<T, length>::begin() const noexcept
	{
		return m_dataBuffer;
	}

	template<typename T, uSize length>
	inline constexpr T* Array<T, length>::end() noexcept
	{
		return m_dataBuffer + length;
	}

	template<typename T, uSize length>
	inline constexpr const T* Array<T, length>::end() const noexcept
	{
		return m_dataBuffer + length;
	}
}

namespace DEngine
{
	namespace Cont = Containers;
}

