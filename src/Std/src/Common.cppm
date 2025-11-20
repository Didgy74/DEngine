module;

#include <DEngine/Std/Containers/impl/Assert.hpp>

export module DEngine.Std.Common;

import DEngine.FixedWidthTypes;
import DEngine.Std.Trait;

export namespace DEngine::Std {
	[[nodiscard]] f32 Ceil(f32 input);
	[[nodiscard]] f64 Ceil(f64 input);

    // Multiple cannot be 0.
    [[nodiscard]] constexpr u8 CeilToMultiple(u8 value, u8 multiple) noexcept;
    [[nodiscard]] constexpr u16 CeilToMultiple(u16 value, u16 multiple) noexcept;
    [[nodiscard]] constexpr u32 CeilToMultiple(u32 value, u32 multiple) noexcept;
    [[nodiscard]] constexpr u64 CeilToMultiple(u64 value, u64 multiple) noexcept;

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
	[[nodiscard]] f64 Floor(f64 input);

    template<typename T>
    [[nodiscard]] constexpr T Max(T a, T b) noexcept { return a > b ? a : b; }

	template<typename T>
	[[nodiscard]] constexpr Trait::RemoveRef<T>&& Move(T&& in) noexcept { return static_cast<Trait::RemoveRef<T>&&>(in); }

	template<typename T>
	[[nodiscard]] constexpr auto Min(T const& a, T const& b) noexcept { return a < b ? a : b; }

	[[nodiscard]] f32 Round(f32 input);
	[[nodiscard]] f64 Round(f64 input);

	template<typename T>
	constexpr void Swap(T& a, T& b) noexcept {
		T temp = Move(a);
		a = Move(b);
		b = Move(temp);
	}
}

namespace DEngine::Std::impl {
    template<typename T>
    [[nodiscard]] constexpr auto CeilToMultiple_Inner(T const& value, T const& multiple) noexcept {
        DENGINE_IMPL_CONTAINERS_ASSERT(multiple != T(0));
        T modulo = value % multiple;
        // Calculate how much we need to add to get to next multiple.
        T remainderToMultiple = T(multiple - modulo);
        // Then take into account if our value is already a multiple.
        remainderToMultiple *= modulo != 0;
        return value + remainderToMultiple;
    }

	template <typename T>
	constexpr T Clamp(T value, T min, T max) {
    	if (value < min) {
    		return min;
    	}
    	if (value > max) {
    		return max;
    	}
    	return value;
    }
}

constexpr DEngine::u8 DEngine::Std::CeilToMultiple(u8 value, u8 multiple) noexcept { return impl::CeilToMultiple_Inner(value, multiple); }
constexpr DEngine::u16 DEngine::Std::CeilToMultiple(u16 value, u16 multiple) noexcept { return impl::CeilToMultiple_Inner(value, multiple); }
constexpr DEngine::u32 DEngine::Std::CeilToMultiple(u32 value, u32 multiple) noexcept { return impl::CeilToMultiple_Inner(value, multiple); }
constexpr DEngine::u64 DEngine::Std::CeilToMultiple(u64 value, u64 multiple) noexcept { return impl::CeilToMultiple_Inner(value, multiple); }

constexpr DEngine::i8 DEngine::Std::Clamp(i8 value, i8 min, i8 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::i16 DEngine::Std::Clamp(i16 value, i16 min, i16 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::i32 DEngine::Std::Clamp(i32 value, i32 min, i32 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::i64 DEngine::Std::Clamp(i64 value, i64 min, i64 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::u8 DEngine::Std::Clamp(u8 value, u8 min, u8 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::u16 DEngine::Std::Clamp(u16 value, u16 min, u16 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::u32 DEngine::Std::Clamp(u32 value, u32 min, u32 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::u64 DEngine::Std::Clamp(u64 value, u64 min, u64 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::f32 DEngine::Std::Clamp(f32 value, f32 min, f32 max) { return impl::Clamp(value, min, max); }
constexpr DEngine::f64 DEngine::Std::Clamp(f64 value, f64 min, f64 max) { return impl::Clamp(value, min, max); }