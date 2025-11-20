#include <DEngine/Gui/Gui.hpp>
#include <DEngine/Gui/RectCollection.hpp>

import DEngine.Math.Common;
import DEngine.Std.Trait;

using namespace DEngine;

Std::Opt<Gui::FindBestNavItem_ReturnT> Gui::FindBestNavItem(
	Rect const& focusWidgetRect,
	f32 navAngle,
	RectCollection const& rectColl,
	Std::Span<Widget*> navItems)
{
	auto const& focusWidgetCenter = focusWidgetRect.CenterPoint();

	// Convert navigation angle to a direction vector
	// angle is [0, 1] where 0 = up, rotates counter-clockwise
	Math::Vec2 const navDirection = Gui::NavDirAngleToVec2(navAngle);

	struct Item {
		f32 score;
		int childIndex;
		Widget* widget = nullptr;
		RectCollection::Iterator childIter;
	};
	Std::Opt<Item> bestItemOpt;

	for (int i = 0; i < navItems.Size(); i += 1) {
		DENGINE_IMPL_ASSERT(navItems[i] != nullptr);
		auto& widget = *navItems[i];

		auto childEntryOpt = rectColl.GetEntry(widget);
		DENGINE_IMPL_GUI_ASSERT(childEntryOpt.Has());
		auto childEntry = childEntryOpt.Value();

		if (!rectColl.GetIsInvokable(childEntry))
			continue;

		auto const& childRectPair = rectColl.GetRect(childEntry);
		auto const& childCenter = childRectPair.widgetRect.CenterPoint();

		// Calculate direction from focus widget to this child
		Math::Vec2 toChild = {
			(f32)(childCenter.x - focusWidgetCenter.x),
			(f32)(childCenter.y - focusWidgetCenter.y) };
		f32 const distanceSqrd = toChild.MagnitudeSqrd();
		// Skip if the child is at the same position as focus widget
		if (distanceSqrd < 0.001f) {
			continue;
		}
		auto const toChildNorm = toChild.GetNormalized();
		// Calculate angle difference between navigation direction and direction to child
		// This gives us a measure of how aligned the child is with the navigation direction
		f32 const dotProduct = Math::Vec2::Dot(navDirection, toChildNorm);
		// Only consider children that are in the general direction we're navigating
		if (dotProduct <= 0.f) {
			continue;
		}

		// Score based on combination of angle and distance
		// Lower score is better. We want children that are:
		// 1. Closely aligned with navigation direction (high dot product, close to 1.0)
		// 2. Close in distance
		// Weight the angle more heavily than distance
		auto const angleWeight = 2.f - dotProduct; // Range [1, 2], lower is better
		auto const distance = Math::Sqrt(distanceSqrd);
		auto const score = angleWeight * 1000.f + distance;
		if (!bestItemOpt.Has() || score < bestItemOpt.Get().score) {
			bestItemOpt = Item{
				.score = score,
				.childIndex = i,
				.widget = &widget,
				.childIter = childEntry, };
		}
	}

	// If we found a valid child, navigate to it
	if (bestItemOpt.Has()) {
		auto& result = bestItemOpt.Get();
		return FindBestNavItem_ReturnT {
			.index = result.childIndex,
			.widget = result.widget,
			.childIter = result.childIter,
		};
	}
	return Std::nullOpt;
}

// NOTE: FindBestNavItem2 now lives in the DEngine.Gui.Utility module implementation
// unit (src/Utility.cpp), alongside its declaration.