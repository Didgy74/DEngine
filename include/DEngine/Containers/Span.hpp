#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Containers/detail/Assert.hpp>
#include <DEngine/Containers/Optional.hpp>

#include <stdexcept>

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

		[[nodiscard]] T* Data() const noexcept;

		[[nodiscard]] uSize Size() const noexcept;

		template<typename Callable>
		[[nodiscard]] Std::Optional<uSize> FindIf(Callable&& test) const;

		[[nodiscard]] T& At(uSize i) const;

		[[nodiscard]] T& operator[](uSize i);
		[[nodiscard]] T const& operator[](uSize i) const;

		[[nodiscard]] T* begin() noexcept;
		[[nodiscard]] T const* begin() const noexcept;
		[[nodiscard]] T* end() noexcept;
		[[nodiscard]] T const* end() const noexcept;

	private:
		T* data = nullptr;
		uSize size = 0;
	};

	template<typename T>
	constexpr Span<T>::Span(T* data, uSize size) noexcept :
		data(data),
		size(size)
	{}

	template<typename T>
	constexpr Span<T const> Span<T>::ConstSpan() const noexcept
	{
		return Span<T const>(data, size);
	}

	template<typename T>
	constexpr Span<T>::operator Span<T const>() const noexcept
	{
		return ConstSpan();
	}

	template<typename T>
	T* Span<T>::Data() const noexcept
	{
		return data;
	}

	template<typename T>
	uSize Span<T>::Size() const noexcept
	{
		return size;
	}

	template<typename T>
	template<typename Callable>
	Std::Optional<uSize> Span<T>::FindIf(Callable&& test) const
	{
		if (data == nullptr)
			return {};
		for (uSize i = 0; i < Size(); i += 1)
		{
			if (test(data[i]))
				return i;
		}
		return {};
	}

	template<typename T>
	T& Span<T>::At(uSize i) const
	{
		if (data == nullptr)
			throw std::runtime_error("Tried to .At() a Span with data pointer set to nullptr.");
		if (i >= size)
			throw std::out_of_range("Tried to .At() a Span with index out of bounds.");
		return data[i];
	}

	template<typename T>
	T& Span<T>::operator[](uSize i)
	{
		return At(i);
	}

	template<typename T>
	T const& Span<T>::operator[](uSize i) const
	{
		return At(i);
	}

	template<typename T>
	T* Span<T>::begin() noexcept
	{
		return data;
	}

	template<typename T>
	T const* Span<T>::begin() const noexcept
	{
		return data;
	}

	template<typename T>
	T* Span<T>::end() noexcept
	{
		return data + size;
	}

	template<typename T>
	T const* Span<T>::end() const noexcept
	{
		return data + size;
	}
}