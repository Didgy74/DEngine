#pragma once

#include "DEngine/FixedWidthTypes.hpp"

#include <cmath>
#include <algorithm>

#include <type_traits>

#if defined( _MSC_VER )
#	include <intrin.h>
#endif

namespace DEngine::Math
{
	template<typename T>
	[[nodiscard]] auto Abs(T input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Abs must be of numeric type.");
		return std::abs(input);
	}
	
	template<typename T>
	[[nodiscard]] auto Ceil(T input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Ceil must be of numeric type.");
		return std::ceil(input);
	}

	template<typename T>
	[[nodiscard]] constexpr auto CeilToNearestMultiple(T value, T multiple)
	{
		static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>, "Input of Math::CeilToNearestMultiple must be of unsigned integral type.");
		if (value < multiple)
			return multiple;
		else
		{
			const auto delta = value % multiple;
			if (delta == T(0))
				return value; 
			else
				return value - delta + multiple;
		}
	}

	[[nodiscard]] inline uint16_t CeilToNearestPowerOf2(u16 in)
	{
		constexpr uint16_t bitSize = uint16_t(sizeof(uint16_t) * 8);
#ifdef _MSC_VER
		return uint16_t(1) << (bitSize - __lzcnt16(in));
#else
#error not supported
#endif
	}

	[[nodiscard]] inline uint32_t CeilToNearestPowerOf2(uint32_t in)
	{
		constexpr uint32_t bitSize = uint32_t(sizeof(uint32_t) * 8);
#ifdef _MSC_VER
		return uint32_t(1) << (bitSize - __lzcnt(in));
#endif
	}

	[[nodiscard]] inline uint64_t CeilToNearestPowerOf2(uint64_t in)
	{
		constexpr uint64_t bitSize = uint64_t(sizeof(uint64_t) * 8);
#ifdef _MSC_VER
		return uint64_t(1) << (bitSize - __lzcnt64(in));
#endif
	}

	template<typename T>
	[[nodiscard]] constexpr auto Clamp(T value, T min, T max) -> T
	{
		static_assert(std::is_arithmetic_v<T>, "Input of " __FUNCTION__ " requires type T to be an arithmetic type.");
		return std::clamp(value, min, max);
	}

	template<typename T>
	[[nodiscard]] auto Floor(T input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of " __FUNCTION__ " must be of numeric type.");
		return std::floor(input);
	}

	[[nodiscard]] inline float Hypot(float x, float y)
	{
		return hypotf(x, y);
	}

	[[nodiscard]] inline double Hypot(float x, float y, float z)
	{
		return std::hypot(x, y, z);
	}

	[[nodiscard]] inline double Hypot(double x, double y)
	{
		return hypotl(x, y);
	}

	[[nodiscard]] inline double Hypot(double x, double y, double z)
	{
		return std::hypot(x, y, z);
	}

	template<typename T1, typename T2, typename T3>
	[[nodiscard]] constexpr auto Lerp(T1 input1, T2 input2, T3 delta)
	{
		static_assert(std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2> && std::is_arithmetic_v<T3>, "Input of Math::Lerp must be of numeric type.");
		using ReturnType = std::common_type_t<T1, T2, T3>;
		if constexpr (std::is_same_v<ReturnType, decltype((input2 - input1) * delta + input1)>)
			return (input2 - input1) * delta + input1;
		else
			return static_cast<std::make_signed_t<ReturnType>>((input2 - input1) * delta + input1);
	}

	template<typename T>
	[[nodiscard]] auto Log(T in)
	{
		return std::log(in);
	}

	template<typename T>
	[[nodiscard]] constexpr T Min(T a, T b)
	{
		static_assert(std::is_arithmetic<T>::value , "Error. Argument of " __FUNCTION__ " must be arithmetic types.");
		return std::min(a, b);
	}

	template<typename T>
	[[nodiscard]] constexpr auto Max(T a, T b)
	{
		static_assert(std::is_arithmetic<T>::value, "Error. Argument of " __FUNCTION__ " must be arithmetic types.");
		return std::max(a, b);
	}

	template<typename T1, typename T2>
	[[nodiscard]] auto Pow(T1 coefficient, T2 exponent)
	{
		static_assert(std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>, "Input of Math::Pow must be of numeric type.");
		return std::pow(coefficient, exponent);
	}

	template<typename T>
	[[nodiscard]] auto Round(T input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Round must be of numeric type.");
		return std::round(input);
	}

	template<typename T>
	[[nodiscard]] constexpr auto Sqrd(T input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Sqrd must be of numeric type.");
		return input * input;
	}

	[[nodiscard]] inline float Sqrt(float input)
	{
		return sqrtf(input);
	}

	[[nodiscard]] inline double Sqrt(double input)
	{
		return sqrtl(input);
	}

	template<typename T>
	[[nodiscard]] auto Truncate(T input)
	{
		static_assert(std::is_arithmetic_v<T>, "Input of Math::Truncate must be of numeric type.");
		return std::trunc(input);
	}
}