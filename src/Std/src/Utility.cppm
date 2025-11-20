module;

#include <DEngine/Std/Defines.hpp>

// Must be included in the global module fragment. Declaring `operator new`
// inside the named module's purview causes clang to attach `std::align_val_t`
// to this module, which then collides with libc++'s declaration wherever
// another TU pulls in <new> (transitively via <vector>, etc.).
#if DENGINE_STD_COMPILER == DENGINE_STD_COMPILER_CLANG_VALUE
#include <new>
#endif

export module DEngine.Std.Utility;

import DEngine.FixedWidthTypes;
import DEngine.Std.Range;
import DEngine.Std.Span;
import DEngine.Std.Trait;

export namespace DEngine::Std {
	struct PlacementNewTagT {};
	constexpr auto placementNewTag = PlacementNewTagT{};
}
export constexpr void* operator new(decltype(sizeof(int)) size, void* ptr, DEngine::Std::PlacementNewTagT) noexcept
{
	return ptr;
}

export constexpr void operator delete(void* block, void* ptr, DEngine::Std::PlacementNewTagT) noexcept {}

export namespace DEngine::Std {
	enum OS {
		Windows = DENGINE_STD_OS_WINDOWS_VALUE,
		Linux = DENGINE_STD_OS_LINUX_VALUE,
		Android = DENGINE_STD_OS_ANDROID_VALUE,
	};

	enum Compiler {
		MSVC = DENGINE_STD_COMPILER_MSVC_VALUE,
		GCC = DENGINE_STD_COMPILER_GCC_VALUE,
	};

	void NameThisThread(Span<char const> name);

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

	template<class Iterator>
	constexpr bool Contains(
		Iterator begin,
		Iterator const& end,
		decltype(*begin) const& value)
	{
		for (; begin != end; begin++) {
			if (*begin == value) {
				return true;
			}
		}
		return false;
	}
}

template<typename Iterator, typename BoolCallable>
bool DEngine::Std::AllOf(
	Iterator begin,
	Iterator end,
	BoolCallable callable)
{
	for (;begin != end; begin++) {
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
	for (; begin != end; begin++) {
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
	for (; begin != end; begin++) {
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