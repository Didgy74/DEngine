#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Math/Setup.hpp"
#include "DEngine/Math/Constant.hpp"

#include <cmath>

namespace DEngine::Math
{	
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] f32 Sin(f32 input)
	{
		if constexpr (angleUnit == AngleUnit::Radians)
			return std::sin(input);
		else if constexpr (angleUnit == AngleUnit::Degrees)
			return std::sin(input * degToRad);
	}

	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] f32 Cos(f32 input)
	{
		if constexpr (angleUnit == AngleUnit::Radians)
			return std::cos(input);
		else if constexpr (angleUnit == AngleUnit::Degrees)
			return std::cos(input * degToRad);
	}

	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] f32 Tan(f32 input)
	{
		if constexpr (angleUnit == AngleUnit::Radians)
			return std::tan(input);
		else if constexpr (angleUnit == AngleUnit::Degrees)
			return std::tan(input * degToRad);
	}
} 
