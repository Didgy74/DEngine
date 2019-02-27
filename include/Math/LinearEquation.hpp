#pragma once

#include "Vector/Vector.hpp"

#include <array>
#include <optional>
#include <iostream>
#include <type_traits>

namespace Math
{
	namespace detail
	{
		namespace LinearEquation
		{
			using LengthType = unsigned char;
			static_assert(std::is_arithmetic_v<LengthType>, "Error. Math::Core::LinearEquation::LengthType must be of integral type.");
		}
	}

	template<detail::LinearEquation::LengthType numUnknowns, typename T = float>
	struct LinearEquation
	{
		using LengthType = decltype(numUnknowns);
		using ValueType = T;

		std::array<std::array<ValueType, numUnknowns>, numUnknowns + 1> data;

		std::optional<Vector<numUnknowns, T>> Solve() const;

		
	};

	template<size_t numUnknowns, typename T>
	constexpr std::optional<Vector<numUnknowns, T>> SolveLinearEquation(const Matrix<numUnknowns + 1, numUnknowns, T>& input)
	{
		using LengthType = decltype(numUnknowns);
		constexpr LengthType width = numUnknowns + 1;
		constexpr LengthType height = numUnknowns;
		 
		Matrix<numUnknowns + 1, numUnknowns, T> copyMatrix = input;

		for (LengthType x = 0; x < height; x++)
		{
			if (copyMatrix[x][x] == 0)
			{
				// Pivot is zero. Swap with another row.
				LengthType validRow = x;

				// Searches for another row. Any row without leading 0 is fine.
				// NB! Can start at X-value because rows above have had their non-pivots calculated to 0.
				for (LengthType y = x; y < height; y++)
				{
					if (copyMatrix[x][y] != 0)
					{
						validRow = y;
						break;
					}
				}

				if (validRow == x)
					// Error. Found no row with non-zero pivot. Matrix cannot be solved.
					return std::optional<Vector<numUnknowns, T>>();

				// Swaps the two rows on both matrices.
				copyMatrix.SwapRows(x, validRow);
			}

			// Calculates non-pivots to 0 in current column.
			for (LengthType y = 0; y < height; y++)
			{
				// Traverses vertically, using current pivot to calculate every non-pivot in column to 0.
				// Using row subtraction. Does not do anything to current pivot's row.
				if (y != x)
				{
					// Scales the row to get this row's current column to zero.
					float rowScale = copyMatrix[x][y] / copyMatrix[x][x];

					// Performs row subtraction
					for (LengthType i = 0; i < width; i++)
						copyMatrix[i][y] -= rowScale * copyMatrix[i][x];

					// Sets element to 0, in case of precision error.
					copyMatrix[x][y] = 0;
				}
			}
		}

		// Normalizes every row. Normally the rest of the matrix should also be normalized, but this is the last operation we perform.
		// So we only do it to the fifth column to get better performance.
		// Traverses vertically

		Vector<numUnknowns, T> returnVector;

		for (LengthType i = 0; i < height; i++)
			returnVector[i] = copyMatrix[width - 1][i] / copyMatrix[i][i];

		return std::optional<Vector<numUnknowns, T>>(returnVector);
	}
}