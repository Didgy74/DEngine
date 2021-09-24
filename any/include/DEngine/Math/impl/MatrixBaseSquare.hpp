#pragma once

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/detail/MatrixBase.hpp>
#include <DEngine/Math/Trait.hpp>

namespace DEngine::Math
{
	template<uSize width, uSize height, typename T>
	struct Matrix;

	namespace detail
	{
		template<uSize width, uSize height, typename T>
		struct MatrixBase;

		template<uSize width, typename T>
		struct MatrixBaseSquare : public MatrixBase<width, width, T>
		{
		public:
			[[nodiscard]] constexpr Math::Matrix<width, width, T> GetAdjugate() const
			{
				Math::Matrix<width, width, T> newMatrix{};
				for (uSize x = 0; x < width; x += 1)
				{
					i8 factor = (!(x % 2)) * 2 - 1;
					for (uSize y = 0; y < width; y += 1)
					{
						newMatrix.At(y, x) = factor * this->GetMinor(x, y).GetDeterminant();
						factor = -factor;
					}
				}
				return newMatrix;
			}

			[[nodiscard]] constexpr T GetDeterminant() const
			{
				if constexpr (width >= 3)
				{
					T determinant = T();
					i8 factor = -1;
					for (uSize x = 0; x < width; x += 1)
					{
						factor = -factor;
						if (this->data[x * width] == T(0))
							continue;
						determinant += factor * this->data[x * width] * this->GetMinor(x, 0).GetDeterminant();
					}
					return determinant;
				}
				else if constexpr (width == 2)
					return this->data[0] * this->data[width + 1] - this->data[width] * this->data[1];
				else if constexpr (width == 1)
					return this->data[0];
				else
					return 1;
			}

			[[nodiscard]] constexpr Std::Opt<Math::Matrix<width, width, T>> GetInverse() const
			{
				Math::Matrix<width, width, T> adjugate = GetAdjugate();
				T determinant = T();
				for (uSize x = 0; x < width; x += 1)
					determinant += this->data[x * width] * adjugate.At(0, x);
				if (!(determinant == T()))
				{
					for (uSize i = 0; i < width * width; i += 1)
						adjugate.data[i] /= determinant;
					return Std::Opt<Math::Matrix<width, width, T>>{ adjugate };
				}
				else
					return {};
			}

			constexpr void Transpose()
			{
				for (uSize x = 1; x < width; x += 1)
				{
					for (uSize y = 0; y < x; y += 1)
					{
						auto temp = this->data[x * width + y];
						this->data[x * width + y] = this->data[y * width + x];
						this->data[y * width + x] = temp;
					}

				}
			}

			[[nodiscard]] static constexpr Math::Matrix<width, width, T> Identity() noexcept
			{
				Math::Matrix<width, width, T> temp = {};
				for (uSize x = 0; x < width; x += 1)
					temp.data[x * width + x] = T(1);
				return temp;
			}

			[[nodiscard]] constexpr bool IsSingular() const
			{
				return GetDeterminant() == T(0);
			}
		};
	}
}