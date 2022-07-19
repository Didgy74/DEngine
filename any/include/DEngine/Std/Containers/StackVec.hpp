#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Containers/Span.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>

#include <initializer_list>

namespace DEngine::Std
{
	// Constant-capacity vector.
	// Use for small vectors of fixed capacity.
	template<class T, unsigned int capacity>
	class StackVec
	{
	public:
		using ValueType = T;

		StackVec() noexcept;
		StackVec(uSize length);
		StackVec(StackVec const& in) noexcept
			requires (Trait::isCopyConstructible<T>)
			: size(in.size)
		{
			for (int i = 0; i < size; i++)
				new(values + i) T(in[i]);
		}
		StackVec(StackVec&& in) noexcept
			requires (Trait::isMoveConstructible<T>)
			: size(in.size)
		{
			for (int i = 0; i < size; i++)
				new(values + i) T(static_cast<T&&>(in[i]));
			in.Clear();
		}
		template<unsigned int otherCap>
		StackVec(StackVec<T, otherCap> const& in) noexcept
			requires (Trait::isCopyConstructible<T> && otherCap < capacity)
			: size{ otherCap }
		{
			for (int i = 0; i < otherCap; i++)
				new(values + i) T(in[i]);
		}
		StackVec(std::initializer_list<T> const& in) noexcept
			requires (Trait::isCopyConstructible<T>)
			: size{ in.size() }
		{
			DENGINE_IMPL_CONTAINERS_ASSERT(in.size());
			int i = 0;
			for (auto const& inItem : in) {
				new(values + i) T(inItem);
				i++;
			}
		}
		~StackVec();

		StackVec<T, capacity>& operator=(StackVec<T, capacity> const&);
		StackVec<T, capacity>& operator=(StackVec<T, capacity>&&) noexcept;

