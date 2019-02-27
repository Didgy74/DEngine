#pragma once

#include "MatrixBase.hpp"
#include "../Trait.hpp"

#include <initializer_list>
#include <optional>

namespace Math
{
	template<size_t width, size_t height, typename T>
	struct Matrix;

	namespace detail
	{
		template<size_t width, size_t height, typename T>
		struct MatrixBase;

		template<size_t width, typename T>
		struct MatrixBaseSquare : public MatrixBase<width, width, T>
		{
		public:
			using ParentType = MatrixBase<width, width, T>;

			constexpr void Transpose();

			[[nodiscard]] constexpr Math::Matrix<width, width, T> GetAdjugate() const;

			[[nodiscard]] constexpr T GetDeterminant() const;
			[[nodiscard]] constexpr bool IsSingular() const;

			[[nodiscard]] constexpr std::optional<Math::Matrix<width, width, T>> GetInverse() const;

			[[nodiscard]] static constexpr Math::Matrix<width, width, T> Identity();
		};

		template<size_t width, typename T>
		constexpr void MatrixBaseSquare<width, T>::Transpose()
		{
			for (size_t x = 1; x < width; x++)
			{
				for (size_t y = 0; y < x; y++)
					std::swap((*this)[x][y], (*this)[y][x]);
			}
		}

		template<size_t width, typename T>
		constexpr Math::Matrix<width, width, T> MatrixBaseSquare<width, T>::GetAdjugate() const
		{
			Math::Matrix<width, width, T> newMatrix;
			for (size_t x = 0; x < width; x++)
			{
				int8_t factor = (!(x % 2)) * 2 - 1;
				for (size_t y = 0; y < width; y++)
				{
					newMatrix[y][x] = factor * this->GetMinor(x, y).GetDeterminant();
					factor = -factor;
				}
			}
			return newMatrix;
		}

		template<size_t width, typename T>
		constexpr T MatrixBaseSquare<width, T>::GetDeterminant() const 
		{
			if constexpr (width >= 3)
			{
				T determinant = T(0);
				int8_t factor = -1;
				for (size_t x = 0; x < width; x++)
				{
					factor = -factor;
					if ((*this)[x][0] == T(0))
						continue;
					determinant += factor * (*this)[x][0] * this->GetMinor(x, 0).GetDeterminant();
				}
				return determinant;
			}
			else if constexpr (width == 2)
				return (*this)[0][0] * (*this)[1][1] - (*this)[1][0] * (*this)[0][1];
			else if constexpr (width == 1)
				return (*this)[0][0];
			else
				return 1;
		}

		template<size_t width, typename T>
		constexpr bool MatrixBaseSquare<width, T>::IsSingular() const { return GetDeterminant() == T(0); }

		template<size_t width, typename T>
		constexpr std::optional<Math::Matrix<width, width, T>> MatrixBaseSquare<width, T>::GetInverse() const
		{
			auto adjugate = this->GetAdjugate();
			T determinant = T();
			for (size_t x = 0; x < width; x++)
				determinant += (*this)[x][0] * adjugate[0][x];
			if (!(determinant == T()))
			{
				for (auto& item : adjugate)
					item /= determinant;
				return adjugate;
			}
			else
				return {};
		}

		template<size_t width, typename T>
		constexpr Math::Matrix<width, width, T> MatrixBaseSquare<width, T>::Identity()
		{
			Math::Matrix<width, width, T> temp{};
			for (size_t x = 0; x < width; x++)
				temp[x][x] = T(1);
			return temp;
		}
	}
}