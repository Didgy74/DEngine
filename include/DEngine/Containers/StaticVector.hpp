#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Containers/Span.hpp>
#include <DEngine/Containers/Optional.hpp>

#include <stdexcept>
#include <new>

namespace DEngine::Std
{
	// Constant-capacity vector.
	// Use for small vectors of fixed capacity.
	template<typename T, uSize maxLength>
	class StaticVector
	{
	private:
		union
		{
			alignas(T) unsigned char unused = 0;
			T values[maxLength];
		};
		
		uSize size = 0;

	public:
		using ValueType = T;

		StaticVector();
		StaticVector(uSize length);
		StaticVector(StaticVector<T, maxLength> const&);
		StaticVector(StaticVector<T, maxLength>&&) noexcept;
		~StaticVector();

		StaticVector<T, maxLength>& operator=(StaticVector<T, maxLength> const&);
		StaticVector<T, maxLength>& operator=(StaticVector<T, maxLength>&&) noexcept;

		[[nodiscard]] constexpr Span<T> ToSpan() noexcept;
		[[nodiscard]] constexpr Span<T const> ToSpan() const noexcept;
		constexpr operator Span<T>() noexcept;
		constexpr operator Span<T const>() noexcept;

		[[nodiscard]] T* Data();
		[[nodiscard]] T const* Data() const;
		[[nodiscard]] static constexpr uSize Capacity();
		[[nodiscard]] uSize Size() const;
		[[nodiscard]] bool IsEmpty() const;

		bool CanPushBack() const noexcept;
		void Clear();
		void Erase(uSize index);
		// Erases the element but does not retain the order of the vector.
		void EraseUnsorted(uSize index);
		void Insert(uSize index, T&& in);
		void PushBack(T const& in);
		void PushBack(T&& in);
		void Resize(uSize newSize);

		template<typename Callable>
		[[nodiscard]] Std::Optional<uSize> FindIf(Callable&& test) const;

		[[nodiscard]] T& At(uSize i);
		[[nodiscard]] T const& At(uSize i) const;
		[[nodiscard]] T& operator[](uSize i);
		[[nodiscard]] T const& operator[](uSize i) const;

		[[nodiscard]] T* begin();
		[[nodiscard]] T const* begin() const;
		[[nodiscard]] T* end();
		[[nodiscard]] T const* end() const;
	};

	template<typename T, uSize maxLength>
	StaticVector<T, maxLength>::StaticVector() {}

	template<typename T, uSize maxLength>
	StaticVector<T, maxLength>::StaticVector(uSize length) :
		size(length)
	{
		if (length > maxLength)
			throw std::out_of_range("Attempted to create a StaticVector with length higher than maxSize().");
		new(values) T[length];
	}

	template<typename T, uSize maxLength>
	StaticVector<T, maxLength>::StaticVector(StaticVector<T, maxLength> const& other) :
		size(other.size)
	{
		for (uSize i = 0; i < size; i += 1)
			new(values + i) T(other[i]);
	}

	template<typename T, uSize maxLength>
	StaticVector<T, maxLength>::StaticVector(StaticVector<T, maxLength>&& other) noexcept :
		size(other.size)
	{
		for (uSize i = 0; i < size; i += 1)
			new(values + i) T(static_cast<T&&>(other[i]));
		other.Clear();
	}

	template<typename T, uSize maxLength>
	StaticVector<T, maxLength>::~StaticVector()
	{
		Clear();
	}

	template<typename T, uSize maxLength>
	StaticVector<T, maxLength>& StaticVector<T, maxLength>::operator=(StaticVector<T, maxLength> const& other)
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

	template<typename T, uSize maxLength>
	StaticVector<T, maxLength>& StaticVector<T, maxLength>::operator=(StaticVector<T, maxLength>&& other) noexcept
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

	template<typename T, uSize maxLength>
	constexpr Span<T> StaticVector<T, maxLength>::ToSpan() noexcept
	{
		return Span<T>(values, size);
	}

	template<typename T, uSize maxLength>
	constexpr Span<T const> StaticVector<T, maxLength>::ToSpan() const noexcept
	{
		return Span<T const>(values, size);
	}

	template<typename T, uSize maxLength>
	constexpr StaticVector<T, maxLength>::operator Span<T>() noexcept
	{
		return ToSpan();
	}

	template<typename T, uSize maxLength>
	constexpr StaticVector<T, maxLength>::operator Span<T const>() noexcept
	{
		return ToSpan();
	}

	template<typename T, uSize maxLength>
	T* StaticVector<T, maxLength>::Data()
	{
		return values;
	}

	template<typename T, uSize maxLength>
	T const* StaticVector<T, maxLength>::Data() const
	{
		return values;
	}

	template<typename T, uSize maxLength>
	constexpr uSize StaticVector<T, maxLength>::Capacity()
	{
		return maxLength;
	}

