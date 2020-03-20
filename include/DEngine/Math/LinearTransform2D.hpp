#pragma once

#include "Setup.hpp"
#include "Matrix/Matrix.hpp"
#include "Vector/Vector.hpp"
#include "UnitQuaternion.hpp"
#include "Trigonometric.hpp"
#include "Common.hpp"

namespace Math
{
	namespace LinearTransform2D
	{
		[[nodiscard]] constexpr Matrix<3, 2> Multiply_Reduced(const Matrix<3, 2>& left, const Matrix<3, 2>& right);
		template<typename T = float>
		[[nodiscard]] constexpr Matrix<4, 4, T> AsMat4(const Matrix<3, 2, T>& input);

		[[nodiscard]] constexpr Matrix3x3 Translate(float x, float y);
		[[nodiscard]] constexpr Matrix3x3 Translate(const Vector2D& input);
		[[nodiscard]] constexpr Matrix<3, 2> Translate_Reduced(float x, float y);
		[[nodiscard]] constexpr Matrix<3, 2> Translate_Reduced(const Vector2D& input);

		constexpr void AddTranslation(Matrix3x3& matrix, float x, float y);
		constexpr void AddTranslation(Matrix3x3& matrix, const Vector2D& input);
		constexpr void AddTranslation(Matrix<3, 2>& matrix, float x, float y);
		constexpr void AddTranslation(Matrix<3, 2>& matrix, const Vector2D& input);

		[[nodiscard]] constexpr Vector2D GetTranslation(const Matrix3x3& matrix);
		[[nodiscard]] constexpr Vector2D GetTranslation(const Matrix<3, 2>& matrix);

		template<AngleUnit unit = Setup::defaultAngleUnit>
		[[nodiscard]] constexpr Matrix2x2 Rotate(float angle);
		template<AngleUnit unit = Setup::defaultAngleUnit>
		[[nodiscard]] constexpr Matrix3x3 Rotate_Homo(float angle);
		template<AngleUnit unit = Setup::defaultAngleUnit>
		[[nodiscard]] constexpr Matrix<3, 2> Rotate_Reduced(float angle);
		[[nodiscard]] constexpr Matrix2x2 Rotate(const UnitQuaternion<float>& input);
		[[nodiscard]] constexpr Matrix3x3 Rotate_Homo(const UnitQuaternion<float>& input);
		[[nodiscard]] constexpr Matrix<3, 2> Rotate_Reduced(const UnitQuaternion<float>& input);

		[[nodiscard]] constexpr Matrix2x2 Scale(float x, float y);
		[[nodiscard]] constexpr Matrix2x2 Scale(const Vector2D& input);
		[[nodiscard]] constexpr Matrix3x3 Scale_Homo(float x, float y);
		[[nodiscard]] constexpr Matrix3x3 Scale_Homo(const Vector2D& input);
		[[nodiscard]] constexpr Matrix<3, 2> Scale_Reduced(float x, float y);
		[[nodiscard]] constexpr Matrix<3, 2> Scale_Reduced(const Vector2D& input);
	}

	namespace LinTran2D = LinearTransform2D;
}

constexpr Math::Matrix<3, 2> Math::LinearTransform2D::Multiply_Reduced(const Matrix<3, 2>& left, const Matrix<3, 2>& right)
{
	Matrix<3, 2> newMatrix;
	for (size_t x = 0; x < 2; x++)
	{
		for (size_t y = 0; y < 2; y++)
		{
			Matrix<3, 2>::ValueType dot{};
			for (size_t i = 0; i < 2; i++)
				dot += left[i][y] * right[x][i];
			newMatrix[x][y] = dot;
		}
	}

	for (size_t y = 0; y < 2; y++)
	{
		Matrix<3, 2>::ValueType dot{};
		for (size_t i = 0; i < 2; i++)
			dot += left[i][y] * right[2][i];
		dot += left[2][y];
		newMatrix[2][y] = dot;
	}

	return newMatrix;
}

template<typename T>
constexpr Math::Matrix<4, 4, T> Math::LinearTransform2D::AsMat4(const Matrix<3, 2, T>& input)
{
	Matrix<4, 4, T> newMatrix;
	newMatrix[2][2] = 1;
	newMatrix[3][3] = 1;

	for (size_t x = 0; x < 3; x++)
	{
		for (size_t y = 0; y < 2; y++)
			newMatrix[x][y] = input[x][y];
	}

	return newMatrix;
}

constexpr Math::Matrix3x3 Math::LinearTransform2D::Translate(float x, float y)
{
	return Matrix3x3
	{
		1, 0, 0,
		0, 1, 0,
		x, y, 1
	};
}

constexpr Math::Matrix3x3 Math::LinearTransform2D::Translate(const Vector2D& input) { return Translate(input.x, input.y); }

constexpr Math::Matrix<3, 2> Math::LinearTransform2D::Translate_Reduced(float x, float y)
{
	return Matrix<3, 2>
	{
		1, 0,
		0, 1,
		x, y,
	};
}

constexpr Math::Matrix<3, 2> Math::LinearTransform2D::Translate_Reduced(const Vector2D& input) { return Translate_Reduced(input.x, input.y); }

