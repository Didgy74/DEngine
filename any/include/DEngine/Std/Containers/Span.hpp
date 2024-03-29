#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Range.hpp>

#include <DEngine/Std/Containers/impl/Assert.hpp>

namespace DEngine::Std
{
	namespace impl {
		template<class T>
		struct Span_IsConst { static constexpr bool value = false; };
		template<class T>
		struct Span_IsConst<T const> { static constexpr bool value = true; };
		template<class Type>
		struct Span_RemoveConst_Struct { using T = Type; };
		template<class Type>
		struct Span_RemoveConst_Struct<Type const> { using T = Type; };
		template<class T>
		using Span_RemoveConst = typename Span_RemoveConst_Struct<T>::T;
		template<class T>
		struct Span_isChar { static constexpr bool value = false; };
		template<>
		struct Span_isChar<char> { static constexpr bool value = true; };
	}

	template<class T>
	class Span
	{
	public:
		using ValueType = impl::Span_RemoveConst<T>;
		static constexpr bool typeIsConst = impl::Span_IsConst<T>::value;
		static constexpr bool isByteSpan = impl::Span_isChar<ValueType>::value;

		constexpr Span() noexcept = default;
		constexpr Span(T* data, uSize size) noexcept;
		// Deleted so the compiler can not implicitly convert integer to nullptr
		constexpr Span(int, uSize) = delete;
		// Testing some stuff
		//template<int N>
		//constexpr Span(T (&arr)[N]) noexcept;
		constexpr Span(T& value) noexcept : m_data{ &value }, m_size{ 1 } {}

		[[nodiscard]] constexpr Span<T const> ConstSpan() const noexcept;
		constexpr operator Span<T const>() const noexcept;

		[[nodiscard]] constexpr Span<char const> ToConstByteSpan() const noexcept;

		[[nodiscard]] constexpr Range<T*> AsRange() const noexcept;
		constexpr operator Range<T*>() const noexcept;

		[[nodiscard]] constexpr Span Subspan(uSize offset, uSize count) const noexcept {
			DENGINE_IMPL_CONTAINERS_ASSERT(offset + count <= m_size);
			return { m_data + offset, count };
		}

		template<class U>
		[[nodiscard]] Std::Span<U> Cast() const requires (isByteSpan) {
			DENGINE_IMPL_CONTAINERS_ASSERT((uSize)m_data % alignof(U) == 0);
			DENGINE_IMPL_CONTAINERS_ASSERT(m_size % sizeof(U) == 0);
			return {
				reinterpret_cast<U*>(m_data),
				m_size / sizeof(U) };
		}
		[[nodiscard]] T* Data() const noexcept;
		[[nodiscard]] uSize Size() const noexcept;
		[[nodiscard]] bool Empty() const noexcept;

		[[nodiscard]] T& At(uSize i) const;
		[[nodiscard]] T& operator[](uSize i) const;

		[[nodiscard]] T* begin() const noexcept;
		[[nodiscard]] T* end() const noexcept;

	protected:
		T* m_data = nullptr;
		uSize m_size = 0;
	};

	using ByteSpan = Std::Span<char>;
	using ConstByteSpan = Std::Span<char const>;

	/*
	class ByteSpan
	{
	public:
		ByteSpan() = default;
		ByteSpan(void* ptr, uSize size) : m_data{ ptr }, m_size{ size } {}

		[[nodiscard]] char* Data() const noexcept { return static_cast<char*>(m_data); }
		[[nodiscard]] uSize Size() const noexcept { return m_size; }

		[[nodiscard]] ByteSpan Offset(uSize offset) const noexcept;

		template<class T>
		[[nodiscard]] Std::Span<T> Cast() const noexcept;
	private:
		void* m_data = nullptr;
		uSize m_size = 0;
	};
	 */

	template<class T>
	constexpr Span<T>::Span(T* data, uSize size) noexcept :
		m_data(data),
		m_size(size)
	{}

	template<class T>
	constexpr Span<T const> Span<T>::ConstSpan() const noexcept { return Span<T const>(m_data, m_size); }

	template<class T>
	constexpr Span<T>::operator Span<T const>() const noexcept { return ConstSpan(); }

	template<class T>
	constexpr Span<char const> Span<T>::ToConstByteSpan() const noexcept {
		return {
			reinterpret_cast<char const*>(m_data),
			m_size * sizeof(T) };
	}

	template<class T>
	constexpr Range<T*> Span<T>::AsRange() const noexcept
	{
		Range<T*> returnVal;
		returnVal.begin = begin();
		returnVal.end = end();
		return returnVal;
	}

	template<class T>
	T* Span<T>::Data() const noexcept { return m_data; }

	template<class T>
	uSize Span<T>::Size() const noexcept { return m_size; }

	template<class T>
	bool Span<T>::Empty() const noexcept { return m_data == nullptr || m_size == 0; }

	template<class T>
	T& Span<T>::At(uSize i) const
	{
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			m_data != nullptr,
			"Tried to .At() a Span with data pointer set to nullptr.");
		DENGINE_IMPL_CONTAINERS_ASSERT_MSG(
			i < m_size,
			"Tried to .At() a Span with index out of bounds.");
		return m_data[i];
	}

	template<class T>
	T& Span<T>::operator[](uSize i) const { return At(i); }

	template<class T>
	T* Span<T>::begin() const noexcept { return m_data; }

	template<class T>
	T* Span<T>::end() const noexcept { return m_data + m_size; }
	/*
	template<class T>
	[[nodiscard]] Span<T> ByteSpan::Cast() const noexcept
	{
		DENGINE_IMPL_CONTAINERS_ASSERT((uSize)(static_cast<char*>(m_data) + m_size) % alignof(T) == 0);
		DENGINE_IMPL_CONTAINERS_ASSERT(m_size % sizeof(T) == 0);
		return {
			static_cast<T*>(m_data),
			m_size / sizeof(T) };
	}
	 */

	// Pass in string literals
	template<unsigned N>
	[[nodiscard]] consteval Span<char const> CStrToSpan(char const (&in)[N]) noexcept { return { in, N - 1 }; }
}