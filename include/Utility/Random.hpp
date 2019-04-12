#pragma once

#include <random>
#include <type_traits>

namespace Utility
{
	namespace Random
	{
		template<typename T>
		T Range(T lower, T upper);
	}
}

template<typename T>
T Utility::Random::Range(T lower, T upper)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	if constexpr (std::is_integral_v<T>)
	{
		std::uniform_int_distribution<T> dis(lower, upper);
		return dis(rd);
	}
	else if constexpr (std::is_floating_point_v<T>)
	{
		std::uniform_real_distribution<T> dis(lower, upper);
		return dis(rd);
	}
}