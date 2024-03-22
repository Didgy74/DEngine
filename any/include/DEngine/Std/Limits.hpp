#pragma once

#include <cfloat>
#include <climits>

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

	template<>
	struct Limits<long long>
	{
		static constexpr long long highest = LLONG_MAX;
		static constexpr long long lowest = LLONG_MIN;
	};
}