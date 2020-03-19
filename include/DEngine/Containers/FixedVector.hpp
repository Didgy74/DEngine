#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Span.hpp"

#include <stdexcept>
#include <new>

namespace DEngine::Containers
{
    template<typename T, uSize maxLength>
    class FixedVector
    {
    private:
        union
        {
            alignas(T) char m_unused = 0;
            T m_values[maxLength];
        };
        
        uSize m_size = 0;

    public:
        FixedVector();
        FixedVector(uSize length);
        ~FixedVector();

        [[nodiscard]] constexpr Span<T> ToSpan() noexcept;
        [[nodiscard]] constexpr Span<T const> ToSpan() const noexcept;
        constexpr operator Span<T>() noexcept;
        constexpr operator Span<T const>() noexcept;

        [[nodiscard]] T* Data();
        [[nodiscard]] T const* Data() const;
        [[nodiscard]] constexpr uSize Capacity() const;
        [[nodiscard]] uSize Size() const;

        void Clear();
        void Resize(uSize newSize);
        bool CanPushBack() const noexcept;
        void PushBack(T const& in);
        void PushBack(T&& in);

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
    FixedVector<T, maxLength>::FixedVector() :
        m_size(0),
        m_unused()
    {

    }

    template<typename T, uSize maxLength>
    FixedVector<T, maxLength>::FixedVector(uSize length) :
        m_size(length)
    {
        if (length > maxLength)
            throw std::out_of_range("Attempted to create a FixedVector with length higher than maxSize().");
        new(m_values) T[length];
    }

    template<typename T, uSize maxLength>
    FixedVector<T, maxLength>::~FixedVector()
    {
        Clear();
    }

    template<typename T, uSize maxLength>
    constexpr Span<T> FixedVector<T, maxLength>::ToSpan() noexcept
    {
        return Span<T>(m_values, m_size);
    }

    template<typename T, uSize maxLength>
    constexpr Span<T const> FixedVector<T, maxLength>::ToSpan() const noexcept
    {
        return Span<T const>(m_values, m_size);
    }

    template<typename T, uSize maxLength>
    constexpr FixedVector<T, maxLength>::operator Span<T>() noexcept
    {
        return ToSpan();
    }

    template<typename T, uSize maxLength>
    constexpr FixedVector<T, maxLength>::operator Span<T const>() noexcept
    {
        return ToSpan();
    }

    template<typename T, uSize maxLength>
    T* FixedVector<T, maxLength>::Data()
    {
        return m_values;
    }

    template<typename T, uSize maxLength>
    T const* FixedVector<T, maxLength>::Data() const
    {
        return m_values;
    }

    template<typename T, uSize maxLength>
    constexpr uSize FixedVector<T, maxLength>::Capacity() const
    {
        return maxLength;
    }

    template<typename T, uSize maxLength>
    uSize FixedVector<T, maxLength>::Size() const
    {
        return m_size;
    }

    template<typename T, uSize maxLength>
    void FixedVector<T, maxLength>::Clear()
    {
        for (uSize i = m_size; i > 0; i -= 1)
            m_values[i - 1].~T();
        m_size = 0;
    }

    template<typename T, uSize maxLength>
    void FixedVector<T, maxLength>::Resize(uSize newSize)
    {
        if (m_size >= maxLength)
            throw std::out_of_range("Attempted to .resize() a FixedVector beyond what it can maximally hold.");
        if (newSize > m_size)
        {
            for (uSize i = m_size; i < newSize; i += 1)
                new(m_values + i) T();
        }
        else
        {
            for (uSize i = m_size - 1; i >= newSize; i -= 1)
                m_values[i].~T();
        }
        m_size = newSize;
    }

    template<typename T, uSize maxLength>
    bool FixedVector<T, maxLength>::CanPushBack() const noexcept
    {
        return m_size < maxLength;
    }

    template<typename T, uSize maxLength>
    void FixedVector<T, maxLength>::PushBack(T const& in)
    {
        if (!CanPushBack())
            throw std::out_of_range("Attempted to .PushBack() a FixedVector when it is already at max capacity.");
        new(m_values + m_size) T(in);
        m_size += 1;
    }

    template<typename T, uSize maxLength>
    void FixedVector<T, maxLength>::PushBack(T&& in)
    {
        if (!CanPushBack())
            throw std::out_of_range("Attempted to .PushBack() a FixedVector when it is already at max capacity.");
        new(m_values + m_size) T(static_cast<T&&>(in));
        m_size += 1;
    }

    template<typename T, uSize maxLength>
    T& FixedVector<T, maxLength>::At(uSize i)
    {
        if (i >= m_size)
            throw std::out_of_range("Attempted to .At() a FixedVector with an index out of bounds.");
        return m_values[i];
    }

    template<typename T, uSize maxLength>
    T const& FixedVector<T, maxLength>::At(uSize i) const
    {
        if (i >= m_size)
            throw std::out_of_range("Attempted to .At() a FixedVector with an index out of bounds.");
        return m_values[i];
    }

    template<typename T, uSize maxLength>
    T& FixedVector<T, maxLength>::operator[](uSize i)
    {
        return At(i);
    }

    template<typename T, uSize maxLength>
    T const& FixedVector<T, maxLength>::operator[](uSize i) const
    {
        return At(i);
    }

    template<typename T, uSize maxLength>
    T* FixedVector<T, maxLength>::begin()
    {
        if (m_size == 0)
            return nullptr;
        else
            return m_values;
    }

    template<typename T, uSize maxLength>
    T const* FixedVector<T, maxLength>::begin() const
    {
        if (m_size == 0)
            return nullptr;
        else
            return m_values;
    }

    template<typename T, uSize maxLength>
    T* FixedVector<T, maxLength>::end()
    {
        if (m_size == 0)
            return nullptr;
        else
            return m_values + m_size;
    }

    template<typename T, uSize maxLength>
    T const* FixedVector<T, maxLength>::end() const
    {
        if (m_size == 0)
            return nullptr;
        else
            return m_values + m_size;
    }
}


namespace DEngine
{
    namespace Cont = Containers;
}