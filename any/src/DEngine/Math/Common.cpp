#include <DEngine/Math/Common.hpp>
#include <DEngine/Math/Constant.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>
#include <DEngine/Math/Trigonometric.hpp>

#include <cmath>

using namespace DEngine;

f32 Math::Ceil(f32 input)
{
	return ceilf(input);
}

f64 Math::Ceil(f64 input)
{
	return ceill(input);
}

f32 Math::Round(f32 input)
{
	return roundf(input);
}

f32 Math::Sqrt(f32 input)
{
	return sqrtf(input);
}

f64 Math::Sqrt(f64 input)
{
	return sqrtl(input);
}

f32 Math::Pow(f32 coefficient, f32 exponent)
{
	return powf(coefficient, exponent);
}

f64 Math::Pow(f64 coefficient, f64 exponent)
{
	return powl(coefficient, exponent);
}

f32 Math::Sin(f32 radians) noexcept
{
	return sinf(radians);
}

f32 Math::Cos(f32 radians) noexcept
{
	return cosf(radians);
}

f32 Math::ArcCos(f32 in) noexcept
{
	return acosf(in);
}

f32 Math::Tan(f32 radians) noexcept
{
	return tanf(radians);
}

f32 Math::ArcTan2(f32 a, f32 b) noexcept
{
	return atan2f(a, b);
}

Math::Matrix<3, 3, f32> Math::LinearTransform3D::Rotate(ElementaryAxis axis, f32 amountRadians)
{
	f32 const cos = Cos(amountRadians);
	f32 const sin = Sin(amountRadians);
	switch (axis)
	{
	case ElementaryAxis::X:
		return Matrix<3, 3, f32>
		{ { {
			1, 0, 0,
			0, cos, sin,
			0, -sin, cos
		} } };
	case ElementaryAxis::Y:
		return Matrix<3, 3, f32>
		{ { {
			cos, 0, -sin,
			0, 1, 0,
			sin, 0, cos
			} } };
	case ElementaryAxis::Z:
		return Matrix<3, 3, f32>
		{ { {
			cos, sin, 0,
			-sin, cos, 0,
			0, 0, 1
		} } };
	default:
		return {};
	};
}

Math::Matrix<4, 4, f32> Math::LinearTransform3D::Rotate_Homo(ElementaryAxis axis, f32 amountRadians)
{
	f32 const cos = Cos(amountRadians);
	f32 const sin = Sin(amountRadians);
	switch (axis)
	{
	case ElementaryAxis::X:
		return Matrix<4, 4, f32>
		{ { {
			1, 0, 0, 0,
			0, cos, sin, 0,
			0, -sin, cos, 0,
			0, 0, 0, 1
		} } };
	case ElementaryAxis::Y:
		return Matrix<4, 4, f32>
		{ { {
			cos, 0, -sin, 0,
			0, 1, 0, 0,
			sin, 0, cos, 0,
			0, 0, 0, 1
		} } };
	case ElementaryAxis::Z:
		return Matrix<4, 4, f32>
		{ { {
			cos, sin, 0, 0,
			-sin, cos, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		} } };
	default:
		DENGINE_IMPL_UNREACHABLE();
		return {};
	};
}

Math::Matrix<3, 3, f32> Math::LinearTransform3D::Rotate(f32 xRadians, f32 yRadians, f32 zRadians)
{
	return Rotate(ElementaryAxis::Z, zRadians) * Rotate(ElementaryAxis::Y, yRadians) * Rotate(ElementaryAxis::X, xRadians);
}

Math::Matrix<3, 3, f32> Math::LinearTransform3D::Rotate(Vector<3, f32> const& radians)
{
	return Rotate(radians.x, radians.y, radians.z);
}

