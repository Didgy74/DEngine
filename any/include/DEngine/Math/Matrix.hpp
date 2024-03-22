#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Trait.hpp>

#include <DEngine/Math/impl/Assert.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;
	template<uSize width, uSize height, class T>
	struct Matrix;

	using Mat2 = Matrix<2, 2, f32>;
	using Mat2Int = Matrix<2, 2, i32>;
	using Mat3 = Matrix<3, 3, f32>;
	using Mat3Int = Matrix<3, 3, i32>;
	using Mat4 = Matrix<4, 4, f32>;
	using Mat4Int = Matrix<4, 4, i32>;

	template<uSize width, uSize height, class T = f32>
	struct Matrix
	{
	public:
		T m_data[width * height];

		[[nodiscard]] constexpr T& At(uSize x, uSize y) noexcept;
		[[nodiscard]] constexpr T const& At(uSize x, uSize y) const noexcept;

		[[nodiscard]] constexpr T* Data();
		[[nodiscard]] constexpr T const* Data() const;

		[[nodiscard]] constexpr Matrix<height, width, T> Transposed() const;
		constexpr void Transpose() noexcept requires (width == height)
		{
			for (uSize x = 1; x < width; x += 1)
			{
				for (uSize y = 0; y < x; y += 1)
				{
					auto& a = At(x, y);
					auto& b = At(y, x);
					auto temp = a;
					a = b;
					b = temp;
				}
			}
		}

		[[nodiscard]] constexpr Matrix<width-1, height-1, T> GetMinor(
			uSize columnToRemove,
			uSize rowToRemove) const noexcept
		{
			Matrix<width-1, height-1, T> newMatrix{};
			for (uSize x = 0; x < width; x += 1)
			{
				if (x == columnToRemove)
					continue;
				for (uSize y = 0; y < height; y += 1)
				{
					if (y == rowToRemove)
						continue;
					auto dstX = x < columnToRemove ? x : x - 1;
					auto dstY = y < rowToRemove ? y : y - 1;
					newMatrix.At(dstX, dstY) = At(x, y);
				}
			}
			return newMatrix;
		}
		[[nodiscard]] constexpr T GetDeterminant() const
			requires (width == height)
		{
			if constexpr (width >= 3)
			{
				T determinant = T();
				i8 factor = -1;
				for (uSize x = 0; x < width; x += 1)
				{
					factor = -factor;
					if (m_data[x * width] == T(0))
						continue;
					determinant += factor * m_data[x * width] * GetMinor(x, 0).GetDeterminant();
				}
				return determinant;
			}
			else if constexpr (width == 2)
				return m_data[0] * m_data[width + 1] - m_data[width] * m_data[1];
			else if constexpr (width == 1)
				return m_data[0];
			else
				return 1;
		}
		[[nodiscard]] constexpr Matrix GetAdjugate() const
			requires (width == height)
		{
			Matrix<width, width, T> newMatrix{};
			for (uSize x = 0; x < width; x += 1)
			{
				i8 factor = (!(x % 2)) * 2 - 1;
				for (uSize y = 0; y < width; y += 1)
				{
					newMatrix.At(y, x) = factor * GetMinor(x, y).GetDeterminant();
					factor = -factor;
				}
			}
			return newMatrix;
		}
		[[nodiscard]] constexpr Std::Opt<Matrix> GetInverse() const
			requires (width == height)
		{
			auto adjugate = GetAdjugate();
			T determinant = T();
			for (uSize x = 0; x < width; x += 1)
				determinant += m_data[x * width] * adjugate.At(0, x);
			if (determinant != T())
			{
				for (uSize i = 0; i < width * width; i += 1)
					adjugate.m_data[i] /= determinant;
				return Std::Opt{ adjugate };
			}
			else
				return Std::nullOpt;
		}

		[[nodiscard]] constexpr bool IsSingular() const
		{
			return GetDeterminant() == T(0);
		}

		[[nodiscard]] static constexpr Matrix SingleValue(T input);
		[[nodiscard]] static constexpr Matrix Zero();
		[[nodiscard]] static constexpr Matrix One();
		[[nodiscard]] static constexpr Matrix Identity() noexcept
			requires (width == height)
		{
			Matrix<width, width, T> temp = {};
			for (uSize x = 0; x < width; x += 1)
				temp.At(x, x) = T(1);
			return temp;
		}

		[[nodiscard]] constexpr Matrix operator+(Matrix const& rhs) const;
		constexpr Matrix& operator+=(Matrix const& rhs);
		[[nodiscard]] constexpr Matrix operator-(Matrix const& rhs) const;
		constexpr Matrix& operator-=(Matrix const& rhs);
		[[nodiscard]] constexpr Matrix operator-() const;
		template<uSize widthB>
		[[nodiscard]] constexpr Matrix<widthB, height, T> operator*(Matrix<widthB, width, T> const& right) const;
		[[nodiscard]] Vector<height, T> operator*(Vector<width, T> const& right) const;
		constexpr Matrix& operator*=(T const& right);
		[[nodiscard]] constexpr bool operator==(Matrix const& right) const;
		[[nodiscard]] constexpr bool operator!=(Matrix const& right) const;
	};

	template<uSize width, uSize height, typename T>
	constexpr T& Matrix<width, height, T>::At(uSize x, uSize y) noexcept
	{
		DENGINE_IMPL_MATH_ASSERT(x < width);
		DENGINE_IMPL_MATH_ASSERT(y < height);
		return m_data[x * height + y];
	}

	template<uSize width, uSize height, typename T>
	constexpr T const& Matrix<width, height, T>::At(uSize x, uSize y) const noexcept
	{
		DENGINE_IMPL_MATH_ASSERT(x < width);
		DENGINE_IMPL_MATH_ASSERT(y < height);
		return m_data[x * height + y];
	}

	template<uSize width, uSize height, typename T>
	constexpr T* Matrix<width, height, T>::Data() { return m_data; }

	template<uSize width, uSize height, typename T>
	constexpr T const* Matrix<width, height, T>::Data() const { return m_data; }

	template<uSize width, uSize height, typename T>
	constexpr Matrix<height, width, T> Matrix<width, height, T>::Transposed() const
	{
		Matrix<height, width, T> temp{};
		for (uSize x = 0; x < width; x += 1)
		{
			for (uSize y = 0; y < height; y += 1)
				temp.At(x, y) = At(y, x);
		}
		return temp;
	}

	template<uSize width, uSize height, typename T>
	constexpr auto Matrix<width, height, T>::SingleValue(T input) -> Matrix
	{
		Matrix<width, height, T> returnMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			returnMatrix.data[i] = input;
		return returnMatrix;
	}

	template<uSize width, uSize height, typename T>
	constexpr auto Matrix<width, height, T>::Zero() -> Matrix { return Matrix<width, height, T>{}; }

	template<uSize width, uSize height, typename T>
	constexpr auto Matrix<width, height, T>::One() -> Matrix
	{
		Matrix<width, height, T> returnMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			returnMatrix.m_data[i] = T(1);
		return returnMatrix;
	}

	template<uSize width, uSize height, typename T>
	constexpr auto Matrix<width, height, T>::operator+(Matrix const& rhs) const -> Matrix
	{
		Matrix<width, height, T> newMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			newMatrix.m_data[i] = m_data[i] + rhs.m_data[i];
		return newMatrix;
	}

	template<uSize width, uSize height, typename T>
	constexpr auto Matrix<width, height, T>::operator+=(Matrix const& rhs) -> Matrix&
	{
		for (uSize i = 0; i < width * height; i += 1)
			m_data[i] += rhs.m_data[i];
		return *this;
	}

	template<uSize width, uSize height, typename T>
	constexpr auto Matrix<width, height, T>::operator-(Matrix const& rhs) const -> Matrix
	{
		Matrix<width, height, T> newMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			newMatrix.m_data[i] = m_data[i] - rhs.m_data[i];
		return newMatrix;
	}

	template<uSize width, uSize height, typename T>
	constexpr auto Matrix<width, height, T>::operator-=(Matrix<width, height, T> const& rhs) -> Matrix&
	{
		for (uSize i = 0; i < width * height; i += 1)
			m_data[i] -= rhs.m_data[i];
		return *this;
	}

	template<uSize width, uSize height, typename T>
	constexpr auto Matrix<width, height, T>::operator-() const -> Matrix
	{
		Matrix<width, height, T> newMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			newMatrix.m_data[i] = -m_data[i];
		return newMatrix;
	}

	template<uSize width, uSize height, typename T>
	template<uSize widthB>
	constexpr Matrix<widthB, height, T> Matrix<width, height, T>::operator*(Matrix<widthB, width, T> const& right) const
	{
		Matrix<widthB, height, T> newMatrix{};
		for (uSize x = 0; x < widthB; x += 1)
		{
			for (uSize y = 0; y < height; y += 1)
			{
				T dot = T(0);
				for (uSize i = 0; i < width; i++)
					dot += At(i, y) * right.At(x, i);
				newMatrix.At(x, y) = dot;
			}
		}
		return newMatrix;
	}

	template<uSize width, uSize height, typename T>
	Vector<height, T> Matrix<width, height, T>::operator*(Vector<width, T> const& right) const
	{
		Vector<height, T> newVector;
		for (uSize y = 0; y < height; y += 1)
		{
			T dot{};
			for (uSize i = 0; i < width; i += 1)
				dot += At(i, y) * right[i];
			newVector[y] = dot;
		}
		return newVector;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T>& Matrix<width, height, T>::operator*=(T const& right)
	{
		for (uSize i = 0; i < width * height; i += 1)
			this->data[i] *= right;
		return *this;
	}

	template<uSize width, uSize height, typename T>
	constexpr bool Matrix<width, height, T>::operator==(Matrix<width, height, T> const& right) const
	{
		for (uSize i = 0; i < width * height; i += 1)
		{
			if (this->data[i] != right.data[i])
				return false;
		}
		return true;
	}

	template<uSize width, uSize height, typename T>
	constexpr bool Matrix<width, height, T>::operator!=(Matrix<width, height, T> const& right) const
	{
		for (uSize i = 0; i < width * height; i += 1)
		{
			if (this->data[i] != right.data[i])
				return true;
		}
		return false;
	}

	template<uSize width, uSize height, typename T>
	[[nodiscard]] constexpr Matrix<width, height, T> operator*(Matrix<width, height, T> const& lhs, T rhs)
	{
		Matrix<width, height, T> newMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			newMatrix.data[i] = lhs.data[i] * rhs;
		return newMatrix;
	}

	template<uSize width, uSize height, typename T>
	[[nodiscard]] constexpr Matrix<width, height, T> operator*(T left, Matrix<width, height, T> const& right)
	{
		Matrix<width, height, T> newMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			newMatrix.data[i] = left * right.data[i];
		return newMatrix;
	}
}