#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Span.hpp"
#include "DEngine/Containers/Optional.hpp"

#include <stdexcept>
#include <new>

namespace DEngine::Std
{
    template<typename T, uSize maxLength>
    class StaticVector
    {
    private:
        union
        {
            alignas(T) char m_unused = 0;
            T m_values[maxLength];
        };
        
        uSize m_size = 0;

    public:
        using ValueType = T;

        StaticVector();
        StaticVector(uSize length);
        ~StaticVector();

        [[nodiscard]] constexpr Span<T> ToSpan() noexcept;
        [[nodiscard]] constexpr Span<T const> ToSpan() const noexcept;
        constexpr operator Span<T>() noexcept;
        constexpr operator Span<T const>() noexcept;

        [[nodiscard]] T* Data();
        [[nodiscard]] T const* Data() const;
        [[nodiscard]] constexpr uSize Capacity() const;
        [[nodiscard]] uSize Size() const;

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
    StaticVector<T, maxLength>::StaticVector() :
        m_size(0),
        m_unused()
    {

    }

    template<typename T, uSize maxLength>
    StaticVector<T, maxLength>::StaticVector(uSize length) :
        m_size(length)
    {
        if (length > maxLength)
            throw std::out_of_range("Attempted to create a StaticVector with length higher than maxSize().");
        new(m_values) T[length];
    }

    template<typename T, uSize maxLength>
    StaticVector<T, maxLength>::~StaticVector()
    {
        Clear();
    }

    template<typename T, uSize maxLength>
    constexpr Span<T> StaticVector<T, maxLength>::ToSpan() noexcept
    {
        return Span<T>(m_values, m_size);
    }

    template<typename T, uSize maxLength>
    constexpr Span<T const> StaticVector<T, maxLength>::ToSpan() const noexcept
    {
        return Span<T const>(m_values, m_size);
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
        return m_values;
    }

    template<typename T, uSize maxLength>
    T const* StaticVector<T, maxLength>::Data() const
    {
        return m_values;
    }

    template<typename T, uSize maxLength>
    constexpr uSize StaticVector<T, maxLength>::Capacity() const
    {
        return maxLength;
    }

    template<typename T, uSize maxLength>
    uSize StaticVector<T, maxLength>::Size() const
    {
        return m_size;
    }

    template<typename T, uSize maxLength>
    bool StaticVector<T, maxLength>::CanPushBack() const noexcept
    {
        return m_size < maxLength;
    }

    template<typename T, uSize maxLength>
    void StaticVector<T, maxLength>::Clear()
    {
        for (uSize i = m_size; i > 0; i -= 1)
            m_values[i - 1].~T();
        m_size = 0;
    }

    template<typename T, uSize maxLength>
    void StaticVector<T, maxLength>::Erase(uSize index)
    {
        if (index >= m_size)
            throw std::out_of_range("Attempted to .Erase() a StaticVector with an out-of-bounds index.");
        m_values[index].~T();
        for (uSize i = index + 1; i < m_size; i += 1)
            new(&m_values[i - 1]) T(static_cast<T&&>(m_values[i]));
        m_size -= 1;
    }

    template<typename T, uSize maxLength>
    void StaticVector<T, maxLength>::EraseUnsorted(uSize index)
    {
        if (index >= m_size)
            throw std::out_of_range("Attempted to .EraseUnsorted() a StaticVector with an out-of-bounds index.");
        m_values[index].~T();
        if (m_size > 0)
        {
            new(&m_values[index]) T(static_cast<T&&>(m_values[m_size - 1]));
            m_values[m_size - 1].~T();
            m_size -= 1;
        }
    }

    template<typename T, uSize maxLength>
    void StaticVector<T, maxLength>::Insert(uSize index, T&& in)
    {
        if (index >= m_size)
            throw std::out_of_range("Attempted to .Insert() a StaticVector at an out-of-bounds index.");
        else if (!CanPushBack())
            throw std::out_of_range("Attempted to .Insert() a StaticVector at max capacity.");
        m_size += 1;
        // Move elements to one index later
        for (uSize i = m_size - 1; i > index; i -= 1)
            new(&m_values[i]) T(static_cast<T&&>(m_values[i - 1]));
        new(&m_values[index]) T(static_cast<T&&>(in));
    }

    template<typename T, uSize maxLength>
    void StaticVector<T, maxLength>::PushBack(T const& in)
    {
        if (!CanPushBack())
            throw std::out_of_range("Attempted to .PushBack() a StaticVector when it is already at max capacity.");
        new(m_values + m_size) T(in);
        m_size += 1;
    }

    template<typename T, uSize maxLength>
    void StaticVector<T, maxLength>::PushBack(T&& in)
    {
        if (!CanPushBack())
            throw std::out_of_range("Attempted to .PushBack() a StaticVector when it is already at max capacity.");
        new(m_values + m_size) T(static_cast<T&&>(in));
        m_size += 1;
    }

    template<typename T, uSize maxLength>
    void StaticVector<T, maxLength>::Resize(uSize newSize)
    {
        if (m_size >= maxLength)
            throw std::out_of_range("Attempted to .resize() a StaticVector beyond what it can maximally hold.");
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
    template<typename Callable>
    Std::Optional<uSize> StaticVector<T, maxLength>::FindIf(Callable&& test) const
    {
        for (uSize i = 0; i < Size(); i += 1)
        {
            if (test(m_values[i]))
                return i;
        }
        return {};
    }

    template<typename T, uSize maxLength>
    T& StaticVector<T, maxLength>::At(uSize i)
    {
        if (i >= m_size)
            throw std::out_of_range("Attempted to .At() a StaticVector with an index out of bounds.");
        return m_values[i];
    }

    template<typename T, uSize maxLength>
    T const& StaticVector<T, maxLength>::At(uSize i) const
    {
        if (i >= m_size)
            throw std::out_of_range("Attempted to .At() a StaticVector with an index out of bounds.");
        return m_values[i];
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
        if (m_size == 0)
            return nullptr;
        else
            return m_values;
    }

    template<typename T, uSize maxLength>
    T const* StaticVector<T, maxLength>::begin() const
    {
        if (m_size == 0)
            return nullptr;
        else
            return m_values;
    }

    template<typename T, uSize maxLength>
    T* StaticVector<T, maxLength>::end()
    {
        if (m_size == 0)
            return nullptr;
        else
            return m_values + m_size;
    }

    template<typename T, uSize maxLength>
    T const* StaticVector<T, maxLength>::end() const
    {
        if (m_size == 0)
            return nullptr;
        else
            return m_values + m_size;
    }
}