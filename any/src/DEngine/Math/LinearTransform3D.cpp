#include <DEngine/Math/LinearTransform3D.hpp>

#include <DEngine/Math/Common.hpp>

#include <DEngine/Math/impl/Assert.hpp>

using namespace DEngine;
using namespace DEngine::Math;

Matrix<4, 3, f32> LinearTransform3D::Multiply_Reduced(
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

Vector<3, f32> LinearTransform3D::Multiply_Reduced(
	Matrix<4, 3, f32> const& left,
	Vector<3, f32> const& right)
{
	Vector<3, f32> newVector{};
	for (uSize y = 0; y < 3; y++)
	{
		f32 dot = 0.f;
		for (uSize i = 0; i < 3; i++)
			dot += left.At(i, y) * right[i];
		newVector[y] = dot + left.At(3, y);
	}
	return newVector;
}

Matrix<4, 4, f32> LinearTransform3D::Translate(Vector<3, f32> const& input)
{
	auto newMat = Matrix<4, 4, f32>::Identity();
	newMat.At(3, 0) = input.x;
	newMat.At(3, 1) = input.y;
	newMat.At(3, 2) = input.z;
	return newMat;
}

void LinearTransform3D::SetTranslation(Matrix<4, 4, f32>& matrix, Vector<3, f32> const& input)
{
	matrix.At(3, 0) = input.x;
	matrix.At(3, 1) = input.y;
	matrix.At(3, 2) = input.z;
}

Matrix<3, 3, f32> LinearTransform3D::Rotate(UnitQuat const& quat)
{
	auto const& s = quat.S();
	auto const& x = quat.X();
	auto const& y = quat.Y();
	auto const& z = quat.Z();
	auto const sqrdX = Sqrd(x);
	auto const sqrdY = Sqrd(y);
	auto const sqrdZ = Sqrd(z);

	
	Matrix<3, 3, f32> newMat = {};

	newMat.At(0, 0) = 1 - 2*sqrdY - 2*sqrdZ;
	newMat.At(0, 1) = 2*x*y + 2*z*s;
	newMat.At(0, 2) = 2*x*z - 2*y*s;

	newMat.At(1, 0) = 2*x*y - 2*z* s;
	newMat.At(1, 1) = 1 - 2*sqrdX - 2*sqrdZ;
	newMat.At(1, 2) = 2*y*z + 2*x*s;

	newMat.At(2, 0) = 2*x*z + 2*y*s;
	newMat.At(2, 1) = 2*y*z - 2*x*s;
	newMat.At(2, 2) = 2*sqrdX - 2*sqrdY;

	return newMat;
}

Matrix<4, 3, f32> LinearTransform3D::Rotate_Reduced(UnitQuat const& quat)
{
	f32 const& s = quat.S();
	f32 const& x = quat.X();
	f32 const& y = quat.Y();
	f32 const& z = quat.Z();
	f32 const sqrdX = Sqrd(x);
	f32 const sqrdY = Sqrd(y);
	f32 const sqrdZ = Sqrd(z);

	Matrix<4, 3> newMat = {};
	newMat.At(0, 0) = 1 - 2*sqrdY - 2*sqrdZ;
	newMat.At(0, 1) = 2*x*y + 2*z*s;
	newMat.At(0, 2) = 2*x*z - 2*y*s;

	newMat.At(1, 0) = 2*x*y - 2*z*s;
	newMat.At(1, 1) = 1 - 2*sqrdX - 2*sqrdZ;
	newMat.At(1, 2) = 2*y*z + 2*x*s;

	newMat.At(2, 0) = 2*x*z + 2*y*s;
	newMat.At(2, 1) = 2*y*z - 2*x*s;
	newMat.At(2, 2) = 1 - 2*sqrdX - 2*sqrdY;

	newMat.At(3, 0) = 0;
	newMat.At(3, 1) = 0;
	newMat.At(3, 2) = 0;

	return newMat;
}

Matrix<3, 3, f32> LinearTransform3D::Rotate(ElementaryAxis axis, f32 amountRadians)
{
	f32 const cos = Cos(amountRadians);
	f32 const sin = Sin(amountRadians);
	auto newMat = Matrix<3, 3, f32>::Identity();
	switch (axis)
	{
		case ElementaryAxis::X:
		{
			newMat.At(1, 1) = cos;
			newMat.At(2, 1) = sin;
			newMat.At(1, 2) = -sin;
			newMat.At(2, 2) = cos;
			break;
		}
		case ElementaryAxis::Y:
		{
			newMat.At(0, 0) = cos;
			newMat.At(2, 0) = -sin;
			newMat.At(0, 2) = sin;
			newMat.At(2, 2) = cos;
			break;
		}
		case ElementaryAxis::Z:
		{
			newMat.At(0, 0) = cos;
			newMat.At(1, 0) = sin;
			newMat.At(0, 1) = -sin;
			newMat.At(1, 1) = cos;
			break;
		}
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
			return {};
	}

	return newMat;
}

Matrix<4, 4, f32> LinearTransform3D::Rotate_Homo(ElementaryAxis axis, f32 amountRadians)
{
	f32 const cos = Cos(amountRadians);
	f32 const sin = Sin(amountRadians);
	auto newMat = Matrix<4, 4, f32>::Identity();
	switch (axis)
	{
		case ElementaryAxis::X:
		{
			newMat.At(1, 1) = cos;
			newMat.At(2, 1) = sin;
			newMat.At(1, 2) = -sin;
			newMat.At(2, 2) = cos;
			break;
		}
		case ElementaryAxis::Y:
		{
			newMat.At(0, 0) = cos;
			newMat.At(2, 0) = -sin;
			newMat.At(0, 2) = sin;
			newMat.At(2, 2) = cos;
			break;
		}
		case ElementaryAxis::Z:
		{
			newMat.At(0, 0) = cos;
			newMat.At(0, 1) = sin;
			newMat.At(1, 0) = -sin;
			newMat.At(1, 1) = cos;
			break;
		}
		default:
			DENGINE_IMPL_MATH_UNREACHABLE();
			return {};
	}

	return newMat;
}

Matrix<3, 3, f32> LinearTransform3D::Rotate(f32 xRadians, f32 yRadians, f32 zRadians)
{
	return Rotate(ElementaryAxis::Z, zRadians) * Rotate(ElementaryAxis::Y, yRadians) * Rotate(ElementaryAxis::X, xRadians);
}

Matrix<3, 3, f32> LinearTransform3D::Rotate(Vector<3, f32> const& radians)
{
	return Rotate(radians.x, radians.y, radians.z);
}

Matrix<3, 3, f32> LinearTransform3D::Rotate(Vector<3, f32> const& axisInput, f32 amountRadians)
{
	auto const axis = axisInput.GetNormalized();
	auto const cos = Cos(amountRadians);
	auto const sin = Sin(amountRadians);
	auto const& x = axis.x;
	auto const& y = axis.y;
	auto const& z = axis.z;

	Matrix<3, 3, f32> newMat = {};

	newMat.At(0, 0) = cos + Sqrd(x) * (1-cos);
	newMat.At(0, 1) = y*x*(1-cos) + z*sin;
	newMat.At(0, 2) = z*x*(1-cos) - y*sin;

	newMat.At(1, 0) = x*y*(1-cos) - z*sin;
	newMat.At(1, 1) = cos + Sqrd(y) * (1-cos);
	newMat.At(1, 2) = z*y*(1-cos) + x*sin;

	newMat.At(2, 0) = x*z*(1-cos) + y*sin;
	newMat.At(2, 1) = y*z*(1-cos) - x*sin;
	newMat.At(2, 2) = cos + Sqrd(z)*(1-cos);

	return newMat;


	return Matrix<3, 3, f32>
		{
			cos + Sqrd(axis.x) * (1 - cos), axis.x*axis.y* (1 - cos) - axis.z * sin, axis.x* axis.z* (1 - cos) + axis.y * sin,
			axis.y* axis.x* (1 - cos) + axis.z * sin, cos + Sqrd(axis.y) * (1 - cos), axis.y* axis.z* (1 - cos) - axis.x * sin,
			axis.z* axis.x* (1 - cos) - axis.y * sin, axis.z* axis.y* (1 - cos) + axis.x * sin, cos + Sqrd(axis.z) * (1 - cos)
		}.Transposed();
}

Matrix<3, 3, f32> LinearTransform3D::Scale(f32 x, f32 y, f32 z)
{
	Matrix<3, 3, f32> newMat = {};
	newMat.At(0, 0) = x;
	newMat.At(1, 1) = y;
	newMat.At(2, 2) = z;
	return newMat;
}

Matrix<3, 3, f32> LinearTransform3D::Scale(Vector<3, f32> const& input)
{
	return Scale(input.x, input.y, input.z);
}

Matrix<4, 4, f32> LinearTransform3D::Scale_Homo(f32 x, f32 y, f32 z)
{
	Matrix<4, 4, f32> newMat = {};
	newMat.At(0, 0) = x;
	newMat.At(1, 1) = y;
	newMat.At(2, 2) = z;
	newMat.At(3, 3) = 1;
	return newMat;
}

Matrix<4, 4, f32> LinearTransform3D::Scale_Homo(Vector<3, f32> const& input)
{
	return Scale_Homo(input.x, input.y, input.z);
}

Matrix<4, 3> LinearTransform3D::Scale_Reduced(f32 x, f32 y, f32 z)
{
	Matrix<4, 3, f32> newMat = {};
	newMat.At(0, 0) = x;
	newMat.At(1, 1) = y;
	newMat.At(2, 2) = z;
	return newMat;
}

Matrix<4, 3, f32> LinearTransform3D::Scale_Reduced(Vector<3, f32> const& input)
{
	return Scale_Reduced(input.x, input.y, input.z);
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
		{
			xAxis.x, yAxis.x, zAxis.x, 0,
			xAxis.y, yAxis.y, zAxis.y, 0,
			xAxis.z, yAxis.z, zAxis.z, 0,
			-Dot(xAxis, position), -Dot(yAxis, position), -Dot(zAxis, position), 1
		};
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
		{
			xAxis.x, yAxis.x, zAxis.x, 0,
			xAxis.y, yAxis.y, zAxis.y, 0,
			xAxis.z, yAxis.z, zAxis.z, 0,
			-Dot(xAxis, position), -Dot(yAxis, position), -Dot(zAxis, position), 1
		};
}

Math::Matrix<4, 4, f32> Math::LinearTransform3D::Perspective_RH_ZO(
	f32 fovYRadians,
	f32 aspectRatio,
	f32 zNear,
	f32 zFar)
{
	f32 const tanHalfFovy = Tan(fovYRadians / 2);
	return Matrix<4, 4, f32>
		{
			1 / (aspectRatio * tanHalfFovy), 0, 0, 0,
			0, 1 / tanHalfFovy, 0, 0,
			0, 0, zFar / (zNear - zFar), -1,
			0, 0, -(zFar * zNear) / (zFar - zNear), 0
		};
}

Math::Matrix<4, 4, f32> Math::LinearTransform3D::Perspective_RH_NO(
	f32 fovYRadians,
	f32 aspectRatio,
	f32 zNear,
	f32 zFar)
{
	f32 const tanHalfFovy = Tan(fovYRadians / 2);
	return Matrix<4, 4, f32>
		{
			1 / (aspectRatio * tanHalfFovy), 0, 0, 0,
			0, 1 / tanHalfFovy, 0, 0,
			0, 0, -(zFar + zNear) / (zFar - zNear), -1,
			0, 0, -(2 * zFar * zNear) / (zFar - zNear), 0
		};
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