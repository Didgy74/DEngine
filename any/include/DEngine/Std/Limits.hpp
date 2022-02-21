#pragma once

#include <cfloat>

namespace DEngine::Std
{
	template<typename T>
	struct Limits;

	template<>
	struct Limits<float>
	{
		static constexpr float highest = FLT_MAX;
		static constexpr float lowest = -highest;
	};

	template<>
	struct Limits<double>
	{
		static constexpr double highest = DBL_MAX;
		static constexpr double lowest = -highest;
	};
}