Math::Matrix<3, 3, f32> Math::LinearTransform3D::Rotate(Vector<3, f32> const& axisInput, f32 amountRadians)
{
	Vector<3, f32> const axis = axisInput.GetNormalized();
	f32 const cos = Cos(amountRadians);
	f32 const sin = Sin(amountRadians);
	return Matrix<3, 3, f32>
	{ { {
		cos + Sqrd(axis.x) * (1 - cos), axis.x* axis.y* (1 - cos) - axis.z * sin, axis.x* axis.z* (1 - cos) + axis.y * sin,
		axis.y* axis.x* (1 - cos) + axis.z * sin, cos + Sqrd(axis.y) * (1 - cos), axis.y* axis.z* (1 - cos) - axis.x * sin,
		axis.z* axis.x* (1 - cos) - axis.y * sin, axis.z* axis.y* (1 - cos) + axis.x * sin, cos + Sqrd(axis.z) * (1 - cos)
	} } }.Transposed();
}

Math::Matrix<4, 4, f32> Math::LinearTransform3D::LookAt_LH(
	Vector<3, f32> const& position,
	Vector<3, f32> const& forward,
	Vector<3, f32> const& upVector)
{
	auto const zAxis = (forward - position).GetNormalized();
	auto const xAxis = Cross(upVector, zAxis).GetNormalized();
	auto const yAxis = Cross(zAxis, xAxis);
	return Mat4
	{ { {
		xAxis.x, yAxis.x, zAxis.x, 0,
		xAxis.y, yAxis.y, zAxis.y, 0,
		xAxis.z, yAxis.z, zAxis.z, 0,
		-Vector<3, f32>::Dot(xAxis, position), -Vector<3, f32>::Dot(yAxis, position), -Vector<3, f32>::Dot(zAxis, position), 1
	} } };
}

Math::Matrix<4, 4, f32> Math::LinearTransform3D::LookAt_RH(
	Vector<3, f32> const& position,
	Vector<3, f32> const& forward,
	Vector<3, f32> const& upVector)
{
	auto const zAxis = (position - forward).GetNormalized();
	auto const xAxis = Cross(upVector, zAxis).GetNormalized();
	auto const yAxis = Cross(zAxis, xAxis);
	return Mat4
	{ { {
		xAxis.x, yAxis.x, zAxis.x, 0,
		xAxis.y, yAxis.y, zAxis.y, 0,
		xAxis.z, yAxis.z, zAxis.z, 0,
		-Dot(xAxis, position), -Dot(yAxis, position), -Dot(zAxis, position), 1
	} } };
}

Math::Matrix<4, 4, f32> Math::LinearTransform3D::Perspective_RH_ZO(
	f32 fovYRadians,
	f32 aspectRatio,
	f32 zNear,
	f32 zFar)
{
	f32 const tanHalfFovy = Tan(fovYRadians / 2);
	return Matrix<4, 4, f32>
	{ { {
		1 / (aspectRatio * tanHalfFovy), 0, 0, 0,
		0, 1 / tanHalfFovy, 0, 0,
		0, 0, zFar / (zNear - zFar), -1,
		0, 0, -(zFar * zNear) / (zFar - zNear), 0
	} } };
}

Math::Matrix<4, 4, f32> Math::LinearTransform3D::Perspective_RH_NO(
	f32 fovYRadians,
	f32 aspectRatio,
	f32 zNear,
	f32 zFar)
{
	f32 const tanHalfFovy = Tan(fovYRadians / 2);
	return Matrix<4, 4, f32>
	{ { {
		1 / (aspectRatio * tanHalfFovy), 0, 0, 0,
		0, 1 / tanHalfFovy, 0, 0,
		0, 0, -(zFar + zNear) / (zFar - zNear), -1,
		0, 0, -(2 * zFar * zNear) / (zFar - zNear), 0
	} } };
}

Math::Matrix<4, 4, f32> Math::LinearTransform3D::Perspective(
	API3D api,
	f32 fovYRadians,
	f32 aspectRatio,
	f32 zNear,
	f32 zFar)
{
	switch (api)
	{
	case API3D::OpenGL:
		return Perspective_RH_NO(fovYRadians, aspectRatio, zNear, zFar);
	case API3D::Vulkan:
		return Perspective_RH_ZO(fovYRadians, aspectRatio, zNear, zFar);
	default:
		return {};
	}
}