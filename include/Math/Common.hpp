#pragma once

#include <cmath>
#include <algorithm>

#include <type_traits>

namespace Math
{
	template<typename T>
	[[nodiscard]] auto Abs(const T& input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Abs must be of numeric type.");
		return std::abs(input);
	}
	
	template<typename T>
	[[nodiscard]] auto Ceil(const T& input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Ceil must be of numeric type.");
		return std::ceil(input);
	}

	template<typename T>
	[[nodiscard]] constexpr T Clamp(const T& value, const T& min, const T& max)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Clamp must be of numeric type.");
		return std::clamp(value, min, max);
	}

	template<typename T>
	[[nodiscard]] auto Floor(const T& input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Floor must be of numeric type.");
		return std::floor(input);
	}

	template<typename T>
	[[nodiscard]] auto Hypot(const T& x, const T& y)
	{
		return std::hypot(x, y);
	}

	template<typename T>
	[[nodiscard]] auto Hypot(const T& x, const T& y, const T& z)
	{
		return std::hypot(x, y, z);
	}

	template<typename T1, typename T2, typename T3>
	[[nodiscard]] constexpr auto Lerp(const T1& input1, const T2& input2, const T3& delta)
	{
		static_assert(std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2> && std::is_arithmetic_v<T3>, "Input of Math::Lerp must be of numeric type.");
		using ReturnType = std::common_type_t<T1, T2, T3>;
		if constexpr (std::is_same_v<ReturnType, decltype((input2 - input1) * delta + input1)>)
			return (input2 - input1) * delta + input1;
		else
			return static_cast<std::make_signed_t<ReturnType>>((input2 - input1) * delta + input1);
	}

	template<typename T1, typename T2>
	[[nodiscard]] auto Pow(const T1& coefficient, const T2& exponent)
	{
		static_assert(std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>, "Input of Math::Pow must be of numeric type.");
		return std::pow(coefficient, exponent);
	}

	template<typename T>
	[[nodiscard]] auto Round(const T& input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Round must be of numeric type.");
		return std::round(input);
	}

	template<typename T>
	[[nodiscard]] constexpr auto Sqrd(const T& input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Sqrd must be of numeric type.");
		return input * input;
	}

	template<typename T>
	[[nodiscard]] auto Sqrt(const T& input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Sqrt must be of numeric type.");
		return std::sqrt(input);
	}

	template<typename T>
	[[nodiscard]] auto Truncate(const T& input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Truncate must be of numeric type.");
		return std::trunc(input);
	}
}