		[[nodiscard]] constexpr Span<T> ToSpan() noexcept;
		[[nodiscard]] constexpr Span<T const> ToSpan() const noexcept;
		constexpr operator Span<T>() noexcept;
		constexpr operator Span<T const>() const noexcept;

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
			alignas(T) unsigned char unused;
			T values[capacity];
		};
		uSize size = 0;
	};

	template<class T, unsigned int capacity>
	StackVec<T, capacity>::StackVec() noexcept {}

	template<class T, unsigned int capacity>
	StackVec<T, capacity>::~StackVec() {
		Clear();
	}

	template<class T, unsigned int capacity>
	StackVec<T, capacity>& StackVec<T, capacity>::operator=(StackVec<T, capacity> const& other)
	{
		if (this == &other)
			return *this;

		int i = 0;
		for (; i < (size < other.size ? size : other.size); i++)
			values[i] = other[i];
		for (; i < size; i++)
			values[i].~T();
		for (; i < other.size; i++)
			new(values + i) T(other[i]);

		size = other.size;

		return *this;
	}

	template<class T, unsigned int capacity>
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

	template<class T, unsigned int capacity>
	constexpr Span<T> StackVec<T, capacity>::ToSpan() noexcept {
		return Span<T>(values, size);
	}

	template<class T, unsigned int capacity>
	constexpr Span<T const> StackVec<T, capacity>::ToSpan() const noexcept {
		return Span<T const>(values, size);
	}

	template<class T, unsigned int capacity>
	constexpr StackVec<T, capacity>::operator Span<T>() noexcept {
		return ToSpan();
	}

	template<class T, unsigned int capacity>
	constexpr StackVec<T, capacity>::operator Span<T const>() const noexcept {
		return ToSpan();
	}

	template<class T, unsigned int capacity>
	bool StackVec<T, capacity>::CanPushBack() const noexcept {
		return size < capacity;
	}

	template<class T, unsigned int capacity>
	constexpr uSize StackVec<T, capacity>::Capacity() noexcept {
		return capacity;
	}

	template<class T, unsigned int capacity>
	T* StackVec<T, capacity>::Data() noexcept {
		return values;
	}

	template<class T, unsigned int capacity>
	T const* StackVec<T, capacity>::Data() const noexcept {
		return values;
	}

	template<class T, unsigned int capacity>
	bool StackVec<T, capacity>::IsEmpty() const noexcept {
		return size == 0;
	}

	template<class T, unsigned int capacity>
	uSize StackVec<T, capacity>::Size() const noexcept {
		return size;
	}

	template<class T, unsigned int capacity>
	void StackVec<T, capacity>::Clear() {
		if constexpr (!Trait::isTriviallyDestructible<T>) {
			for (uSize i = 0; i < size; i += 1)
				values[i].~T();
		}
		size = 0;
	}

	template<class T, unsigned int capacity>
	void StackVec<T, capacity>::Erase(uSize index)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			index < size,
			"Attempted to .Erase() a StackVec with an out-of-bounds index.");
		for (uSize i = index; i < size - 1; i += 1)
			values[i] = static_cast<T&&>(values[i + 1]);
		if constexpr (!Trait::isTriviallyDestructible<T>)
			values[size - 1].~T();
		size -= 1;
	}

	template<class T, unsigned int capacity>
	void StackVec<T, capacity>::EraseUnsorted(uSize index)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			index < size,
			"Attempted to .EraseUnsorted() a StackVec with an out-of-bounds index.");
		values[index] = static_cast<T&&>(values[size - 1]);
		values[size - 1].~T();
		size -= 1;
	}

	template<class T, unsigned int capacity>
	void StackVec<T, capacity>::Insert(uSize index, T&& in)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			index < size,
			"Attempted to .Insert() a StackVec at an out-of-bounds index.");
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			CanPushBack(),
			"Attempted to .Insert() a StackVec at max capacity.");
		size += 1;
		// Move elements to one index later
		for (uSize i = size - 1; i > index; i -= 1)
			new(&values[i]) T(static_cast<T&&>(values[i - 1]));
		new(&values[index]) T(static_cast<T&&>(in));
	}

	template<class T, unsigned int capacity>
	void StackVec<T, capacity>::PushBack(T const& in)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			CanPushBack(),
			"Attempted to .PushBack() a StackVec at max capacity.");
		new(values + size) T(in);
		size += 1;
	}

	template<class T, unsigned int capacity>
	void StackVec<T, capacity>::PushBack(T&& in)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			CanPushBack(),
			"Attempted to .PushBack() a StackVec at max capacity.");
		new(values + size) T(static_cast<T&&>(in));
		size += 1;
	}

	template<class T, unsigned int capacity>
	void StackVec<T, capacity>::Resize(uSize newSize)
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			size <= capacity,
			"Attempted to .Resize() a StackVec beyond it's capacity.");
		if (newSize > size) {
			for (int i = size; i < newSize; i += 1)
				new(values + i) T();
		} else {
			for (int i = newSize; i < size; i += 1)
				values[i].~T();
		}
		size = newSize;
	}

	template<class T, unsigned int capacity>
	T& StackVec<T, capacity>::At(uSize i) noexcept
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			i < size,
			"Attempted to .At() a StackVec with an out-of-bounds index.");
		return values[i];
	}

	template<class T, unsigned int capacity>
	T const& StackVec<T, capacity>::At(uSize i) const noexcept
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			i < size,
			"Attempted to .At() a StackVec with an out-of-bounds index.");
		return values[i];
	}

	template<class T, unsigned int capacity>
	T& StackVec<T, capacity>::operator[](uSize i) noexcept {
		return At(i);
	}

	template<class T, unsigned int capacity>
	T const& StackVec<T, capacity>::operator[](uSize i) const noexcept {
		return At(i);
	}

	template<class T, unsigned int capacity>
	T* StackVec<T, capacity>::begin() noexcept {
		if (size == 0)
			return nullptr;
		else
			return values;
	}

	template<class T, unsigned int capacity>
	T const* StackVec<T, capacity>::begin() const noexcept
	{
		if (size == 0)
			return nullptr;
		else
			return values;
	}

	template<class T, unsigned int capacity>
	T* StackVec<T, capacity>::end() noexcept
	{
		if (size == 0)
			return nullptr;
		else
			return values + size;
	}

	template<class T, unsigned int capacity>
	T const* StackVec<T, capacity>::end() const noexcept
	{
		if (size == 0)
			return nullptr;
		else
			return values + size;
	}
}