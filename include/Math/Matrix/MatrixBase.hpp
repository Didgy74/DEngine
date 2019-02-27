#pragma once

#include "../Trait.hpp"

#include <string>
#include <type_traits>
#include <initializer_list>
#include <iterator>
#include <array>
#include <cassert>

namespace Math
{
	template<size_t width, size_t height, typename T>
	struct Matrix;

	namespace detail
	{
		template<size_t width, size_t height, typename T>
		struct MatrixBase
		{
		public:
			static constexpr bool IsColumnMajor();

			std::array<T, width * height> data = {};

			[[nodiscard]] constexpr T& At(size_t x, size_t y);
			[[nodiscard]] constexpr const T& At(size_t x, size_t y) const;

			[[nodiscard]] constexpr Math::Matrix<width - 1, height - 1, T> GetMinor(size_t columnIndexToSlice, size_t rowIndexToSlice) const;

			[[nodiscard]] constexpr T* operator[](size_t index);
			[[nodiscard]] constexpr const T* operator[](size_t index) const;
			[[nodiscard]] constexpr T& operator()(size_t x, size_t y);
			[[nodiscard]] constexpr const T& operator()(size_t x, size_t y) const;

			[[nodiscard]] constexpr auto begin() noexcept;
			[[nodiscard]] constexpr auto begin() const noexcept;
			[[nodiscard]] constexpr auto end() noexcept;
			[[nodiscard]] constexpr auto end() const noexcept;
		};

		template<size_t width, size_t height, typename T>
		constexpr bool MatrixBase<width, height, T>::IsColumnMajor() { return true; }

		template<size_t width, size_t height, typename T>
		constexpr T& MatrixBase<width, height, T>::At(size_t x, size_t y) { return (*this)(x, y); }

		template<size_t width, size_t height, typename T>
		constexpr const T& MatrixBase<width, height, T>::At(size_t x, size_t y) const { return (*this)(x, y); }

		template<size_t width, size_t height, typename T>
		constexpr Math::Matrix<width - 1, height - 1, T> MatrixBase<width, height, T>::GetMinor(size_t columnIndexToSlice, size_t rowIndexToSlice) const
		{
			assert(columnIndexToSlice < width && rowIndexToSlice < height);

			Math::Matrix<width - 1, height - 1, T> newMatrix;
			for (size_t x = 0; x < width; x++)
			{
				if (x == columnIndexToSlice)
					continue;

				for (size_t y = 0; y < height; y++)
				{
					if (y == rowIndexToSlice)
						continue;

					newMatrix(x < columnIndexToSlice ? x : x - 1, y < rowIndexToSlice ? y : y - 1) = (*this)(x, y);
				}
			}
			return newMatrix;
		}

		template<size_t width, size_t height, typename T>
		constexpr T* MatrixBase<width, height, T>::operator[](size_t index) { return data.data() + index * height; }

		template<size_t width, size_t height, typename T>
		constexpr const T* MatrixBase<width, height, T>::operator[](size_t index) const { return data.data() + index * height; }

		template<size_t width, size_t height, typename T>
		constexpr T& MatrixBase<width, height, T>::operator()(size_t x, size_t y) { return const_cast<T&>(std::as_const(*this)(x, y)); }

		template<size_t width, size_t height, typename T>
		constexpr const T& MatrixBase<width, height, T>::operator()(size_t x, size_t y) const 
		{
			if constexpr (IsColumnMajor())
				return data[x * height + y];
			else
				return data[y * width + x];
		}

		template<size_t width, size_t height, typename T>
		constexpr auto MatrixBase<width, height, T>::begin() noexcept { return data.begin(); }

		template<size_t width, size_t height, typename T>
		constexpr auto MatrixBase<width, height, T>::begin() const noexcept { return data.begin(); }

		template<size_t width, size_t height, typename T>
		constexpr auto MatrixBase<width, height, T>::end() noexcept { return data.end(); }

		template<size_t width, size_t height, typename T>
		constexpr auto MatrixBase<width, height, T>::end() const noexcept { return data.end(); }
	}
}