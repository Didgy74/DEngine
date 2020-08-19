#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Containers/IteratorPair.hpp>

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

	template<typename Iterator, typename BoolCallable>
	bool AllOf(
		Iterator begin,
		Iterator end,
		BoolCallable const& callable);
	template<typename Iterator, typename BoolCallable>
	bool AnyOf(
		Iterator begin,
		Iterator end,
		BoolCallable const& callable);

	template<typename Iterator, typename BoolCallable>
	constexpr Iterator FindIf(
		Iterator begin,
		Iterator end,
		BoolCallable const& callable);
	template<typename Iterator, typename BoolCallable>
	constexpr Iterator FindIf(
		Range<Iterator> range,
		BoolCallable const& callable);
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

template<typename Iterator, typename BoolCallable>
bool DEngine::Std::AllOf(
	Iterator begin,
	Iterator end,
	BoolCallable const& callable)
{
	for (;begin != end; begin++)
	{
		if (!callable(*begin))
			return false;
	}
	return true;
}

template<typename Iterator, typename BoolCallable>
bool DEngine::Std::AnyOf(
	Iterator begin,
	Iterator end,
	BoolCallable const& callable)
{
	for (; begin != end; begin++)
	{
		if (callable(*begin))
			return true;
	}
	return false;
}

template<typename Iterator, typename BoolFunc>
constexpr Iterator DEngine::Std::FindIf(
	Iterator begin,
	Iterator end,
	BoolFunc const& callable)
{
	for (; begin != end; begin++)
	{
		if (callable(*begin))
			return begin;
	}
	return end;
}

template<typename Iterator, typename BoolCallable>
constexpr Iterator DEngine::Std::FindIf(
	Range<Iterator> range,
	BoolCallable const& callable)
{
	return FindIf(range.begin, range.end, callable);
}