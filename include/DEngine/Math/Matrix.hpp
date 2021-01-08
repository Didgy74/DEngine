#pragma once

#include <DEngine/detail/Assert.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Trait.hpp>

#include <DEngine/Math/detail/MatrixBase.hpp>
#include <DEngine/Math/detail/MatrixBaseSquare.hpp>

namespace DEngine::Math
{
	template<uSize length, typename T>
	struct Vector;

	using Mat2 = Matrix<2, 2, f32>;
	using Mat2Int = Matrix<2, 2, i32>;
	using Mat3 = Matrix<3, 3, f32>;
	using Mat3Int = Matrix<3, 3, i32>;
	using Mat4 = Matrix<4, 4, f32>;
	using Mat4Int = Matrix<4, 4, i32>;

	template<uSize width, uSize height, typename T = f32>
	struct Matrix : public Std::Trait::Cond<width == height, detail::MatrixBaseSquare<width, T>, detail::MatrixBase<width, height, T>>
	{
	public:
		[[nodiscard]] constexpr T& At(uSize x, uSize y) noexcept;
		[[nodiscard]] constexpr T At(uSize x, uSize y) const noexcept;

		[[nodiscard]] constexpr T* Data();
		[[nodiscard]] constexpr T const* Data() const;

		[[nodiscard]] constexpr Matrix<height, width, T> Transposed() const;

		[[nodiscard]] static constexpr Matrix<width, height, T> SingleValue(T input);
		[[nodiscard]] static constexpr Matrix<width, height, T> Zero();
		[[nodiscard]] static constexpr Matrix<width, height, T> One();

		[[nodiscard]] constexpr Matrix<width, height, T> operator+(Matrix<width, height, T> const& rhs) const;
		constexpr Matrix<width, height, T>& operator+=(Matrix<width, height, T> const& rhs);
		[[nodiscard]] constexpr Matrix<width, height, T> operator-(Matrix<width, height, T> const& rhs) const;
		constexpr Matrix<width, height, T>& operator-=(Matrix<width, height, T> const& rhs);
		[[nodiscard]] constexpr Matrix<width, height, T> operator-() const;
		template<uSize widthB>
		[[nodiscard]] constexpr Matrix<widthB, height, T> operator*(Matrix<widthB, width, T> const& right) const;
		[[nodiscard]] Vector<height, T> operator*(Vector<width, T> const& right) const;
		constexpr Matrix<width, height, T>& operator*=(T const& right);
		[[nodiscard]] constexpr bool operator==(Matrix<width, height, T> const& right) const;
		[[nodiscard]] constexpr bool operator!=(Matrix<width, height, T> const& right) const;
	};

	template<uSize width, uSize height, typename T>
	constexpr T& Matrix<width, height, T>::At(uSize x, uSize y) noexcept
	{
		DENGINE_DETAIL_ASSERT(x < width);
		DENGINE_DETAIL_ASSERT(y < height);
		return this->data[x * height + y];
	}

	template<uSize width, uSize height, typename T>
	constexpr T Matrix<width, height, T>::At(uSize x, uSize y) const noexcept
	{
		DENGINE_DETAIL_ASSERT(x < width);
		DENGINE_DETAIL_ASSERT(y < height);
		return this->data[x * height + y];
	}

	template<uSize width, uSize height, typename T>
	constexpr T* Matrix<width, height, T>::Data()
	{
		return this->data;
	}

	template<uSize width, uSize height, typename T>
	constexpr T const* Matrix<width, height, T>::Data() const
	{
		return this->data;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<height, width, T> Matrix<width, height, T>::Transposed() const
	{
		Matrix<height, width, T> temp{};
		for (size_t x = 0; x < width; x += 1)
		{
			for (size_t y = 0; y < height; y += 1)
				temp.At(x, y) = At(y, x);
		}
		return temp;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T> Matrix<width, height, T>::SingleValue(T input)
	{
		Matrix<width, height, T> returnMatrix{};
		for (size_t i = 0; i < width * height; i += 1)
			returnMatrix.data[i] = input;
		return returnMatrix;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T> Matrix<width, height, T>::Zero()
	{
		return Matrix<width, height, T>{};
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T> Matrix<width, height, T>::One()
	{
		Matrix<width, height, T> returnMatrix{};
		for (size_t i = 0; i < width * height; i += 1)
			returnMatrix.data[i] = T(1);
		return returnMatrix;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T> Matrix<width, height, T>::operator+(Matrix<width, height, T> const& rhs) const
	{
		Matrix<width, height, T> newMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			newMatrix.data[i] = this->data[i] + rhs.data[i];
		return newMatrix;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T>& Matrix<width, height, T>::operator+=(Matrix<width, height, T> const& rhs)
	{
		for (uSize i = 0; i < width * height; i += 1)
			this->data[i] += rhs.data[i];
		return *this;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T> Matrix<width, height, T>::operator-(Matrix<width, height, T> const& rhs) const
	{
		Matrix<width, height, T> newMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			newMatrix.data[i] = this->data[i] - rhs.data[i];
		return newMatrix;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T>& Matrix<width, height, T>::operator-=(Matrix<width, height, T> const& rhs)
	{
		for (uSize i = 0; i < width * height; i += 1)
			this->data[i] -= rhs.data[i];
		return *this;
	}

	template<uSize width, uSize height, typename T>
	constexpr Matrix<width, height, T> Matrix<width, height, T>::operator-() const
	{
		Matrix<width, height, T> newMatrix{};
		for (uSize i = 0; i < width * height; i += 1)
			newMatrix.data[i] = -this->data[i];
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
		for (size_t i = 0; i < width * height; i += 1)
			newMatrix.data[i] = left * right.data[i];
		return newMatrix;
	}
}