constexpr void Math::LinearTransform2D::AddTranslation(Matrix3x3& matrix, float x, float y)
{
	matrix[2][0] += x;
	matrix[2][1] += y;
}

constexpr void Math::LinearTransform2D::AddTranslation(Matrix3x3& matrix, const Vector2D& input) { AddTranslation(matrix, input.x, input.y); }

constexpr void Math::LinearTransform2D::AddTranslation(Matrix<3, 2>& matrix, float x, float y)
{
	matrix[2][0] += x;
	matrix[2][1] += y;
}

constexpr void Math::LinearTransform2D::AddTranslation(Matrix<3, 2>& matrix, const Vector2D& input) 
{ 
	return AddTranslation(matrix, input.x, input.y); 
}

constexpr Math::Vector2D Math::LinearTransform2D::GetTranslation(const Matrix3x3& matrix) { return Math::Vector2D{matrix[2][0], matrix[2][1] }; }

constexpr Math::Vector2D Math::LinearTransform2D::GetTranslation(const Matrix<3, 2>& matrix) 
{ 
	return Math::Vector2D{ matrix[2][0], matrix[2][1] }; 
}

template<Math::AngleUnit unit>
constexpr Math::Matrix2x2 Math::LinearTransform2D::Rotate(float angle)
{
	const float cos = Cos<unit>(angle);
	const float sin = Sin<unit>(angle);
	return Matrix2x2
	({
		cos, sin,
		-sin, cos
	});
}

template<Math::AngleUnit unit>
constexpr Math::Matrix3x3 Math::LinearTransform2D::Rotate_Homo(float angle)
{
	const float cos = Cos<unit>(angle);
	const float sin = Sin<unit>(angle);
	return Matrix3x3
	({
		cos, sin, 0,
		-sin, cos, 0,
		0, 0, 1
	});
}

template<Math::AngleUnit unit>
constexpr Math::Matrix<3, 2> Math::LinearTransform2D::Rotate_Reduced(float angle)
{
	const float cos = Cos<unit>(angle);
	const float sin = Sin<unit>(angle);
	return Matrix<3, 2>
	({
		cos, sin,
		-sin, cos,
		0, 0,
	});
}

constexpr Math::Matrix2x2 Math::LinearTransform2D::Rotate(const UnitQuaternion<float>& input)
{
	return Matrix2x2
	{
		1 - 2*Sqrd(input.GetY()) - 2*Sqrd(input.GetZ()), 2*input.GetX()*input.GetY() + 2*input.GetZ()*input.GetS(),
		2*input.GetX()*input.GetY() - 2*input.GetZ()*input.GetS(), 1 - 2*Sqrd(input.GetX()) - 2*Sqrd(input.GetZ())
	};
}

constexpr Math::Matrix3x3 Math::LinearTransform2D::Rotate_Homo(const UnitQuaternion<float>& input)
{
	return Matrix3x3
	{
		1 - 2*Sqrd(input.GetY()) - 2*Sqrd(input.GetZ()), 2*input.GetX()*input.GetY() + 2*input.GetZ()*input.GetS(), 0,
		2*input.GetX()*input.GetY() - 2*input.GetZ()*input.GetS(), 1 - 2*Sqrd(input.GetX()) - 2*Sqrd(input.GetZ()), 0,
		0, 0, 1
	};
}

constexpr Math::Matrix<3, 2> Math::LinearTransform2D::Rotate_Reduced(const UnitQuaternion<float>& input)
{
	return Matrix<3, 2>
	{
		1 - 2 * Sqrd(input.GetY()) - 2 * Sqrd(input.GetZ()), 2 * input.GetX()*input.GetY() + 2 * input.GetZ()*input.GetS(),
		2 * input.GetX()*input.GetY() - 2 * input.GetZ()*input.GetS(), 1 - 2 * Sqrd(input.GetX()) - 2 * Sqrd(input.GetZ()),
		0, 0,
	};
}

constexpr Math::Matrix2x2 Math::LinearTransform2D::Scale(float x, float y)
{
	return Matrix2x2
	{
		x, 0,
		0, y
	};
}

constexpr Math::Matrix2x2 Math::LinearTransform2D::Scale(const Vector2D& input) { return Scale(input.x, input.y); }

constexpr Math::Matrix3x3 Math::LinearTransform2D::Scale_Homo(float x, float y)
{
	return Matrix3x3
	{
		x, 0, 0,
		0, y, 0,
		0, 0, 1
	};
}

constexpr Math::Matrix3x3 Math::LinearTransform2D::Scale_Homo(const Vector2D& input) { return Scale_Homo(input.x, input.y); }

constexpr Math::Matrix<3, 2> Math::LinearTransform2D::Scale_Reduced(float x, float y)
{
	return Matrix<3, 2>
	{
		x, 0,
		0, y,
		0, 0,
	};
}

constexpr Math::Matrix<3, 2> Math::LinearTransform2D::Scale_Reduced(const Vector2D& input) { return Scale_Reduced(input.x, input.y); }