	template<typename T, uSize maxLength>
	uSize StaticVector<T, maxLength>::Size() const
	{
		return size;
	}

	template<typename T, uSize maxLength>
	bool StaticVector<T, maxLength>::IsEmpty() const
	{
		return size == 0;
	}

	template<typename T, uSize maxLength>
	bool StaticVector<T, maxLength>::CanPushBack() const noexcept
	{
		return size < maxLength;
	}

	template<typename T, uSize maxLength>
	void StaticVector<T, maxLength>::Clear()
	{
		for (uSize i = 0; i < size; i += 1)
			values[i].~T();
		size = 0;
	}

	template<typename T, uSize maxLength>
	void StaticVector<T, maxLength>::Erase(uSize index)
	{
		if (index >= size)
			throw std::out_of_range("Attempted to .Erase() a StaticVector with an out-of-bounds index.");
		values[index].~T();
		for (uSize i = index + 1; i < size; i += 1)
			new(&values[i - 1]) T(static_cast<T&&>(values[i]));
		size -= 1;
	}

	template<typename T, uSize maxLength>
	void StaticVector<T, maxLength>::EraseUnsorted(uSize index)
	{
		if (index >= size)
			throw std::out_of_range("Attempted to .EraseUnsorted() a StaticVector with an out-of-bounds index.");
		values[index].~T();
		if (size > 0)
		{
			new(&values[index]) T(static_cast<T&&>(values[size - 1]));
			values[size - 1].~T();
			size -= 1;
		}
	}

	template<typename T, uSize maxLength>
	void StaticVector<T, maxLength>::Insert(uSize index, T&& in)
	{
		if (index >= size)
			throw std::out_of_range("Attempted to .Insert() a StaticVector at an out-of-bounds index.");
		else if (!CanPushBack())
			throw std::out_of_range("Attempted to .Insert() a StaticVector at max capacity.");
		size += 1;
		// Move elements to one index later
		for (uSize i = size - 1; i > index; i -= 1)
			new(&values[i]) T(static_cast<T&&>(values[i - 1]));
		new(&values[index]) T(static_cast<T&&>(in));
	}

	template<typename T, uSize maxLength>
	void StaticVector<T, maxLength>::PushBack(T const& in)
	{
		if (!CanPushBack())
			throw std::out_of_range("Attempted to .PushBack() a StaticVector when it is already at max capacity.");
		new(values + size) T(in);
		size += 1;
	}

	template<typename T, uSize maxLength>
	void StaticVector<T, maxLength>::PushBack(T&& in)
	{
		if (!CanPushBack())
			throw std::out_of_range("Attempted to .PushBack() a StaticVector when it is already at max capacity.");
		new(values + size) T(static_cast<T&&>(in));
		size += 1;
	}

	template<typename T, uSize maxLength>
	void StaticVector<T, maxLength>::Resize(uSize newSize)
	{
		if (size >= maxLength)
			throw std::out_of_range("Attempted to .resize() a StaticVector beyond what it can maximally hold.");
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

	template<typename T, uSize maxLength>
	template<typename Callable>
	Std::Optional<uSize> StaticVector<T, maxLength>::FindIf(Callable&& test) const
	{
		for (uSize i = 0; i < Size(); i += 1)
		{
			if (test(values[i]))
				return i;
		}
		return {};
	}

	template<typename T, uSize maxLength>
	T& StaticVector<T, maxLength>::At(uSize i)
	{
		if (i >= size)
			throw std::out_of_range("Attempted to .At() a StaticVector with an index out of bounds.");
		return values[i];
	}

	template<typename T, uSize maxLength>
	T const& StaticVector<T, maxLength>::At(uSize i) const
	{
		if (i >= size)
			throw std::out_of_range("Attempted to .At() a StaticVector with an index out of bounds.");
		return values[i];
	}

	template<typename T, uSize maxLength>
	T& StaticVector<T, maxLength>::operator[](uSize i)
	{
		return At(i);
	}

	template<typename T, uSize maxLength>
	T const& StaticVector<T, maxLength>::operator[](uSize i) const
	{
		return At(i);
	}

	template<typename T, uSize maxLength>
	T* StaticVector<T, maxLength>::begin()
	{
		if (size == 0)
			return nullptr;
		else
			return values;
	}

	template<typename T, uSize maxLength>
	T const* StaticVector<T, maxLength>::begin() const
	{
		if (size == 0)
			return nullptr;
		else
			return values;
	}

	template<typename T, uSize maxLength>
	T* StaticVector<T, maxLength>::end()
	{
		if (size == 0)
			return nullptr;
		else
			return values + size;
	}

	template<typename T, uSize maxLength>
	T const* StaticVector<T, maxLength>::end() const
	{
		if (size == 0)
			return nullptr;
		else
			return values + size;
	}
}