#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/detail/Assert.hpp>

#include <new>

namespace DEngine::Std
{
	// Constant-capacity vector.
	// Use for small vectors of fixed capacity.
	template<typename T, uSize capacity>
	class StackVec
	{
	public:
		using ValueType = T;

		StackVec() noexcept;
		StackVec(uSize length);
		StackVec(StackVec<T, capacity> const&);
		StackVec(StackVec<T, capacity>&&) noexcept;
		~StackVec();

		StackVec<T, capacity>& operator=(StackVec<T, capacity> const&);
		StackVec<T, capacity>& operator=(StackVec<T, capacity>&&) noexcept;

		[[nodiscard]] constexpr Span<T> ToSpan() noexcept;
		[[nodiscard]] constexpr Span<T const> ToSpan() const noexcept;
		constexpr operator Span<T>() noexcept;
		constexpr operator Span<T const>() noexcept;

		[[nodiscard]] bool CanPushBack() const noexcept;
		[[nodiscard]] static constexpr uSize Capacity() noexcept;
		[[nodiscard]] T* Data() noexcept;
		[[nodiscard]] T const* Data() const noexcept;
		[[nodiscard]] bool IsEmpty() const noexcept;
		[[nodiscard]] uSize Size() const noexcept;

		void Clear();
		void Erase(uSize index);
		void EraseUnsorted(uSize index);
		void Insert(uSize index, T&& in);
		void PushBack(T const& in);
		void PushBack(T&& in);
		void Resize(uSize newSize);

		[[nodiscard]] T& At(uSize i) noexcept;
		[[nodiscard]] T const& At(uSize i) const noexcept;
		[[nodiscard]] T& operator[](uSize i) noexcept;
		[[nodiscard]] T const& operator[](uSize i) const noexcept;

		[[nodiscard]] T* begin() noexcept;
		[[nodiscard]] T const* begin() const noexcept;
		[[nodiscard]] T* end() noexcept;
		[[nodiscard]] T const* end() const noexcept;

	private:
		union
		{
			alignas(T) unsigned char unused = 0;
			T values[capacity];
		};
		uSize size = 0;
	};

	template<typename T, uSize capacity>
	StackVec<T, capacity>::StackVec() noexcept {}

