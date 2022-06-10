#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Gui/impl/Assert.hpp>

#include <DEngine/impl/Assert.hpp>

#include <DEngine/Math/Vector.hpp>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/Span.hpp>

namespace DEngine::Gui
{
	struct Extent
	{
		u32 width;
		u32 height;

		[[nodiscard]] constexpr bool IsNothing() const noexcept;
		[[nodiscard]] constexpr f32 Aspect() const noexcept;
		[[nodiscard]] constexpr bool FitsInside(Extent const& other) const noexcept;

		// Gets the minimum of both directions
		[[nodiscard]] constexpr static Extent Min(Extent const& a, Extent const& b) noexcept;

		[[nodiscard]] constexpr u32& operator[](uSize index) noexcept
		{
			switch (index)
			{
				case 0: return width;
				case 1: return height;
				default:
					DENGINE_IMPL_UNREACHABLE();
					return width;
			}
		}
		[[nodiscard]] constexpr u32 operator[](uSize index) const noexcept
		{
			switch (index)
			{
				case 0: return width;
				case 1: return height;
				default:
					DENGINE_IMPL_UNREACHABLE();
					return width;
			}
		}

		[[nodiscard]] constexpr bool operator==(Extent const&) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Extent const&) const noexcept;
	};

	// Represents a rectangle in UI space. Includes position and extent.
	// Axis-aligned.
	struct Rect
	{
		Math::Vec2Int position;
		Extent extent;

		// Returns the Y coordinate of the top side.
		[[nodiscard]] constexpr i32 Top() const noexcept;
		// Returns the Y coordinate of the bottom side.
		[[nodiscard]] constexpr i32 Bottom() const noexcept;
		// Returns the X coordinate of the left side.
		[[nodiscard]] constexpr i32 Left() const noexcept;
		// Returns the Y coordinate of the right side.
		[[nodiscard]] constexpr i32 Right() const noexcept;

		[[nodiscard]] constexpr bool IsNothing() const noexcept;
		[[nodiscard]] constexpr bool FitsInside(Rect const& other) const noexcept;

		[[nodiscard]] constexpr bool PointIsInside(Math::Vec2Int point) const noexcept;
		[[nodiscard]] constexpr bool PointIsInside(Math::Vec2 point) const noexcept;

		[[nodiscard]] constexpr static Rect Intersection(Rect const&, Rect const&) noexcept;

		[[nodiscard]] constexpr bool operator==(Rect const&) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Rect const&) const noexcept;
	};

	[[nodiscard]] constexpr Rect Intersection(Rect const& a, Rect const& b) noexcept {
		return Rect::Intersection(a, b);
	}

	[[nodiscard]] inline Std::Opt<uSize> PointIsInside(Std::Span<Rect const> rects, Math::Vec2 point);
}

constexpr bool DEngine::Gui::Extent::IsNothing() const noexcept { return width == 0 || height == 0; }
constexpr DEngine::f32 DEngine::Gui::Extent::Aspect() const noexcept { return (f32)width / (f32)height; }
constexpr bool DEngine::Gui::Extent::FitsInside(Extent const& a) const noexcept
{
	return width <= a.width && height <= a.height;
}

constexpr auto DEngine::Gui::Extent::Min(Extent const& a, Extent const& b) noexcept -> Extent
{
	Extent returnValue = {};
	returnValue.width = a.width < b.width ? a.width : b.width;
	returnValue.height = a.height < b.height ? a.height : b.height;
	return returnValue;
}

constexpr bool DEngine::Gui::Extent::operator==(Extent const& other) const noexcept
{
	return width == other.width && height == other.height;
}

constexpr bool DEngine::Gui::Extent::operator!=(Extent const& other) const noexcept
{
	return !(*this == other);
}

constexpr DEngine::i32 DEngine::Gui::Rect::Top() const noexcept { return (i32)position.y; }
constexpr DEngine::i32 DEngine::Gui::Rect::Bottom() const noexcept { return (i32)position.y + (i32)extent.height; }
constexpr DEngine::i32 DEngine::Gui::Rect::Left() const noexcept { return (i32)position.x; }
constexpr DEngine::i32 DEngine::Gui::Rect::Right() const noexcept { return (i32)position.x + (i32)extent.width; }

constexpr bool DEngine::Gui::Rect::IsNothing() const noexcept { return extent.IsNothing(); }
constexpr bool DEngine::Gui::Rect::FitsInside(Rect const& other) const noexcept
{
	return
		Left() >= other.Left() &&
		Top() >= other.Top() &&
		Right() <= other.Right() &&
		Bottom() <= other.Bottom();
}

constexpr bool DEngine::Gui::Rect::PointIsInside(Math::Vec2Int point) const noexcept
{
	return
		point.x >= Left() && point.x < Right() &&
		point.y >= Top() && point.y < Bottom();
}

constexpr bool DEngine::Gui::Rect::PointIsInside(Math::Vec2 point) const noexcept
{
	return
		point.x >= (f32)Left() && point.x < (f32)Right() &&
		point.y >= (f32)Top() && point.y < (f32)Bottom();
}

constexpr DEngine::Gui::Rect DEngine::Gui::Rect::Intersection(Rect const& a, Rect const& b) noexcept
{
	constexpr auto max = [](i32 a, i32 b) { return a > b ? a : b; };
	constexpr auto min = [](i32 a, i32 b) { return a < b ? a : b; };

	Rect returnVal = {};
	// For loop to handle X and Y directions
	for (uSize i = 0; i < 2; i += 1)
	{
		i32 minPoint = max(a.position[i], b.position[i]);
		returnVal.position[i] = minPoint;
		i32 maxPoint = min(a.position[i] + (i32)a.extent[i], b.position[i] + (i32)b.extent[i]);
		returnVal.extent[i] = (u32)max(0, maxPoint - returnVal.position[i]);
	}
	return returnVal;
}

constexpr bool DEngine::Gui::Rect::operator==(Rect const& other) const noexcept
{
	return position == other.position && extent == other.extent;
}

constexpr bool DEngine::Gui::Rect::operator!=(Rect const& other) const noexcept
{
	return !(*this == other);
}

inline auto DEngine::Gui::PointIsInside(Std::Span<Rect const> rects, Math::Vec2 point) -> Std::Opt<uSize> {
	for (uSize i = 0; i < rects.Size(); i += 1) {
		if (rects[i].PointIsInside(point))
			return i;
	}
	return Std::nullOpt;
}