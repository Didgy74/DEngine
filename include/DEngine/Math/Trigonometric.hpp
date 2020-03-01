#pragma once

#include "Setup.hpp"
#include "Constant.hpp"

#include <cmath>

namespace Math
{	
	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] auto Sin(float input)
	{
		if constexpr (angleUnit == AngleUnit::Radians)
			return std::sin(input);
		else if constexpr (angleUnit == AngleUnit::Degrees)
			return std::sin(input * degToRad);
	}

	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] auto Cos(float input)
	{
		if constexpr (angleUnit == AngleUnit::Radians)
			return std::cos(input);
		else if constexpr (angleUnit == AngleUnit::Degrees)
			return std::cos(input * degToRad);
	}

	template<AngleUnit angleUnit = Setup::defaultAngleUnit>
	[[nodiscard]] auto Tan(float input)
	{
		if constexpr (angleUnit == AngleUnit::Radians)
			return std::tan(input);
		else if constexpr (angleUnit == AngleUnit::Degrees)
			return std::tan(input * degToRad);
	}
} 