	template<typename T, uSize capacity>
	StackVec<T, capacity>::StackVec(uSize length) :
		size(length)
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			length <= capacity, 
			"Attempted to create a StackVec of size larger than it's capacity.");
		new(values) T[length];
	}

	template<typename T, uSize capacity>
	StackVec<T, capacity>::StackVec(StackVec<T, capacity> const& other) :
		size(other.size)
	{
		for (uSize i = 0; i < size; i += 1)
			new(values + i) T(other[i]);
	}

	template<typename T, uSize capacity>
	StackVec<T, capacity>::StackVec(StackVec<T, capacity>&& other) noexcept :
		size(other.size)
	{
		for (uSize i = 0; i < size; i += 1)
			new(values + i) T(static_cast<T&&>(other[i]));
		other.Clear();
	}

	template<typename T, uSize capacity>
	StackVec<T, capacity>::~StackVec()
	{
		Clear();
	}

	template<typename T, uSize capacity>
	StackVec<T, capacity>& StackVec<T, capacity>::operator=(StackVec<T, capacity> const& other)
	{
		if (this == &other)
			return *this;

		uSize i = 0;
		for (; i < (size < other.size ? size : other.size); i++)
			values[i] = other[i];
		for (; i < size; i++)
			values[i].~T();
		for (; i < other.size; i++)
			new(values + i) T(other[i]);

		size = other.size;

		return *this;
	}

	template<typename T, uSize capacity>
	StackVec<T, capacity>& StackVec<T, capacity>::operator=(StackVec<T, capacity>&& other) noexcept
	{
		if (this == &other)
			return *this;

		uSize i = 0;
		for (; i < (size < other.size ? size : other.size); i++)
			values[i] = static_cast<T&&>(other[i]);
		for (; i < size; i++)
			values[i].~T();
		for (; i < other.size; i++)
			new(values + i) T(static_cast<T&&>(other[i]));
		size = other.size;

		other.Clear();
		return *this;
	}

	template<typename T, uSize capacity>
	constexpr Span<T> StackVec<T, capacity>::ToSpan() noexcept
	{
		return Span<T>(values, size);
	}

	template<typename T, uSize capacity>
	constexpr Span<T const> StackVec<T, capacity>::ToSpan() const noexcept
	{
		return Span<T const>(values, size);
	}

	template<typename T, uSize capacity>
	constexpr StackVec<T, capacity>::operator Span<T>() noexcept
	{
		return ToSpan();
	}

	template<typename T, uSize capacity>
	constexpr StackVec<T, capacity>::operator Span<T const>() noexcept
	{
		return ToSpan();
	}

	template<typename T, uSize capacity>
	bool StackVec<T, capacity>::CanPushBack() const noexcept
	{
		return size < capacity;
	}

	template<typename T, uSize capacity>
	constexpr uSize StackVec<T, capacity>::Capacity() noexcept
	{
		return capacity;
	}

	template<typename T, uSize capacity>
	T* StackVec<T, capacity>::Data() noexcept
	{
		return values;
	}

	template<typename T, uSize capacity>
	T const* StackVec<T, capacity>::Data() const noexcept
	{
		return values;
	}

	template<typename T, uSize capacity>
	bool StackVec<T, capacity>::IsEmpty() const noexcept
	{
		return size == 0;
	}

	template<typename T, uSize capacity>
	uSize StackVec<T, capacity>::Size() const noexcept
	{
		return size;
	}

	template<typename T, uSize capacity>
	void StackVec<T, capacity>::Clear()
	{
		for (uSize i = 0; i < size; i += 1)
			values[i].~T();
		size = 0;
	}

	template<typename T, uSize capacity>
	void StackVec<T, capacity>::Erase(uSize index)
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			index < size,
			"Attempted to .Erase() a StackVec with an out-of-bounds index.");
		for (uSize i = index; i < size - 1; i += 1)
			values[i] = static_cast<T&&>(values[i + 1]);
		values[size - 1].~T();
		size -= 1;
	}

	template<typename T, uSize capacity>
	void StackVec<T, capacity>::EraseUnsorted(uSize index)
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			index < size,
			"Attempted to .EraseUnsorted() a StackVec with an out-of-bounds index.");
		values[index] = static_cast<T&&>(values[size - 1]);
		values[size - 1].~T();
		size -= 1;
	}

	template<typename T, uSize capacity>
	void StackVec<T, capacity>::Insert(uSize index, T&& in)
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			index < size,
			"Attempted to .Insert() a StackVec at an out-of-bounds index.");
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			CanPushBack(),
			"Attempted to .Insert() a StackVec at max capacity.");
		size += 1;
		// Move elements to one index later
		for (uSize i = size - 1; i > index; i -= 1)
			new(&values[i]) T(static_cast<T&&>(values[i - 1]));
		new(&values[index]) T(static_cast<T&&>(in));
	}

	template<typename T, uSize capacity>
	void StackVec<T, capacity>::PushBack(T const& in)
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			CanPushBack(),
			"Attempted to .PushBack() a StackVec at max capacity.");
		new(values + size) T(in);
		size += 1;
	}

	template<typename T, uSize capacity>
	void StackVec<T, capacity>::PushBack(T&& in)
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			CanPushBack(),
			"Attempted to .PushBack() a StackVec at max capacity.");
		new(values + size) T(static_cast<T&&>(in));
		size += 1;
	}

	template<typename T, uSize capacity>
	void StackVec<T, capacity>::Resize(uSize newSize)
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			size <= capacity,
			"Attempted to .Resize() a StackVec beyond it's capacity.");
		if (newSize > size)
		{
			for (uSize i = size; i < newSize; i += 1)
				new(values + i) T();
		}
		else
		{
			for (uSize i = newSize; i < size; i += 1)
				values[i].~T();
		}
		size = newSize;
	}

	template<typename T, uSize capacity>
	T& StackVec<T, capacity>::At(uSize i) noexcept
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			i < size,
			"Attempted to .At() a StackVec with an out-of-bounds index.");
		return values[i];
	}

	template<typename T, uSize capacity>
	T const& StackVec<T, capacity>::At(uSize i) const noexcept
	{
		DENGINE_DETAIL_CONTAINERS_ASSERT_MSG(
			i < size,
			"Attempted to .At() a StackVec with an out-of-bounds index.");
		return values[i];
	}

	template<typename T, uSize capacity>
	T& StackVec<T, capacity>::operator[](uSize i) noexcept
	{
		return At(i);
	}

	template<typename T, uSize capacity>
	T const& StackVec<T, capacity>::operator[](uSize i) const noexcept
	{
		return At(i);
	}

	template<typename T, uSize capacity>
	T* StackVec<T, capacity>::begin() noexcept
	{
		if (size == 0)
			return nullptr;
		else
			return values;
	}

	template<typename T, uSize capacity>
	T const* StackVec<T, capacity>::begin() const noexcept
	{
		if (size == 0)
			return nullptr;
		else
			return values;
	}

	template<typename T, uSize capacity>
	T* StackVec<T, capacity>::end() noexcept
	{
		if (size == 0)
			return nullptr;
		else
			return values + size;
	}

	template<typename T, uSize capacity>
	T const* StackVec<T, capacity>::end() const noexcept
	{
		if (size == 0)
			return nullptr;
		else
			return values + size;
	}
}