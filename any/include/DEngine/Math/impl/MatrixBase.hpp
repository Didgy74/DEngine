#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{
	template<uSize width, uSize height, typename T>
	struct Matrix;

	namespace detail
	{
		template<uSize width, uSize height, typename T>
		struct MatrixBase
		{
		public:
			T data[width * height];

			[[nodiscard]] constexpr Math::Matrix<width-1, height-1, T> GetMinor(
				uSize columnToRemove, 
				uSize rowToRemove) const;
		};

		template<uSize width, uSize height, typename T>
		constexpr Math::Matrix<width-1, height-1, T> MatrixBase<width, height, T>::GetMinor(
			uSize columnToRemove, 
			uSize rowToRemove) const
		{
			Math::Matrix<width-1, height-1, T> newMatrix{};
			for (uSize x = 0; x < width; x += 1)
			{
				if (x == columnToRemove)
					continue;

				for (uSize y = 0; y < height; y += 1)
				{
					if (y == rowToRemove)
						continue;

					newMatrix.At(x < columnToRemove ? x : x - 1, y < rowToRemove ? y : y - 1) = data[x * height + y];
				}
			}
			return newMatrix;
		}
	}
}