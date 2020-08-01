#pragma once

#include <DEngine/FixedWidthTypes.hpp>

#include <DEngine/Math/Vector.hpp>

namespace DEngine::Gui
{
	struct Extent
	{
		u32 width;
		u32 height;
	};

	struct Rect
	{
		Math::Vec2Int position;
		Extent extent;

		bool PointIsInside(Math::Vec2Int point) const
		{
			return point.x >= position.x && point.x < position.x + (i32)extent.width &&
				point.y >= position.y && point.y < position.y + (i32)extent.height;
		}

		bool PointIsInside(Math::Vec2 point) const
		{
			return point.x >= (f32)position.x && point.x < (f32)position.x + (f32)extent.width &&
				point.y >= (f32)position.y && point.y < (f32)position.y + (f32)extent.height;
		}
	};

}