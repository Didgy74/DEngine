#pragma once

#include "DEngine/Int.hpp"
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
            alignas(T) unsigned char m_data[sizeof(T) * maxLength] = {};
            // Mostly used to let debuggers see the contents
            T m_unused[maxLength];
        };
        
        uSize m_size = 0;

    public:
        FixedVector();
        FixedVector(uSize length);
        ~FixedVector();

        [[nodiscard]] constexpr Span<T> span() noexcept;
        [[nodiscard]] constexpr Span<T const> span() const noexcept;
        constexpr operator Span<T>() noexcept;
        constexpr operator Span<T const>() noexcept;

        [[nodiscard]] T* data();
        [[nodiscard]] T const* data() const;
        [[nodiscard]] constexpr uSize maxSize() const;
        [[nodiscard]] uSize size() const;

        void resize(uSize newSize);
        bool canPushBack() const noexcept;
        void pushBack(T const& in);
        void pushBack(T&& in);

        [[nodiscard]] T& at(uSize i);
        [[nodiscard]] T const& at(uSize i) const;

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
        m_data()
    {

    }

    template<typename T, uSize maxLength>
    FixedVector<T, maxLength>::FixedVector(uSize length) :
        m_size(length)
    {
        if (length > maxLength)
            throw std::out_of_range("Attempted to create a FixedVector with length higher than maxSize().");
        new(m_data) T[length];
    }

    template<typename T, uSize maxLength>
    FixedVector<T, maxLength>::~FixedVector()
    {
        for (uSize i = m_size; i > 0; i -= 1)
            m_unused[i - 1].~T();
    }

    template<typename T, uSize maxLength>
    constexpr Span<T> FixedVector<T, maxLength>::span() noexcept
    {
        return Span<T>(reinterpret_cast<T*>(m_data), m_size);
    }

    template<typename T, uSize maxLength>
    constexpr Span<T const> FixedVector<T, maxLength>::span() const noexcept
    {
        return Span<T const>(reinterpret_cast<T const*>(m_data), m_size);
    }

    template<typename T, uSize maxLength>
    constexpr FixedVector<T, maxLength>::operator Span<T>() noexcept
    {
        return span();
    }

    template<typename T, uSize maxLength>
    constexpr FixedVector<T, maxLength>::operator Span<T const>() noexcept
    {
        return span();
    }

    template<typename T, uSize maxLength>
    T* FixedVector<T, maxLength>::data()
    {
        return reinterpret_cast<T*>(m_data);
    }

    template<typename T, uSize maxLength>
    T const* FixedVector<T, maxLength>::data() const
    {
        return reinterpret_cast<T const*>(m_data);
    }

    template<typename T, uSize maxLength>
    constexpr uSize FixedVector<T, maxLength>::maxSize() const
    {
        return maxLength;
    }

    template<typename T, uSize maxLength>
    uSize FixedVector<T, maxLength>::size() const
    {
        return m_size;
    }

    template<typename T, uSize maxLength>
    void FixedVector<T, maxLength>::resize(uSize newSize)
    {
        if (m_size >= maxLength)
            throw std::out_of_range("Attempted to .resize() a FixedVector beyond what it can maximally hold.");
        if (newSize > m_size)
            new(m_data + sizeof(T) * m_size) T[newSize - m_size];
        else
        {
            for (uSize i = m_size - 1; i >= newSize; i -= 1)
                (reinterpret_cast<T*>(m_data) + i)->~T();
        }
        m_size = newSize;
    }

    template<typename T, uSize maxLength>
    bool FixedVector<T, maxLength>::canPushBack() const noexcept
    {
        return m_size < maxLength;
    }

    template<typename T, uSize maxLength>
    void FixedVector<T, maxLength>::pushBack(T const& in)
    {
        if (!canPushBack())
            throw std::out_of_range("Attempted to .pushBack() a FixedVector when it was already at max capacity.");
        new(m_data + sizeof(T) * m_size) T(in);
        m_size += 1;
    }

    template<typename T, uSize maxLength>
    void FixedVector<T, maxLength>::pushBack(T&& in)
    {
        if (!canPushBack())
            throw std::out_of_range("Attempted to .pushBack() a FixedVector when it was already at max capacity.");
        new(m_data + sizeof(T) * m_size) T(static_cast<T&&>(in));
        m_size += 1;
    }

    template<typename T, uSize maxLength>
    T& FixedVector<T, maxLength>::at(uSize i)
    {
        if (i >= m_size)
            throw std::out_of_range("Attempted to .at() a FixedVector with an index out of bounds.");
        return *(reinterpret_cast<T*>(m_data) + i);
    }

    template<typename T, uSize maxLength>
    T const& FixedVector<T, maxLength>::at(uSize i) const
    {
        if (i >= m_size)
            throw std::out_of_range("Attempted to .at() a FixedVector with an index out of bounds.");
        return *(reinterpret_cast<T*>(m_data) + i);
    }

    template<typename T, uSize maxLength>
    T& FixedVector<T, maxLength>::operator[](uSize i)
    {
        if (i >= m_size)
            throw std::out_of_range("Attempted to .at() a FixedVector with an index out of bounds.");
        return *(reinterpret_cast<T*>(m_data) + i);
    }

    template<typename T, uSize maxLength>
    T const& FixedVector<T, maxLength>::operator[](uSize i) const
    {
        if (i >= m_size)
            throw std::out_of_range("Attempted to .at() a FixedVector with an index out of bounds.");
        return *(reinterpret_cast<T const*>(m_data) + i);
    }

    template<typename T, uSize maxLength>
    T* FixedVector<T, maxLength>::begin()
    {
        if (m_size == 0)
            return nullptr;
        else
            return reinterpret_cast<T*>(m_data);
    }

    template<typename T, uSize maxLength>
    T const* FixedVector<T, maxLength>::begin() const
    {
        if (m_size == 0)
            return nullptr;
        else
            return reinterpret_cast<T const*>(m_data);
    }

    template<typename T, uSize maxLength>
    T* FixedVector<T, maxLength>::end()
    {
        if (m_size == 0)
            return nullptr;
        else
            return reinterpret_cast<T*>(m_data + sizeof(T) * m_size);
    }

    template<typename T, uSize maxLength>
    T const* FixedVector<T, maxLength>::end() const
    {
        if (m_size == 0)
            return nullptr;
        else
            return reinterpret_cast<T const*>(m_data + sizeof(T) * m_size);
    }
}