#pragma once

#include "DEngine/Math/Trigonometric.hpp"
#include "DEngine/Math/Matrix/Matrix.hpp"
#include "DEngine/Math/Vector/Vector.hpp"
#include "DEngine/Math/Enums.hpp"
#include "DEngine/Math/Setup.hpp"

namespace DEngine::Math::LinearTransform3D
{
	template<typename T>
	[[nodiscard]] constexpr Matrix<4, 3, T> Multiply_Reduced(Matrix<4, 3, T> const& left, Matrix<4, 3, T> const& right)
	{
		Matrix<4, 3, T> newMatrix{};
		for (uSize x = 0; x < 3; x += 1)
		{
			for (uSize y = 0; y < 3; y += 1)
			{
				T dot{};
				for (uSize i = 0; i < 3; i += 1)
					dot += left[i][y] * right[x][i];
				newMatrix[x][y] = dot;
			}
		}

		for (uSize y = 0; y < 3; y += 1)
		{
			T dot{};
			for (uSize i = 0; i < 3; i += 1)
				dot += left[i][y] * right[3][i];
			dot += left[3][y];
			newMatrix[3][y] = dot;
		}

		return newMatrix;
	}
	template<typename T>
	[[nodiscard]] constexpr Vector<3, T> Multiply_Reduced(const Matrix<4, 3, T>& left, const Vector<3, T>& right)
	{
		Vector<3, T> newVector;

		for (size_t y = 0; y < 3; y++)
		{
			T dot = T(0);
			for (size_t i = 0; i < 3; i++)
				dot += left[i][y] * right[i];
			newVector[y] = dot + left[3][y];
		}

		return newVector;
	}
	template<typename T>
	[[nodiscard]] constexpr Matrix<4, 4, T> AsMat4(const Matrix<4, 3, T>& input)
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

