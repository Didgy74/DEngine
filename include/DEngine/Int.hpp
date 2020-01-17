#pragma once

namespace DEngine
{
	// Integers
	using i8 = signed char;
	using i16 = signed short;
	using i32 = signed int;
	using i64 = signed long long;

	using u8 = unsigned char;
	using u16 = unsigned short;
	using u32 = unsigned int;
	using u64 = unsigned long long;

	using iSize = i64;
	using uSize = u64;

	using f32 = float;
	using f64 = double;
}

static_assert(sizeof(DEngine::i8) == 1, "Error. Size of i8 is not 1");
static_assert(sizeof(DEngine::i16) == 2, "Error. Size of i16 is not 2");
static_assert(sizeof(DEngine::i32) == 4, "Error. Size of i32 is not 4");
static_assert(sizeof(DEngine::i64) == 8, "Error. Size of i64 is not 8");

static_assert(sizeof(DEngine::u8) == 1, "Error. Size of u8 is not 1");
static_assert(sizeof(DEngine::u16) == 2, "Error. Size of u16 is not 2");
static_assert(sizeof(DEngine::u32) == 4, "Error. Size of u32 is not 4");
static_assert(sizeof(DEngine::u64) == 8, "Error. Size of u64 is not 8");

static_assert(sizeof(DEngine::iSize) == sizeof(void*), "Error. Size of iSize is not pointer-sized.");
static_assert(sizeof(DEngine::uSize) == sizeof(void*), "Error. Size of uSize is not pointer-sized.");

static_assert(sizeof(DEngine::f32) == 4, "Error. Size of f32 is not 4.");
static_assert(sizeof(DEngine::f64) == 8, "Error. Size of f64 is not 8");