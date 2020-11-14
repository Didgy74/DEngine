#pragma once

#include <DEngine/Std/Containers/Range.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Trait.hpp>

#include <string_view>

namespace DEngine::Std
{
	void NameThisThread(std::string const& name);
	
	template<typename T>
	[[nodiscard]] constexpr Trait::RemoveRef<T>&& Move(T&& in) noexcept;

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
	constexpr void Swap(T& a, T& b) noexcept;

	template<typename Iterator, typename BoolCallable>
	bool AllOf(
		Iterator begin,
		Iterator end,
		BoolCallable callable);
	template<typename Iterator, typename BoolCallable>
	bool AllOf(
		Range<Iterator> range,
		BoolCallable callable);
	template<typename Iterator, typename BoolCallable>
	bool AnyOf(
		Iterator begin,
		Iterator end,
		BoolCallable callable);
	template<typename Iterator, typename BoolCallable>
	bool AnyOf(
		Range<Iterator> range,
		BoolCallable callable);

	template<typename Iterator, typename BoolCallable>
	constexpr Iterator FindIf(
		Iterator begin,
		Iterator end,
		BoolCallable callable);
	template<typename Iterator, typename BoolCallable>
	constexpr Iterator FindIf(
		Range<Iterator> range,
		BoolCallable callable);
}

template<typename T>
constexpr DEngine::Std::Trait::RemoveRef<T>&& DEngine::Std::Move(T&& in) noexcept
{
	return static_cast<Trait::RemoveRef<T>&&>(in);
}

template<typename T>
constexpr void DEngine::Std::Swap(T& a, T& b) noexcept
{
	T temp = Move(a);
	a = Move(b);
	b = Move(temp);
}

template<typename Iterator, typename BoolCallable>
bool DEngine::Std::AllOf(
	Iterator begin,
	Iterator end,
	BoolCallable callable)
{
	for (;begin != end; begin++)
	{
		if (!callable(*begin))
			return false;
	}
	return true;
}

template<typename Iterator, typename BoolCallable>
bool DEngine::Std::AllOf(
	Range<Iterator> range,
	BoolCallable callable)
{
	return AllOf(range.begin, range.end, callable);
}

template<typename Iterator, typename BoolCallable>
bool DEngine::Std::AnyOf(
	Iterator begin,
	Iterator end,
	BoolCallable callable)
{
	for (; begin != end; begin++)
	{
		if (callable(*begin))
			return true;
	}
	return false;
}

template<typename Iterator, typename BoolCallable>
bool DEngine::Std::AnyOf(
	Range<Iterator> range,
	BoolCallable callable)
{
	return AnyOf(range.begin, range.end, callable);
}

template<typename Iterator, typename BoolFunc>
constexpr Iterator DEngine::Std::FindIf(
	Iterator begin,
	Iterator end,
	BoolFunc callable)
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
	BoolCallable callable)
{
	return FindIf(range.begin, range.end, callable);
}