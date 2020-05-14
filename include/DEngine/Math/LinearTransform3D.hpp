#pragma once

#include "DEngine/Math/Trigonometric.hpp"
#include "DEngine/Math/Matrix.hpp"
#include "DEngine/Math/Vector.hpp"
#include "DEngine/Math/Enums.hpp"
#include "DEngine/Math/Setup.hpp"

namespace DEngine::Math::LinearTransform3D
{
	[[nodiscard]] constexpr Matrix<4, 3, f32> Multiply_Reduced(
		Matrix<4, 3, f32> const& left, 
		Matrix<4, 3, f32> const& right);

	[[nodiscard]] constexpr Vector<3, f32> Multiply_Reduced(
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

	[[nodiscard]] constexpr Matrix<4, 4, f32> Translate(Vector<3, f32> const& input);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Translate_Reduced(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Translate_Reduced(Vector<3, f32> const& input);

	constexpr void SetTranslation(Matrix<4, 4, f32>& matrix, Vector<3, f32> const& input);
	constexpr void SetTranslation(Matrix<4, 3, f32>& matrix, Vector<3, f32> const& input);
	constexpr Vector<3, f32> GetTranslation(Matrix<4, 4, f32> const& input);
	constexpr Vector<3, f32> GetTranslation(Matrix<4, 3, f32> const& input);

	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate(ElementaryAxis axis, f32 amount);
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate_Homo(ElementaryAxis axis, f32 amount);
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate(f32 x, f32 y, f32 z);
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate(Vector<3, f32> const& angles);
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] Mat3 Rotate(Vector<3, f32> const& axis, f32 amount);
	[[nodiscard]] constexpr Matrix<3, 3, f32> Rotate(UnitQuat const& quat);
	[[nodiscard]] constexpr Mat4 Rotate_Homo(UnitQuat const& quat);
	[[nodiscard]] constexpr Matrix<4, 3> Rotate_Reduced(UnitQuat const& quat);

	[[nodiscard]] constexpr Vector<3, f32> ForwardVector(UnitQuat const& quat);
	[[nodiscard]] constexpr Vector<3, f32> UpVector(UnitQuat const& quat);
	[[nodiscard]] constexpr Vector<3, f32> RightVector(UnitQuat const& quat);

	[[nodiscard]] constexpr Matrix<3, 3, f32> Scale(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<3, 3, f32> Scale(Vector<3, f32> const& input);
	[[nodiscard]] constexpr Matrix<4, 4, f32> Scale_Homo(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 4, f32> Scale_Homo(Vector<3, f32> const& input);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Scale_Reduced(f32 x, f32 y, f32 z);
	[[nodiscard]] constexpr Matrix<4, 3, f32> Scale_Reduced(Vector<3, f32> const& input);

	[[nodiscard]] inline Matrix<4, 4, f32> LookAt_LH(
		Vector<3, f32> const& position, 
		Vector<3, f32> const& forward, 
		Vector<3, f32> const& upVector);
	[[nodiscard]] inline Matrix<4, 4, f32> LookAt_RH(
		Vector<3, f32> const& position,
		Vector<3, f32> const& forward,
		Vector<3, f32> const& upVector);

	[[nodiscard]] inline Matrix<4, 4, f32> Perspective_RH_ZO(
		f32 fovY, 
		f32 aspectRatio, 
		f32 zNear, 
		f32 zFar);
	[[nodiscard]] inline Matrix<4, 4, f32> Perspective_RH_NO(
		f32 fovY, 
		f32 aspectRatio, 
		f32 zNear, 
		f32 zFar);
	[[nodiscard]] inline Matrix<4, 4, f32> Perspective(
		API3D api, 
		f32 fovY, 
		f32 aspectRatio, 
		f32 zNear, 
		f32 zFar);

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

constexpr DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Translate(Vector<3, f32> const& input)
{
	return Mat4
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
DEngine::Math::Matrix<3, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Rotate(Vector<3, f32> const& angles)
{
	return Rotate<angleUnit>(angles.x, angles.y, angles.z);
}

template<DEngine::Math::AngleUnit angleUnit>
DEngine::Math::Matrix<3, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Rotate(Vector<3, f32> const& axisInput, f32 amount)
{
	Vector<3, f32> axis = axisInput.Normalized();
	const f32 cos = Cos<angleUnit>(amount);
	const f32 sin = Sin<angleUnit>(amount);
	return Matrix<3, 3, f32>
	{
		cos + Sqrd(axis.x)*(1 - cos), axis.x*axis.y*(1 - cos) - axis.z*sin, axis.x*axis.z*(1 - cos) + axis.y*sin,
		axis.y*axis.x*(1 - cos) + axis.z*sin, cos + Sqrd(axis.y)*(1 - cos), axis.y*axis.z*(1 - cos) - axis.x*sin,
		axis.z*axis.x*(1 - cos) - axis.y*sin, axis.z*axis.y*(1 - cos) + axis.x*sin, cos + Sqrd(axis.z)*(1 - cos)
	}.Transposed();
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

constexpr DEngine::Math::Vector<3, DEngine::f32> DEngine::Math::LinearTransform3D::ForwardVector(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();

	Vector<3, f32> returnVal{
		(2*x*z) + (2*y*s),
		(2*y*z) - (2*x*s),
		1 - (2*x*x) - (2*y*y)
	};

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
	{
		x, 0, 0,
		0, y, 0,
		0, 0, z
	};
}

constexpr DEngine::Math::Matrix<3, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Scale(Vector<3, f32> const& input)
{
	return Scale(input.x, input.y, input.z);
}

constexpr DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Scale_Homo(f32 x, f32 y, f32 z)
{
	return Matrix<4, 4, f32>
	{
		 x, 0, 0, 0,
		 0, y, 0, 0,
		 0, 0, z, 0,
		 0, 0, 0, 1
	};
}

constexpr DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Scale_Homo(Vector<3, f32> const& input)
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

constexpr DEngine::Math::Matrix<4, 3, DEngine::f32> DEngine::Math::LinearTransform3D::Scale_Reduced(Vector<3, f32> const& input) 
{ 
	return Scale_Reduced(input.x, input.y, input.z); 
}

inline DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::LookAt_LH(
	Vector<3, f32> const& position, 
	Vector<3, f32> const& forward, 
	Vector<3, f32> const& upVector)
{
	Vector<3, f32> zAxis = (forward - position).Normalized();
	Vector<3, f32> xAxis = Vector<3, f32>::Cross(upVector, zAxis).Normalized();
	Vector<3, f32> yAxis = Vector<3, f32>::Cross(zAxis, xAxis);
	return Mat4
	({
		xAxis.x, yAxis.x, zAxis.x, 0,
		xAxis.y, yAxis.y, zAxis.y, 0,
		xAxis.z, yAxis.z, zAxis.z, 0,
		-Vector<3, f32>::Dot(xAxis, position), -Vector<3, f32>::Dot(yAxis, position), -Vector<3, f32>::Dot(zAxis, position), 1
	});
}

inline DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::LookAt_RH(
	Vector<3, f32> const& position, 
	Vector<3, f32> const& forward, 
	Vector<3, f32> const& upVector)
{
	Vector<3, f32> zAxis = (position - forward).Normalized();
	Vector<3, f32> xAxis = Vector<3, f32>::Cross(upVector, zAxis).Normalized();
	Vector<3, f32> yAxis = Vector<3, f32>::Cross(zAxis, xAxis);
	return Mat4
	{
		xAxis.x, yAxis.x, zAxis.x, 0,
		xAxis.y, yAxis.y, zAxis.y, 0,
		xAxis.z, yAxis.z, zAxis.z, 0,
		-Vector<3, f32>::Dot(xAxis, position), -Vector<3, f32>::Dot(yAxis, position), -Vector<3, f32>::Dot(zAxis, position), 1
	};
}

inline DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Perspective_RH_ZO(
	f32 fovY, 
	f32 aspectRatio, 
	f32 zNear, 
	f32 zFar)
{
	f32 const tanHalfFovy = Tan<AngleUnit::Degrees>(fovY / 2);
	return Matrix<4, 4, f32>
	{
		1 / (aspectRatio * tanHalfFovy), 0, 0, 0,
		0, 1 / tanHalfFovy, 0, 0,
		0, 0, zFar / (zNear - zFar), -1,
		0, 0, -(zFar * zNear) / (zFar - zNear), 0
	};
}

inline DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Perspective_RH_NO(
	f32 fovY, 
	f32 aspectRatio,
	f32 zNear,
	f32 zFar)
{
	f32 const tanHalfFovy = Tan<AngleUnit::Degrees>(fovY / 2);
	return Matrix<4, 4, f32>
	{
		1 / (aspectRatio * tanHalfFovy), 0, 0, 0,
		0, 1 / tanHalfFovy, 0, 0,
		0, 0, -(zFar + zNear) / (zFar - zNear), -1,
		0, 0, -(2 * zFar * zNear) / (zFar - zNear), 0
	};
}

inline DEngine::Math::Matrix<4, 4, DEngine::f32> DEngine::Math::LinearTransform3D::Perspective(
	API3D api, 
	f32 fovY, 
	f32 aspectRatio, 
	f32 zNear, 
	f32 zFar)
{
	switch (api)
	{
	case API3D::OpenGL:
		return Perspective_RH_NO(fovY, aspectRatio, zNear, zFar);
	case API3D::Vulkan:
		return Perspective_RH_ZO(fovY, aspectRatio, zNear, zFar);
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