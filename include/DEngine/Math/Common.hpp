#pragma once

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Math
{
	[[nodiscard]] inline f32 Abs(f32 input)
	{
		return std::abs(input);
	}
	
	[[nodiscard]] inline f32 Ceil(f32 input)
	{
		return std::ceil(input);
	}

	[[nodiscard]] constexpr uSize CeilToNearestMultiple(uSize value, uSize multiple)
	{
		if (value < multiple)
			return multiple;
		else
		{
			const auto delta = value % multiple;
			if (delta == 0)
				return value; 
			else
				return value - delta + multiple;
		}
	}

	[[nodiscard]] inline u16 CeilToNearestPowerOf2(u16 in)
	{
		constexpr u16 bitSize = u16(sizeof(u16) * 8);
		//return u16(1) << (bitSize - __lzcnt16(in));
	}

	[[nodiscard]] inline u32 CeilToNearestPowerOf2(u32 in)
	{
		constexpr u32 bitSize = u32(sizeof(u32) * 8);
	}

	[[nodiscard]] inline u64 CeilToNearestPowerOf2(u64 in)
	{
		constexpr u64 bitSize = u64(sizeof(u64) * 8);
	}

	[[nodiscard]] constexpr u8 Clamp(u8 value, u8 min, u8 max)
	{
		if (value > max)
			return max;
		else if (value < min)
			return min;
		else
			return value;
	}

	[[nodiscard]] inline f32 Floor(f32 input)
	{
		return std::floor(input);
	}

	[[nodiscard]] constexpr f32 Lerp(f32 a, f32 b, f32 delta)
	{
		return (b - a) * delta + a;
	}

	[[nodiscard]] inline f32 Log(f32 in)
	{
		return std::log(in);
	}

	[[nodiscard]] constexpr i8 Min(i8 a, i8 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr i16 Min(i16 a, i16 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr i32 Min(i32 a, i32 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr i64 Min(i64 a, i64 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr u8 Min(u8 a, u8 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr u16 Min(u16 a, u16 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr u32 Min(u32 a, u32 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr u64 Min(u64 a, u64 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr f32 Min(f32 a, f32 b) { return a < b ? a : b; }
	[[nodiscard]] constexpr f64 Min(f64 a, f64 b) { return a < b ? a : b; }

	[[nodiscard]] constexpr i8 Max(i8 a, i8 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr i16 Max(i16 a, i16 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr i32 Max(i32 a, i32 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr i64 Max(i64 a, i64 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr u8 Max(u8 a, u8 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr u16 Max(u16 a, u16 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr u32 Max(u32 a, u32 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr u64 Max(u64 a, u64 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr f32 Max(f32 a, f32 b) { return a > b ? a : b; }
	[[nodiscard]] constexpr f64 Max(f64 a, f64 b) { return a > b ? a : b; }

	[[nodiscard]] inline f32 Pow(f32 coefficient, f32 exponent)
	{
		return std::pow(coefficient, exponent);
	}

	[[nodiscard]] inline f32 Round(f32 input)
	{
		return std::round(input);
	}

	[[nodiscard]] constexpr f32 Sqrd(f32 input)
	{
		return input * input;
	}

	[[nodiscard]] constexpr f64 Sqrd(f64 input)
	{
		return input * input;
	}

	[[nodiscard]] inline f32 Sqrt(f32 input)
	{
		return std::sqrt(input);
	}

	[[nodiscard]] inline f64 Sqrt(f64 input)
	{
		return std::sqrt(input);
	}

	[[nodiscard]] inline f32 Truncate(f32 input)
	{
		return std::trunc(input);
	}
}