	[[nodiscard]] constexpr Mat4 Translate(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Mat4 Translate(Vec3D const& input);
	[[nodiscard]] constexpr Matrix<4, 3> Translate_Reduced(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 3> Translate_Reduced(Vec3D const& input);

	constexpr void SetTranslation(Mat4& matrix, f32 x, f32 y, f32 z);
	constexpr void SetTranslation(Mat4& matrix, Vec3D const& input);
	constexpr void SetTranslation(Matrix<4, 3>& matrix, f32 x, f32 y, f32 z);
	constexpr void SetTranslation(Matrix<4, 3>& matrix, Vec3D const& input);
	template<typename T = f32>
	constexpr Vector<3, T> GetTranslation(Matrix<4, 4, T> const& input);
	template<typename T = f32>
	constexpr Vector<3, T> GetTranslation(Matrix<4, 3, T> const& input);

	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate(ElementaryAxis axis, f32 amount);
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate_Homo(ElementaryAxis axis, f32 amount);
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate(f32 x, f32 y, f32 z);
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate(Vec3D const& angles);
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate(Vec3D const& axis, f32 amount);
	template<typename T>
	[[nodiscard]] constexpr Matrix<3, 3, T> Rotate(UnitQuaternion<T> const& quat);
	[[nodiscard]] constexpr Mat4 Rotate_Homo(UnitQuat const& quat);
	[[nodiscard]] constexpr Matrix<4, 3> Rotate_Reduced(UnitQuat const& quat);

	[[nodiscard]] constexpr Vec3D ForwardVector(UnitQuat const& quat);
	[[nodiscard]] constexpr Vec3D UpVector(UnitQuat const& quat);
	[[nodiscard]] constexpr Vec3D RightVector(UnitQuat const& quat);

	[[nodiscard]] constexpr Mat3 Scale(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Mat3 Scale(Vec3D const& input);
	[[nodiscard]] constexpr Mat4 Scale_Homo(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Mat4 Scale_Homo(Vec3D const& input);
	[[nodiscard]] constexpr Matrix<4, 3> Scale_Reduced(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 3> Scale_Reduced(Vec3D const& input);

	[[nodiscard]] Mat4 LookAt_LH(Vec3D const& position, Vec3D const& forward, Vec3D const& upVector);
	[[nodiscard]] Mat4 LookAt_RH(Vec3D const& position, Vec3D const& forward, Vec3D const& upVector);

	template<typename T = f32>
	[[nodiscard]] Matrix<4, 4, T> Perspective_RH_ZO(T fovY, T aspectRatio, T zNear, T zFar);
	template<typename T = f32>
	[[nodiscard]] Matrix<4, 4, T> Perspective_RH_NO(T fovY, T aspectRatio, T zNear, T zFar);
	template<typename T = f32>
	[[nodiscard]] Matrix<4, 4, T> Perspective(API3D api, T fovY, T aspectRatio, T zNear, T zFar);

	template<typename T = f32>
	[[nodiscard]] Matrix<4, 4, T> Orthographic_RH_ZO(T left, T right, T bottom, T top, T zNear, T zFar);
	template<typename T = f32>
	[[nodiscard]] Matrix<4, 4, T> Orthographic_RH_NO(T left, T right, T bottom, T top, T zNear, T zFar);
	template<typename T = f32>
	[[nodiscard]] Matrix<4, 4, T> Orthographic(API3D api, T left, T right, T bottom, T top, T zNear, T zFar);

	
}

namespace DEngine::Math
{
	namespace LinTran3D = LinearTransform3D;
}

constexpr DEngine::Math::Mat4 DEngine::Math::LinearTransform3D::Translate(f32 x, f32 y, f32 z)
{
	return Mat4
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		x, y, z, 1
	};
}

constexpr DEngine::Math::Mat4 DEngine::Math::LinearTransform3D::Translate(Vec3D const& input)
{
	return Translate(input.x, input.y, input.z);
}

constexpr DEngine::Math::Matrix<4, 3> DEngine::Math::LinearTransform3D::Translate_Reduced(f32 x, f32 y, f32 z)
{
	return Matrix<4, 3>
	{
		1, 0, 0,
		0, 1, 0,
		0, 0, 1,
		x, y, z,
	};
}

constexpr DEngine::Math::Matrix<4, 3> DEngine::Math::LinearTransform3D::Translate_Reduced(Vec3D const& input)
{ 
	return Translate_Reduced(input.x, input.y, input.z); 
}

constexpr void DEngine::Math::LinearTransform3D::SetTranslation(Mat4& matrix, f32 x, f32 y, f32 z)
{
	matrix.At(3, 0) = x;
	matrix.At(3, 1) = y;
	matrix.At(3, 2) = z;
}

constexpr void DEngine::Math::LinearTransform3D::SetTranslation(Mat4& matrix, Vec3D const& input)
{
	SetTranslation(matrix, input.x, input.y, input.z);
}

constexpr void DEngine::Math::LinearTransform3D::SetTranslation(Matrix<4, 3>& matrix, f32 x, f32 y, f32 z)
{
	matrix.At(3, 0) = x;
	matrix.At(3, 1) = y;
	matrix.At(3, 2) = z;
}

constexpr void DEngine::Math::LinearTransform3D::SetTranslation(Matrix<4, 3>& matrix, Vec3D const& input)
{ 
	SetTranslation(matrix, input.x, input.y, input.z);
}

template<typename T>
constexpr DEngine::Math::Vector<3, T> DEngine::Math::LinearTransform3D::GetTranslation(Matrix<4, 4, T> const& input)
{
	return Vector<3, T>{ input.At(3, 0), input.At(3, 1), input.At(3, 2) };
}

template<typename T>
constexpr DEngine::Math::Vector<3, T> DEngine::Math::LinearTransform3D::GetTranslation(Matrix<4, 3, T> const& input)
{
	return Vector<3, T>{ input.At(3, 0), input.At(3, 1), input.At(3, 2) };
}

/*
template<DEngine::Math::AngleUnit angleUnit>
DEngine::Math::Mat4 DEngine::Math::LinearTransform3D::Rotate_Homo(ElementaryAxis axis, f32 amount)
{
#if defined( _MSC_VER )
	__assume(axis == Math::ElementaryAxis::X || axis == Math::ElementaryAxis::Y || axis == Math::ElementaryAxis::Z);
#endif
	f32 const cos = Cos<angleUnit>(amount);
	f32 const sin = Sin<angleUnit>(amount);
	switch (axis)
	{
	case ElementaryAxis::X:
		return Mat4
		({
			1, 0, 0, 0,
			0, cos, sin, 0,
			0, -sin, cos, 0,
			0, 0, 0, 1
		});
	case ElementaryAxis::Y:
		return Mat4
		({
			cos, 0, -sin, 0,
			0, 1, 0, 0,
			sin, 0, cos, 0,
			0, 0, 0, 1
		});
	case ElementaryAxis::Z:
		return Mat4
		({
			cos, sin, 0, 0,
			-sin, cos, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		});
	default:
		return {};
	};
}
*/

template<DEngine::Math::AngleUnit angleUnit>
DEngine::Math::Mat3 DEngine::Math::LinearTransform3D::Rotate(f32 x, f32 y, f32 z)
{
	return Rotate<angleUnit>(ElementaryAxis::Z, z) * Rotate<angleUnit>(ElementaryAxis::Y, y) * Rotate<angleUnit>(ElementaryAxis::X, x);
}

template<DEngine::Math::AngleUnit angleUnit>
DEngine::Math::Mat3 DEngine::Math::LinearTransform3D::Rotate(Vec3D const& angles)
{
	return Rotate<angleUnit>(angles.x, angles.y, angles.z);
}

template<DEngine::Math::AngleUnit angleUnit>
DEngine::Math::Mat3 DEngine::Math::LinearTransform3D::Rotate(Vec3D const& axisInput, f32 amount)
{
	Vec3D axis = axisInput.GetNormalized();
	const float cos = Cos<angleUnit>(amount);
	const float sin = Sin<angleUnit>(amount);
	return Matrix3x3
	{
		cos + Sqrd(axis.x)*(1 - cos), axis.x*axis.y*(1 - cos) - axis.z*sin, axis.x*axis.z*(1 - cos) + axis.y*sin,
		axis.y*axis.x*(1 - cos) + axis.z*sin, cos + Sqrd(axis.y)*(1 - cos), axis.y*axis.z*(1 - cos) - axis.x*sin,
		axis.z*axis.x*(1 - cos) - axis.y*sin, axis.z*axis.y*(1 - cos) + axis.x*sin, cos + Sqrd(axis.z)*(1 - cos)
	}.GetTransposed();
}

template<typename T>
[[nodiscard]] constexpr DEngine::Math::Matrix<3, 3, T> DEngine::Math::LinearTransform3D::Rotate(UnitQuaternion<T> const& quat)
{
	T const& s = quat.S();
	T const& x = quat.X();
	T const& y = quat.Y();
	T const& z = quat.Z();

	return Matrix<3, 3, T>
	{
		1 - 2 * Sqrd(y) - 2 * Sqrd(z), 2 * x* y + 2 * z * s, 2 * x* z - 2 * y * s,
		2 * x* y - 2 * z * s, 1 - 2 * Sqrd(x) - 2 * Sqrd(z), 2 * y* z + 2 * x * s,
		2 * x* z + 2 * y * s, 2 * y* z - 2 * x * s, 1 - 2 * Sqrd(x) - 2 * Sqrd(y),
	};
}

constexpr DEngine::Math::Mat4 DEngine::Math::LinearTransform3D::Rotate_Homo(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();
	return Mat4
	{
		1 - (2*y*y) - (2*z*z),(2*x*y) + (2*z*s), (2*x*z) - (2*y*s), 0,
		(2*x*y) - (2*z*s), 1 - (2*x*x) - (2*z*z), (2*y*z) + (2*x*s), 0,
		(2*x*z) + (2*y*s), (2*y*z) - (2*x*s), 1 - (2*x*x) - (2*y*y), 0,
		0, 0, 0, 1
	};
}

constexpr DEngine::Math::Matrix<4, 3> DEngine::Math::LinearTransform3D::Rotate_Reduced(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();
	return Matrix<4, 3>
	{
		1 - 2 * Sqrd(y) - 2 * Sqrd(z), 2 * x * y + 2 * z * s, 2 * x * z - 2 * y * s,
		2 * x * y - 2 * z * s, 1 - 2 * Sqrd(x) - 2 * Sqrd(z), 2 * y * z + 2 * x * s,
		2 * x * z + 2 * y * s, 2 * y * z - 2 * x * s, 1 - 2 * Sqrd(x) - 2 * Sqrd(y),
		0, 0, 0,
	};
}

constexpr DEngine::Math::Vec3D DEngine::Math::LinearTransform3D::ForwardVector(UnitQuat const& quat)
{
	auto s = quat.S();
	auto x = quat.X();
	auto y = quat.Y();
	auto z = quat.Z();

	Vec3D returnVal{};
	returnVal.x = (2*x*z) + (2*y*s);
	returnVal.y = (2*y*z) - (2*x*s);
	returnVal.z = 1 - (2*x*x) - (2*y*y);

	return returnVal;
}

constexpr DEngine::Math::Vec3D DEngine::Math::LinearTransform3D::UpVector(UnitQuat const& quat)
{
	auto s = quat.S();
	auto x = quat.X();
	auto y = quat.Y();
	auto z = quat.Z();

	Vec3D returnVal{};
	returnVal.x = (2*x*y) - (2*z*s);
	returnVal.y = 1 - (2*x*x) - (2*z*z);
	returnVal.z = (2*y*z) + (2*x*s);
	return returnVal;
}

constexpr DEngine::Math::Vec3D DEngine::Math::LinearTransform3D::RightVector(UnitQuat const& quat)
{
	auto s = quat.S();
	auto x = quat.X();
	auto y = quat.Y();
	auto z = quat.Z();

	Vec3D returnVal{};
	returnVal.x = 1 - (2*y*y) - (2*z*z);
	returnVal.y = (2*x*y) + (2*z*s);
	returnVal.z = (2*x*z) - (2*y*s);
	return returnVal;
}

constexpr DEngine::Math::Mat3 DEngine::Math::LinearTransform3D::Scale(f32 x, f32 y, f32 z)
{
	return Mat3
	{
		x, 0, 0,
		0, y, 0,
		0, 0, z
	};
}

constexpr DEngine::Math::Mat3 DEngine::Math::LinearTransform3D::Scale(Vec3D const& input)
{
	return Scale(input.x, input.y, input.z);
}

constexpr DEngine::Math::Mat4 DEngine::Math::LinearTransform3D::Scale_Homo(f32 x, f32 y, f32 z)
{
	return Mat4
	{
		 x, 0, 0, 0,
		 0, y, 0, 0,
		 0, 0, z, 0,
		 0, 0, 0, 1
	};
}

constexpr DEngine::Math::Mat4 DEngine::Math::LinearTransform3D::Scale_Homo(Vec3D const& input)
{
	return Scale_Homo(input.x, input.y, input.z);
}

constexpr DEngine::Math::Matrix<4, 3> DEngine::Math::LinearTransform3D::Scale_Reduced(f32 x, f32 y, f32 z)
{
	return Matrix<4, 3>
	{
		x, 0, 0,
		0, y, 0,
		0, 0, z,
		0, 0, 0,
	};
}

constexpr DEngine::Math::Matrix<4, 3> DEngine::Math::LinearTransform3D::Scale_Reduced(Vec3D const& input) 
{ 
	return Scale_Reduced(input.x, input.y, input.z); 
}

inline DEngine::Math::Mat4 DEngine::Math::LinearTransform3D::LookAt_LH(Vec3D const& position, Vec3D const& forward, Vec3D const& upVector)
{
	Vec3D zAxis = (forward - position).Normalized();
	Vec3D xAxis = Vec3D::Cross(upVector, zAxis).Normalized();
	Vec3D yAxis = Vec3D::Cross(zAxis, xAxis);
	return Mat4
	({
		xAxis.x, yAxis.x, zAxis.x, 0,
		xAxis.y, yAxis.y, zAxis.y, 0,
		xAxis.z, yAxis.z, zAxis.z, 0,
		-Vec3D::Dot(xAxis, position), -Vec3D::Dot(yAxis, position), -Vec3D::Dot(zAxis, position), 1
	});
}

inline DEngine::Math::Mat4 DEngine::Math::LinearTransform3D::LookAt_RH(Vec3D const& position, Vec3D const& forward, Vec3D const& upVector)
{
	Vec3D zAxis = (position - forward).Normalized();
	Vec3D xAxis = Vec3D::Cross(upVector, zAxis).Normalized();
	Vec3D yAxis = Vec3D::Cross(zAxis, xAxis);
	return Mat4
	{
		xAxis.x, yAxis.x, zAxis.x, 0,
		xAxis.y, yAxis.y, zAxis.y, 0,
		xAxis.z, yAxis.z, zAxis.z, 0,
		-Vec3D::Dot(xAxis, position), -Vec3D::Dot(yAxis, position), -Vec3D::Dot(zAxis, position), 1
	};
}

template<typename T>
inline DEngine::Math::Matrix<4, 4, T> DEngine::Math::LinearTransform3D::Perspective_RH_ZO(
	T fovY, 
	T aspectRatio, 
	T zNear, 
	T zFar)
{
	T const tanHalfFovy = Tan<AngleUnit::Degrees>(fovY / 2);
	return Matrix<4, 4, T>
	{
		1 / (aspectRatio * tanHalfFovy), 0, 0, 0,
		0, 1 / tanHalfFovy, 0, 0,
		0, 0, zFar / (zNear - zFar), -1,
		0, 0, -(zFar * zNear) / (zFar - zNear), 0
	};
}

template<typename T>
inline DEngine::Math::Matrix<4, 4, T> DEngine::Math::LinearTransform3D::Perspective_RH_NO(T fovY, T aspectRatio, T zNear, T zFar)
{
	T const tanHalfFovy = Tan<AngleUnit::Degrees>(fovY / 2);
	return Matrix<4, 4, T>
	{
		1 / (aspectRatio * tanHalfFovy), 0, 0, 0,
		0, 1 / tanHalfFovy, 0, 0,
		0, 0, -(zFar + zNear) / (zFar - zNear), -1,
		0, 0, -(2 * zFar * zNear) / (zFar - zNear), 0
	};
}

template<typename T>
inline DEngine::Math::Matrix<4, 4, T> DEngine::Math::LinearTransform3D::Perspective(API3D api, T fovY, T aspectRatio, T zNear, T zFar)
{
	switch (api)
	{
	case API3D::OpenGL:
		return Perspective_RH_NO<T>(fovY, aspectRatio, zNear, zFar);
	case API3D::Vulkan:
		return Perspective_RH_ZO<T>(fovY, aspectRatio, zNear, zFar);
	default:
		return {};
	}
}

template<typename T>
DEngine::Math::Matrix<4, 4, T> DEngine::Math::LinearTransform3D::Orthographic_RH_ZO(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return Matrix<4, 4, T>
	{
		2 / (right - left), 0, 0, 0,
		0, 2 / (top - bottom), 0, 0,
		0, 0, -(1 / (zFar - zNear)), 0,
		-(right + left) / (right - left), -(top + bottom) / (top - bottom), -zNear / (zFar - zNear), 1
	};
}

template<typename T>
DEngine::Math::Matrix<4, 4, T> DEngine::Math::LinearTransform3D::Orthographic_RH_NO(T left, T right, T bottom, T top, T zNear, T zFar)
{
	return Matrix<4, 4, T>
	{
		2 / (right - left), 0, 0, 0,
		0, 2 / (top - bottom), 0, 0,
		0, 0, -(2 / (zFar - zNear)), 0,
		-(right + left) / (right - left), -(top + bottom) / (top - bottom), -zNear / (zFar - zNear), 1
	};
}

template<typename T>
DEngine::Math::Matrix<4, 4, T> DEngine::Math::LinearTransform3D::Orthographic(API3D api, T left, T right, T bottom, T top, T zNear, T zFar)
{
	switch (api)
	{
	case API3D::OpenGL:
		return Orthographic_RH_NO<T>(left, right, bottom, top, zNear, zFar);
	case API3D::Vulkan:
		return Orthographic_RH_ZO<T>(left, right, bottom, top, zNear, zFar);
	default:
		return {};
	}
}