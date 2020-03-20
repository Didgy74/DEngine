#pragma once

namespace DEngine::Math
{
	enum class AngleUnit : unsigned char
	{
		Radians,
		Degrees
	};

	namespace Setup
	{
		constexpr AngleUnit defaultAngleUnit = AngleUnit::Degrees;
	}
}