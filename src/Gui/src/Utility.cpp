module DEngine.Gui.Utility;

import DEngine.FixedWidthTypes;
import DEngine.Math.Common;
import DEngine.Math.Vector;
import DEngine.Std.Opt;
import DEngine.Std.RangeFnRef;

using namespace DEngine;

Std::Opt<Gui::FindBestNavItem2_ReturnT> Gui::FindBestNavItem2(
	Rect const& focusWidgetRect,
	f32 navAngle,
	Std::RangeFnRef<Std::Opt<Rect>> navItems)
{
	auto const& focusWidgetCenter = focusWidgetRect.CenterPoint();

	// Convert navigation angle to a direction vector
	// angle is [0, 1] where 0 = up, rotates counter-clockwise
	Math::Vec2 const navDirection = Gui::NavDirAngleToVec2(navAngle);

	struct Item {
		f32 score;
		int childIndex;
	};
	Std::Opt<Item> bestItemOpt;

	for (int i = 0; i < navItems.Size(); i += 1) {
		auto childRectOpt = navItems.Invoke(i);
		if (!childRectOpt.Has()) {
			continue;
		}
		auto const& childCenter = childRectOpt.Get().CenterPoint();

		// Calculate direction from focus widget to this child
		Math::Vec2 toChild = { (f32) (childCenter.x - focusWidgetCenter.x),
							   (f32) (childCenter.y - focusWidgetCenter.y) };
		f32 const distanceSqrd = toChild.MagnitudeSqrd();
		// Skip if the child is at the same position as focus widget
		if (distanceSqrd < 0.001F) {
			continue;
		}
		auto const toChildNorm = toChild.GetNormalized();
		// Calculate angle difference between navigation direction and direction to child
		// This gives us a measure of how aligned the child is with the navigation direction
		f32 const dotProduct = Math::Vec2::Dot(navDirection, toChildNorm);
		// Only consider children that are in the general direction we're navigating
		if (dotProduct <= 0) {
			continue;
		}

		// Score based on combination of angle and distance
		// Lower score is better. We want children that are:
		// 1. Closely aligned with navigation direction (high dot product, close to 1.0)
		// 2. Close in distance
		// Weight the angle more heavily than distance
		// angleWeight is 0 for perfect alignment (dotProduct == 1) and up to 1000 for
		// perpendicular. This means a perfectly aligned item scores just its distance.
		auto const angleWeight = (1 - dotProduct) * 1000; // Range [0, 1000), lower is better
		auto const distance = Math::Sqrt(distanceSqrd);
		auto const score = angleWeight + distance;
		if (!bestItemOpt.Has() || score < bestItemOpt.Get().score) {
			bestItemOpt = Item {
				.score = score,
				.childIndex = i, };
		}
	}

	// If we found a valid child, navigate to it
	if (bestItemOpt.Has()) {
		auto& result = bestItemOpt.Get();
		return FindBestNavItem2_ReturnT{
			.index = result.childIndex,
		};
	}
	return Std::nullOpt;
}
