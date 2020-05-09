#pragma once

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Math
{
	[[nodiscard]] constexpr i8 Abs(i8 input);
	[[nodiscard]] constexpr i16 Abs(i16 input);
	[[nodiscard]] constexpr i32 Abs(i32 input);
	[[nodiscard]] constexpr i64 Abs(i64 input);
	[[nodiscard]] constexpr f32 Abs(f32 input);
	[[nodiscard]] constexpr f64 Abs(f64 input);
	
	[[nodiscard]] constexpr f32 Ceil(f32 input);
	[[nodiscard]] constexpr f64 Ceil(f64 input);

	[[nodiscard]] constexpr u64 CeilToMultiple(u64 value, u64 multiple)
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

	[[nodiscard]] constexpr u8 Clamp(u8 value, u8 min, u8 max)
	{
		if (value > max)
			return max;
		else if (value < min)
			return min;
		else
			return value;
	}

	[[nodiscard]] f32 Floor(f32 input);

	[[nodiscard]] constexpr f32 Lerp(f32 a, f32 b, f32 delta);
	[[nodiscard]] constexpr f64 Lerp(f64 a, f64 b, f64 delta);

	[[nodiscard]] f32 Log(f32 in);

	[[nodiscard]] constexpr i8 Min(i8 a, i8 b);
	[[nodiscard]] constexpr i16 Min(i16 a, i16 b);
	[[nodiscard]] constexpr i32 Min(i32 a, i32 b);
	[[nodiscard]] constexpr i64 Min(i64 a, i64 b);
	[[nodiscard]] constexpr u8 Min(u8 a, u8 b);
	[[nodiscard]] constexpr u16 Min(u16 a, u16 b);
	[[nodiscard]] constexpr u32 Min(u32 a, u32 b);
	[[nodiscard]] constexpr u64 Min(u64 a, u64 b);
	[[nodiscard]] constexpr f32 Min(f32 a, f32 b);
	[[nodiscard]] constexpr f64 Min(f64 a, f64 b);

	[[nodiscard]] constexpr i8 Max(i8 a, i8 b);
	[[nodiscard]] constexpr i16 Max(i16 a, i16 b);
	[[nodiscard]] constexpr i32 Max(i32 a, i32 b);
	[[nodiscard]] constexpr i64 Max(i64 a, i64 b);
	[[nodiscard]] constexpr u8 Max(u8 a, u8 b);
	[[nodiscard]] constexpr u16 Max(u16 a, u16 b);
	[[nodiscard]] constexpr u32 Max(u32 a, u32 b);
	[[nodiscard]] constexpr u64 Max(u64 a, u64 b);
	[[nodiscard]] constexpr f32 Max(f32 a, f32 b);
	[[nodiscard]] constexpr f64 Max(f64 a, f64 b);

	[[nodiscard]] f32 Pow(f32 coefficient, f32 exponent);
	[[nodiscard]] f64 Pow(f64 coefficient, f64 exponent);

	[[nodiscard]] f32 Round(f32 input);
	[[nodiscard]] f64 Round(f64 input);

	[[nodiscard]] constexpr i32 Sqrd(i32 input);
	[[nodiscard]] constexpr i64 Sqrd(i64 input);
	[[nodiscard]] constexpr u32 Sqrd(u32 input);
	[[nodiscard]] constexpr u64 Sqrd(u64 input);
	[[nodiscard]] constexpr f32 Sqrd(f32 input);
	[[nodiscard]] constexpr f64 Sqrd(f64 input);

	[[nodiscard]] f32 Sqrt(f32 input);
	[[nodiscard]] f64 Sqrt(f64 input);

	[[nodiscard]] f32 Truncate(f32 input);
}

constexpr DEngine::i32 DEngine::Math::Abs(i32 input) { return input < 0 ? -input : input; }
constexpr DEngine::i64 DEngine::Math::Abs(i64 input) { return input < 0 ? -input : input; }
constexpr DEngine::f32 DEngine::Math::Abs(f32 input) { return input < 0.f ? -input : input; }
constexpr DEngine::f64 DEngine::Math::Abs(f64 input) { return input < 0.0 ? -input : input; }

constexpr DEngine::f32 DEngine::Math::Lerp(f32 a, f32 b, f32 delta) { return (b - a) * delta + a; }
constexpr DEngine::f64 DEngine::Math::Lerp(f64 a, f64 b, f64 delta) { return (b - a) * delta + a; }

constexpr DEngine::i8 DEngine::Math::Min(i8 a, i8 b) { return a < b ? a : b; }
constexpr DEngine::i16 DEngine::Math::Min(i16 a, i16 b) { return a < b ? a : b; }
constexpr DEngine::i32 DEngine::Math::Min(i32 a, i32 b) { return a < b ? a : b; }
constexpr DEngine::i64 DEngine::Math::Min(i64 a, i64 b) { return a < b ? a : b; }
constexpr DEngine::u8 DEngine::Math::Min(u8 a, u8 b) { return a < b ? a : b; }
constexpr DEngine::u16 DEngine::Math::Min(u16 a, u16 b) { return a < b ? a : b; }
constexpr DEngine::u32 DEngine::Math::Min(u32 a, u32 b) { return a < b ? a : b; }
constexpr DEngine::u64 DEngine::Math::Min(u64 a, u64 b) { return a < b ? a : b; }
constexpr DEngine::f32 DEngine::Math::Min(f32 a, f32 b) { return a < b ? a : b; }
constexpr DEngine::f64 DEngine::Math::Min(f64 a, f64 b) { return a < b ? a : b; }

constexpr DEngine::i8 DEngine::Math::Max(i8 a, i8 b) { return a > b ? a : b; }
constexpr DEngine::i16 DEngine::Math::Max(i16 a, i16 b) { return a > b ? a : b; }
constexpr DEngine::i32 DEngine::Math::Max(i32 a, i32 b) { return a > b ? a : b; }
constexpr DEngine::i64 DEngine::Math::Max(i64 a, i64 b) { return a > b ? a : b; }
constexpr DEngine::u8 DEngine::Math::Max(u8 a, u8 b) { return a > b ? a : b; }
constexpr DEngine::u16 DEngine::Math::Max(u16 a, u16 b) { return a > b ? a : b; }
constexpr DEngine::u32 DEngine::Math::Max(u32 a, u32 b) { return a > b ? a : b; }
constexpr DEngine::u64 DEngine::Math::Max(u64 a, u64 b) { return a > b ? a : b; }
constexpr DEngine::f32 DEngine::Math::Max(f32 a, f32 b) { return a > b ? a : b; }
constexpr DEngine::f64 DEngine::Math::Max(f64 a, f64 b) { return a > b ? a : b; }

constexpr DEngine::i32 DEngine::Math::Sqrd(i32 input) { return input * input; }
constexpr DEngine::i64 DEngine::Math::Sqrd(i64 input) { return input * input; }
constexpr DEngine::u32 DEngine::Math::Sqrd(u32 input) { return input * input; }
constexpr DEngine::u64 DEngine::Math::Sqrd(u64 input) { return input * input; }
constexpr DEngine::f32 DEngine::Math::Sqrd(f32 input) { return input * input; }
constexpr DEngine::f64 DEngine::Math::Sqrd(f64 input) { return input * input; }