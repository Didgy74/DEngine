#pragma once

#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Math
{
	[[nodiscard]] constexpr i8 Abs(i8 input);
	[[nodiscard]] constexpr i16 Abs(i16 input);
	[[nodiscard]] constexpr i32 Abs(i32 input);
	[[nodiscard]] constexpr i64 Abs(i64 input);
	[[nodiscard]] constexpr f32 Abs(f32 input);
	[[nodiscard]] constexpr f64 Abs(f64 input);

	[[nodiscard]] f32 Ceil(f32 input);
	[[nodiscard]] f64 Ceil(f64 input);

	[[nodiscard]] constexpr u64 CeilToPOT(u64 value, u8 factor) noexcept
	{
		auto const minus1 = factor - 1;
		return (value + minus1) & !minus1;
	}

	template<typename T>
	[[nodiscard]] constexpr auto CeilToMultiple(T const& value, T const& multiple) noexcept
	{
		return ((value - T(1)) | (multiple - T(1))) + T(1);
	}
	/*
	[[nodiscard]] constexpr u8 CeilToMultiple(u8 value, u8 multiple);
	[[nodiscard]] constexpr u16 CeilToMultiple(u16 value, u16 multiple);
	[[nodiscard]] constexpr u32 CeilToMultiple(u32 value, u32 multiple);
	[[nodiscard]] constexpr u64 CeilToMultiple(u64 value, u64 multiple);
	 */

	[[nodiscard]] constexpr i8 Clamp(i8 value, i8 min, i8 max);
	[[nodiscard]] constexpr i16 Clamp(i16 value, i16 min, i16 max);
	[[nodiscard]] constexpr i32 Clamp(i32 value, i32 min, i32 max);
	[[nodiscard]] constexpr i64 Clamp(i64 value, i64 min, i64 max);
	[[nodiscard]] constexpr u8 Clamp(u8 value, u8 min, u8 max);
	[[nodiscard]] constexpr u16 Clamp(u16 value, u16 min, u16 max);
	[[nodiscard]] constexpr u32 Clamp(u32 value, u32 min, u32 max);
	[[nodiscard]] constexpr u64 Clamp(u64 value, u64 min, u64 max);
	[[nodiscard]] constexpr f32 Clamp(f32 value, f32 min, f32 max);
	[[nodiscard]] constexpr f64 Clamp(f64 value, f64 min, f64 max);

	[[nodiscard]] f32 Floor(f32 input);

	[[nodiscard]] constexpr f32 Lerp(f32 a, f32 b, f32 delta);
	[[nodiscard]] constexpr f64 Lerp(f64 a, f64 b, f64 delta);

	[[nodiscard]] f32 Log(f32 in);

	template<typename T>
	[[nodiscard]] constexpr auto Min(T const& a, T const& b) noexcept { return a < b ? a : b; }

	template<typename T>
	[[nodiscard]] constexpr auto Max(T const& a, T const& b) noexcept { return a > b ? a : b; }
	/*
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
	 */

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

namespace DEngine::Math::detail
{
	template<typename T>
	constexpr T Abs(T in) { return in < T(0) ? -in : in; }

	template<typename T>
	constexpr T CeilToMultiple_Unsigned(T value, T multiple)
	{
		return value + multiple - T(1) - (value - T(1)) % multiple;
	}

	template <typename T>
	constexpr T Clamp(T value, T min, T max)
	{
		if (value < min)
			return min;
		else if (value > max)
			return max;
		else
			return value;
	}

	template<typename T>
	constexpr T Lerp(T a, T b, T delta) { return (b - a) * delta + a; }

	template<typename T>
	constexpr T Min(T a, T b) { return a < b ? a : b; }

	template<typename T>
	constexpr T Max(T a, T b) { return a > b ? a : b; }

	template<typename T>
	constexpr T Sqrd(T in) { return in * in; }
}

constexpr DEngine::i8 DEngine::Math::Abs(i8 input) { return detail::Abs(input); }
constexpr DEngine::i16 DEngine::Math::Abs(i16 input) { return detail::Abs(input); }
constexpr DEngine::i32 DEngine::Math::Abs(i32 input) { return detail::Abs(input); }
constexpr DEngine::i64 DEngine::Math::Abs(i64 input) { return detail::Abs(input); }
constexpr DEngine::f32 DEngine::Math::Abs(f32 input) { return detail::Abs(input); }
constexpr DEngine::f64 DEngine::Math::Abs(f64 input) { return detail::Abs(input); }

/*
constexpr DEngine::u8 DEngine::Math::CeilToMultiple(u8 value, u8 multiple) { return detail::CeilToMultiple_Unsigned(value, multiple); }
constexpr DEngine::u16 DEngine::Math::CeilToMultiple(u16 value, u16 multiple) { return detail::CeilToMultiple_Unsigned(value, multiple); }
constexpr DEngine::u32 DEngine::Math::CeilToMultiple(u32 value, u32 multiple) { return detail::CeilToMultiple_Unsigned(value, multiple); }
constexpr DEngine::u64 DEngine::Math::CeilToMultiple(u64 value, u64 multiple) { return detail::CeilToMultiple_Unsigned(value, multiple); }
*/

constexpr DEngine::i8 DEngine::Math::Clamp(i8 value, i8 min, i8 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::i16 DEngine::Math::Clamp(i16 value, i16 min, i16 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::i32 DEngine::Math::Clamp(i32 value, i32 min, i32 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::i64 DEngine::Math::Clamp(i64 value, i64 min, i64 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::u8 DEngine::Math::Clamp(u8 value, u8 min, u8 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::u16 DEngine::Math::Clamp(u16 value, u16 min, u16 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::u32 DEngine::Math::Clamp(u32 value, u32 min, u32 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::u64 DEngine::Math::Clamp(u64 value, u64 min, u64 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::f32 DEngine::Math::Clamp(f32 value, f32 min, f32 max) { return detail::Clamp(value, min, max); }
constexpr DEngine::f64 DEngine::Math::Clamp(f64 value, f64 min, f64 max) { return detail::Clamp(value, min, max); }

constexpr DEngine::f32 DEngine::Math::Lerp(f32 a, f32 b, f32 delta) { return detail::Lerp(a, b, delta); }
constexpr DEngine::f64 DEngine::Math::Lerp(f64 a, f64 b, f64 delta) { return detail::Lerp(a, b, delta); }

/*
constexpr DEngine::i8 DEngine::Math::Min(i8 a, i8 b) { return detail::Min(a, b); }
constexpr DEngine::i16 DEngine::Math::Min(i16 a, i16 b) { return detail::Min(a, b); }
constexpr DEngine::i32 DEngine::Math::Min(i32 a, i32 b) { return detail::Min(a, b); }
constexpr DEngine::i64 DEngine::Math::Min(i64 a, i64 b) { return detail::Min(a, b); }
constexpr DEngine::u8 DEngine::Math::Min(u8 a, u8 b) { return detail::Min(a, b); }
constexpr DEngine::u16 DEngine::Math::Min(u16 a, u16 b) { return detail::Min(a, b); }
constexpr DEngine::u32 DEngine::Math::Min(u32 a, u32 b) { return detail::Min(a, b); }
constexpr DEngine::u64 DEngine::Math::Min(u64 a, u64 b) { return detail::Min(a, b); }
constexpr DEngine::f32 DEngine::Math::Min(f32 a, f32 b) { return detail::Min(a, b); }
constexpr DEngine::f64 DEngine::Math::Min(f64 a, f64 b) { return detail::Min(a, b); }
 */
/*
constexpr DEngine::i8 DEngine::Math::Max(i8 a, i8 b) { return detail::Max(a, b); }
constexpr DEngine::i16 DEngine::Math::Max(i16 a, i16 b) { return detail::Max(a, b); }
constexpr DEngine::i32 DEngine::Math::Max(i32 a, i32 b) { return detail::Max(a, b); }
constexpr DEngine::i64 DEngine::Math::Max(i64 a, i64 b) { return detail::Max(a, b); }
constexpr DEngine::u8 DEngine::Math::Max(u8 a, u8 b) { return detail::Max(a, b); }
constexpr DEngine::u16 DEngine::Math::Max(u16 a, u16 b) { return detail::Max(a, b); }
constexpr DEngine::u32 DEngine::Math::Max(u32 a, u32 b) { return detail::Max(a, b); }
constexpr DEngine::u64 DEngine::Math::Max(u64 a, u64 b) { return detail::Max(a, b); }
constexpr DEngine::f32 DEngine::Math::Max(f32 a, f32 b) { return detail::Max(a, b); }
constexpr DEngine::f64 DEngine::Math::Max(f64 a, f64 b) { return detail::Max(a, b); }
*/
constexpr DEngine::i32 DEngine::Math::Sqrd(i32 input) { return detail::Sqrd(input); }
constexpr DEngine::i64 DEngine::Math::Sqrd(i64 input) { return detail::Sqrd(input); }
constexpr DEngine::u32 DEngine::Math::Sqrd(u32 input) { return detail::Sqrd(input); }
constexpr DEngine::u64 DEngine::Math::Sqrd(u64 input) { return detail::Sqrd(input); }
constexpr DEngine::f32 DEngine::Math::Sqrd(f32 input) { return detail::Sqrd(input); }
constexpr DEngine::f64 DEngine::Math::Sqrd(f64 input) { return detail::Sqrd(input); }