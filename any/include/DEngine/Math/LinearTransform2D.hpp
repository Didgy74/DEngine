#pragma once

#include <DEngine/FixedWidthTypes.hpp>

#include <DEngine/Math/Matrix.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Math/Trigonometric.hpp>
#include <DEngine/Math/Common.hpp>

namespace DEngine::Math::LinearTransform2D
{
	[[nodiscard]] inline Mat2 Rotate(f32 radians) noexcept;
}

namespace DEngine::Math
{
	namespace LinTran2D = LinearTransform2D;
}

inline DEngine::Math::Mat2 DEngine::Math::LinearTransform2D::Rotate(f32 radians) noexcept
{
	f32 const cos = Cos(radians);
	f32 const sin = Sin(radians);
	return Mat2
	{
		cos, sin,
		-sin, cos
	};
}