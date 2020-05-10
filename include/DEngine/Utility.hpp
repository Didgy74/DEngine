#pragma once

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Std
{
	template<typename T = f32>
	T Rand() = delete;
	// Returns a value 0-1
	template<>
	f32 Rand<f32>();

	template<typename T = f32>
	T RandRange(T a, T b) = delete;
	template<>
	u64 RandRange<u64>(u64 a, u64 b);
	template<>
	f32 RandRange<f32>(f32 a, f32 b);

	template<typename T>
	T&& Move(T&& in) noexcept;
	template<typename T>
	T&& Move(T& in) noexcept;
}

template<typename T>
T&& DEngine::Std::Move(T&& in) noexcept
{
	return static_cast<T&&>(in);
}

template<typename T>
T&& DEngine::Std::Move(T& in) noexcept
{
	return static_cast<T&&>(in);
}