#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Gui/impl/Assert.hpp>

#include <DEngine/Math/Common.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine::Gui
{
	struct Extent
	{
		u32 width{};
		u32 height{};

		[[nodiscard]] constexpr u32& operator[](uSize index) noexcept
		{
			DENGINE_IMPL_GUI_ASSERT_MSG(
				index < 2,
				"Attempted to index into an Extent with an index out of bounds.");
			return (&width)[index];
		}
		[[nodiscard]] constexpr u32 operator[](uSize index) const noexcept
		{
			DENGINE_IMPL_GUI_ASSERT_MSG(
				index < 2,
				"Attempted to index into an Extent with an index out of bounds.");
			return (&width)[index];
		}

		[[nodiscard]] constexpr bool operator==(Extent const&) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Extent const&) const noexcept;
	};

	struct Rect
	{
		Math::Vec2Int position{};
		Extent extent{};

		[[nodiscard]] constexpr i32 Top() const noexcept;
		[[nodiscard]] constexpr i32 Bottom() const noexcept;
		[[nodiscard]] constexpr i32 Left() const noexcept;
		[[nodiscard]] constexpr i32 Right() const noexcept;

		[[nodiscard]] constexpr bool IsNothing() const noexcept;

		[[nodiscard]] constexpr bool PointIsInside(Math::Vec2Int point) const noexcept;
		[[nodiscard]] constexpr bool PointIsInside(Math::Vec2 point) const noexcept;

		[[nodiscard]] constexpr static Rect Intersection(Rect const&, Rect const&) noexcept;

		[[nodiscard]] constexpr bool operator==(Rect const&) const noexcept;
		[[nodiscard]] constexpr bool operator!=(Rect const&) const noexcept;
	};
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

constexpr bool DEngine::Gui::Rect::IsNothing() const noexcept { return extent.width == 0 || extent.height == 0; }

constexpr bool DEngine::Gui::Rect::PointIsInside(Math::Vec2Int point) const noexcept
{
	return point.x >= position.x && point.x < position.x + (i32)extent.width &&
		point.y >= position.y && point.y < position.y + (i32)extent.height;
}

constexpr bool DEngine::Gui::Rect::PointIsInside(Math::Vec2 point) const noexcept
{
	return point.x >= (f32)position.x && point.x < (f32)position.x + (f32)extent.width &&
		point.y >= (f32)position.y && point.y < (f32)position.y + (f32)extent.height;
}

constexpr DEngine::Gui::Rect DEngine::Gui::Rect::Intersection(Rect const& a, Rect const& b) noexcept
{
	Rect returnVal{};
	for (uSize i = 0; i < 2; i += 1)
	{
		i32 minPoint = Math::Max(a.position[i], b.position[i]);
		returnVal.position[i] = minPoint;
		i32 maxPoint = Math::Min(a.position[i] + (i32)a.extent[i], b.position[i] + (i32)b.extent[i]);
		returnVal.extent[i] = (u32)Math::Max(0, maxPoint - returnVal.position[i]);
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