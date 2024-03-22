#pragma once

#include <DEngine/Math/Enums.hpp>
#include <DEngine/Math/Matrix.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Math/Trigonometric.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine::Math::LinearTransform3D
{
	[[nodiscard]] Matrix<4, 3, f32> Multiply_Reduced(
		Matrix<4, 3, f32> const& left, 
		Matrix<4, 3, f32> const& right);

	[[nodiscard]] Vector<3, f32> Multiply_Reduced(
		Matrix<4, 3, f32> const& left,
		Vector<3, f32> const& right);

	template<typename T>
	[[nodiscard]] constexpr Matrix<4, 4, T> AsMat4(Matrix<4, 3, T> const& input);

	[[nodiscard]] Matrix<4, 4, f32> Translate(Vector<3, f32> const& input);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Translate_Reduced(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Translate_Reduced(Vector<3, f32> const& input);

	void SetTranslation(Matrix<4, 4, f32>& matrix, Vector<3, f32> const& input);
	constexpr void SetTranslation(Matrix<4, 4, f32>& matrix, f32 x, f32 y, f32 z);
	void SetTranslation(Matrix<4, 3, f32>& matrix, Vector<3, f32> const& input);
	constexpr Vector<3, f32> GetTranslation(Matrix<4, 4, f32> const& input);
	constexpr Vector<3, f32> GetTranslation(Matrix<4, 3, f32> const& input);

	[[nodiscard]] Matrix<3, 3, f32> Rotate(ElementaryAxis axis, f32 amountRadians);
	[[nodiscard]] Matrix<4, 4, f32> Rotate_Homo(ElementaryAxis axis, f32 amountRadians);
	[[nodiscard]] Matrix<3, 3, f32> Rotate(f32 xRadians, f32 yRadians, f32 zRadians);
	[[nodiscard]] Matrix<3, 3, f32> Rotate(Vector<3, f32> const& radians);
	[[nodiscard]] Matrix<3, 3, f32> Rotate(Vector<3, f32> const& axis, f32 amountRadians);
	// Rotate a vector by a unit quaternion.
	[[nodiscard]] constexpr Vector<3, f32> Rotate(Vector<3, f32> const& in, UnitQuat const& rotation) noexcept;
	[[nodiscard]] Matrix<3, 3, f32> Rotate(UnitQuat const& quat);
	[[nodiscard]] constexpr Matrix<4, 4, f32> Rotate_Homo(UnitQuat const& quat);
	[[nodiscard]] Matrix<4, 3, f32> Rotate_Reduced(UnitQuat const& quat);
	
	[[nodiscard]] constexpr Vector<3, f32> ForwardVector(UnitQuat const& quat);
	[[nodiscard]] constexpr Vector<3, f32> UpVector(UnitQuat const& quat);
	[[nodiscard]] constexpr Vector<3, f32> RightVector(UnitQuat const& quat);

	[[nodiscard]] Matrix<3, 3, f32> Scale(f32 x, f32 y, f32 z);
	[[nodiscard]] Matrix<3, 3, f32> Scale(Vector<3, f32> const& input);
	[[nodiscard]] Matrix<4, 4, f32> Scale_Homo(f32 x, f32 y, f32 z);
	[[nodiscard]] Matrix<4, 4, f32> Scale_Homo(Vector<3, f32> const& input);
	[[nodiscard]] Matrix<4, 3, f32> Scale_Reduced(f32 x, f32 y, f32 z);
	[[nodiscard]] Matrix<4, 3, f32> Scale_Reduced(Vector<3, f32> const& input);

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
	namespace LinAlg3D = LinearTransform3D;
}



template<typename T>
constexpr DEngine::Math::Matrix<4, 4, T> DEngine::Math::LinearTransform3D::AsMat4(Matrix<4, 3, T> const& input)
{
	Matrix<4, 4, T> newMat;
	for (uSize x = 0; x < 4; x++)
	{
		for (uSize y = 0; y < 3; y++)
			newMat[x][y] = input[x][y];
	}
	newMat[0][3] = T(0);
	newMat[1][3] = T(0);
	newMat[2][3] = T(0);
	newMat[3][3] = T(1);
	return newMat;
}

constexpr DEngine::Math::Matrix<4, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Translate_Reduced(Vector<3, f32> const& input)
{
	return Matrix<4, 3>
	{
		1, 0, 0,
		0, 1, 0,
		0, 0, 1,
		input.x, input.y, input.z,
	};
}

constexpr void DEngine::Math::LinearTransform3D::SetTranslation(Matrix<4, 4, f32>& matrix, f32 x, f32 y, f32 z)
{
	matrix.At(3, 0) = x;
	matrix.At(3, 1) = y;
	matrix.At(3, 2) = z;
}

constexpr DEngine::Math::Vector<3, DEngine::f32> DEngine::Math::LinearTransform3D::Rotate(
	Vector<3, f32> const& in, 
	UnitQuat const& rotation) noexcept
{
	// Reference:
	// https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion

	auto const& v = in;

	// Extract the vector part of the quaternion
	Math::Vector<3, f32> const u = { rotation.X(), rotation.Y(), rotation.Z() };

	// Extract the scalar part of the quaternion
	f32 const s = rotation.S();

	// Do the math
	return 2.0f * Vector<3, f32>::Dot(u, v) * u
		+ (s * s - Vector<3, f32>::Dot(u, u)) * v
		+ 2.0f * s * Vector<3, f32>::Cross(u, v);
}



constexpr DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Rotate_Homo(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();
	return Mat4
	{
		1 - (2 * y * y) - (2 * z * z),(2 * x * y) + (2 * z * s), (2 * x * z) - (2 * y * s), 0,
		(2 * x * y) - (2 * z * s), 1 - (2 * x * x) - (2 * z * z), (2 * y * z) + (2 * x * s), 0,
		(2 * x * z) + (2 * y * s), (2 * y * z) - (2 * x * s), 1 - (2 * x * x) - (2 * y * y), 0,
		0, 0, 0, 1
	};
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

	Vector<3, f32> returnVal {
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

