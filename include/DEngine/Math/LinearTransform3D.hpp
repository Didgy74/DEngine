#pragma once

#include <DEngine/Math/Enums.hpp>
#include <DEngine/Math/Matrix.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Math/Trigonometric.hpp>
#include <DEngine/Math/Vector.hpp>


namespace DEngine::Math::LinearTransform3D
{
	[[nodiscard]] constexpr Matrix<4, 3, f32> Multiply_Reduced(
		Matrix<4, 3, f32> const& left, 
		Matrix<4, 3, f32> const& right);

	[[nodiscard]] constexpr Vector<3, f32> Multiply_Reduced(
		Matrix<4, 3, f32> const& left,
		Vector<3, f32> const& right);

	template<typename T>
	[[nodiscard]] constexpr Matrix<4, 4, T> AsMat4(Matrix<4, 3, T> const& input);

	[[nodiscard]] constexpr Matrix<4, 4, f32> Translate(Vector<3, f32> const& input);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Translate_Reduced(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Translate_Reduced(Vector<3, f32> const& input);

	constexpr void SetTranslation(Matrix<4, 4, f32>& matrix, Vector<3, f32> const& input);
	constexpr void SetTranslation(Matrix<4, 3, f32>& matrix, Vector<3, f32> const& input);
	constexpr Vector<3, f32> GetTranslation(Matrix<4, 4, f32> const& input);
	constexpr Vector<3, f32> GetTranslation(Matrix<4, 3, f32> const& input);

	[[nodiscard]] Matrix<3, 3, f32> Rotate(ElementaryAxis axis, f32 amountRadians);
	[[nodiscard]] Matrix<4, 4, f32> Rotate_Homo(ElementaryAxis axis, f32 amountRadians);
	[[nodiscard]] Matrix<3, 3, f32> Rotate(f32 xRadians, f32 yRadians, f32 zRadians);
	[[nodiscard]] Matrix<3, 3, f32> Rotate(Vector<3, f32> const& radians);
	[[nodiscard]] Matrix<3, 3, f32> Rotate(Vector<3, f32> const& axis, f32 amountRadians);
	[[nodiscard]] constexpr Matrix<3, 3, f32> Rotate(UnitQuat const& quat);
	[[nodiscard]] constexpr Matrix<4, 4, f32> Rotate_Homo(UnitQuat const& quat);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Rotate_Reduced(UnitQuat const& quat);
	
	[[nodiscard]] constexpr Vector<3, f32> ForwardVector(UnitQuat const& quat);
	[[nodiscard]] constexpr Vector<3, f32> UpVector(UnitQuat const& quat);
	[[nodiscard]] constexpr Vector<3, f32> RightVector(UnitQuat const& quat);

	[[nodiscard]] constexpr Matrix<3, 3, f32> Scale(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<3, 3, f32> Scale(Vector<3, f32> const& input);
	[[nodiscard]] constexpr Matrix<4, 4, f32> Scale_Homo(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 4, f32> Scale_Homo(Vector<3, f32> const& input);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Scale_Reduced(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Scale_Reduced(Vector<3, f32> const& input);

	[[nodiscard]] Matrix<4, 4, f32> LookAt_LH(
		Vector<3, f32> const& position, 
		Vector<3, f32> const& forward, 
		Vector<3, f32> const& upVector);
	[[nodiscard]] Matrix<4, 4, f32> LookAt_RH(
		Vector<3, f32> const& position,
		Vector<3, f32> const& forward,
		Vector<3, f32> const& upVector);

	[[nodiscard]] Matrix<4, 4, f32> Perspective_RH_ZO(
		f32 fovYRadians,
		f32 aspectRatio, 
		f32 zNear, 
		f32 zFar);
	[[nodiscard]] Matrix<4, 4, f32> Perspective_RH_NO(
		f32 fovY, 
		f32 aspectRatio, 
		f32 zNear, 
		f32 zFar);
	[[nodiscard]] Matrix<4, 4, f32> Perspective(
		API3D api, 
		f32 fovYRadians,
		f32 aspectRatio, 
		f32 zNear, 
		f32 zFar);
}

namespace DEngine::Math
{
	namespace LinTran3D = LinearTransform3D;
}

constexpr DEngine::Math::Matrix<4, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Multiply_Reduced(
	Matrix<4, 3, f32> const& left, 
	Matrix<4, 3, f32> const& right)
{
	Matrix<4, 3, f32> newMatrix{};
	for (uSize x = 0; x < 3; x += 1)
	{
		for (uSize y = 0; y < 3; y += 1)
		{
			f32 dot = 0.f;
			for (uSize i = 0; i < 3; i += 1)
				dot += left.At(i, y) * right.At(x, i);
			newMatrix.At(x, y) = dot;
		}
	}
	for (uSize y = 0; y < 3; y += 1)
	{
		f32 dot = 0.f;
		for (uSize i = 0; i < 3; i += 1)
			dot += left.At(i, y) * right.At(3, i);
		dot += left.At(3, y);
		newMatrix.At(3, y) = dot;
	}
	return newMatrix;
}

constexpr DEngine::Math::Vector<3, DEngine::f32> DEngine::Math::LinearTransform3D::Multiply_Reduced(
	Matrix<4, 3, f32> const& left,
	Vector<3, f32> const& right)
{
	Vector<3, f32> newVector{};
	for (size_t y = 0; y < 3; y++)
	{
		f32 dot = 0.f;
		for (size_t i = 0; i < 3; i++)
			dot += left.At(i, y) * right[i];
		newVector[y] = dot + left.At(3, y);
	}
	return newVector;
}

template<typename T>
constexpr DEngine::Math::Matrix<4, 4, T> DEngine::Math::LinearTransform3D::AsMat4(Matrix<4, 3, T> const& input)
{
	Matrix<4, 4, T> newMat;
	for (size_t x = 0; x < 4; x++)
	{
		for (size_t y = 0; y < 3; y++)
			newMat[x][y] = input[x][y];
	}
	newMat[0][3] = T(0);
	newMat[1][3] = T(0);
	newMat[2][3] = T(0);
	newMat[3][3] = T(1);
	return newMat;
}

constexpr DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Translate(Vector<3, f32> const& input)
{
	return Matrix<4, 4, f32>
	{ { {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		input.x, input.y, input.z, 1
	} } };
}

constexpr DEngine::Math::Matrix<4, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Translate_Reduced(Vector<3, f32> const& input)
{
	return Matrix<4, 3>
	{ { {
		1, 0, 0,
		0, 1, 0,
		0, 0, 1,
		input.x, input.y, input.z,
	} } };
}

constexpr void DEngine::Math::LinearTransform3D::SetTranslation(Matrix<4, 4, f32>& matrix, Vector<3, f32> const& input)
{
	matrix.At(3, 0) = input.x;
	matrix.At(3, 1) = input.y;
	matrix.At(3, 2) = input.z;
}

constexpr void DEngine::Math::LinearTransform3D::SetTranslation(Matrix<4, 3, f32>& matrix, Vector<3, f32> const& input)
{ 
	matrix.At(3, 0) = input.x;
	matrix.At(3, 1) = input.y;
	matrix.At(3, 2) = input.z;
}

constexpr DEngine::Math::Matrix<3, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Rotate(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();

	return Matrix<3, 3, f32>
	{ { {
		1 - 2 * Sqrd(y) - 2 * Sqrd(z), 2 * x * y + 2 * z * s, 2 * x * z - 2 * y * s,
		2 * x * y - 2 * z * s, 1 - 2 * Sqrd(x) - 2 * Sqrd(z), 2 * y * z + 2 * x * s,
		2 * x * z + 2 * y * s, 2 * y * z - 2 * x * s, 1 - 2 * Sqrd(x) - 2 * Sqrd(y),
	} } };
}

constexpr DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Rotate_Homo(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();
	return Mat4
	{ { {
		1 - (2 * y * y) - (2 * z * z),(2 * x * y) + (2 * z * s), (2 * x * z) - (2 * y * s), 0,
		(2 * x * y) - (2 * z * s), 1 - (2 * x * x) - (2 * z * z), (2 * y * z) + (2 * x * s), 0,
		(2 * x * z) + (2 * y * s), (2 * y * z) - (2 * x * s), 1 - (2 * x * x) - (2 * y * y), 0,
		0, 0, 0, 1
	} } };
}

constexpr DEngine::Math::Matrix<4, 3> DEngine::Math::LinearTransform3D::Rotate_Reduced(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();
	return Matrix<4, 3>
	{ { {
		1 - 2 * Sqrd(y) - 2 * Sqrd(z), 2 * x * y + 2 * z * s, 2 * x * z - 2 * y * s,
		2 * x * y - 2 * z * s, 1 - 2 * Sqrd(x) - 2 * Sqrd(z), 2 * y * z + 2 * x * s,
		2 * x * z + 2 * y * s, 2 * y * z - 2 * x * s, 1 - 2 * Sqrd(x) - 2 * Sqrd(y),
		0, 0, 0,
	} } };
}


constexpr DEngine::Math::Vector<3, DEngine::f32> DEngine::Math::LinearTransform3D::ForwardVector(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();

	Vector<3, f32> returnVal{
		(2*x*z) + (2*y*s),
		(2*y*z) - (2*x*s),
		1 - (2*x*x) - (2*y*y) };

	return returnVal;
}

constexpr DEngine::Math::Vector<3, DEngine::f32> DEngine::Math::LinearTransform3D::UpVector(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();

	Vector<3, f32> returnVal{
		(2*x*y) - (2*z*s),
		1 - (2*x*x) - (2*z*z),
		(2*y*z) + (2*x*s)
	};
	return returnVal;
}

constexpr DEngine::Math::Vector<3, DEngine::f32> DEngine::Math::LinearTransform3D::RightVector(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();

	Vector<3, f32> returnVal{
		1 - (2*y*y) - (2*z*z),
		(2*x*y) + (2*z*s),
		(2*x*z) - (2*y*s)
	};
	return returnVal;
}

constexpr DEngine::Math::Matrix<3, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Scale(f32 x, f32 y, f32 z)
{
	return Matrix<3, 3, f32>
	{ { {
		x, 0, 0,
		0, y, 0,
		0, 0, z
	} } };
}

constexpr DEngine::Math::Matrix<3, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Scale(Vector<3, f32> const& input)
{
	return Scale(input.x, input.y, input.z);
}

constexpr DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Scale_Homo(f32 x, f32 y, f32 z)
{
	return Matrix<4, 4, f32>
	{ { {
		x, 0, 0, 0,
		0, y, 0, 0,
		0, 0, z, 0,
		0, 0, 0, 1
	} } };
}

constexpr DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Scale_Homo(Vector<3, f32> const& input)
{
	return Scale_Homo(input.x, input.y, input.z);
}

constexpr DEngine::Math::Matrix<4, 3> DEngine::Math::LinearTransform3D::Scale_Reduced(f32 x, f32 y, f32 z)
{
	return Matrix<4, 3>
	{ { {
		x, 0, 0,
		0, y, 0,
		0, 0, z,
		0, 0, 0,
	} } };
}

constexpr DEngine::Math::Matrix<4, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Scale_Reduced(Vector<3, f32> const& input) 
{ 
	return Scale_Reduced(input.x, input.y, input.z); 
}