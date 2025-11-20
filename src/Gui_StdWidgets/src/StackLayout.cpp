#include <DEngine/Gui/StdWidgets/StackLayout.hpp>


import DEngine.Gui.DrawEngine;

using namespace DEngine;
using namespace DEngine::Gui;

struct StackLayout::Impl {
	// Contains references to stuff like Extent width and height based on
	// the StackLayout direction. This is so we don't have
	// to care whether the StackLayout is horizontal or vertical
	template<class T>
	struct StackLayoutDirRefs {
		T& dir;
		T& nonDir;
	};
	[[nodiscard]] static auto GetDirRefs(StackLayout::Direction dir, Extent& in) noexcept
		-> StackLayoutDirRefs<u32>
	{
		if (dir == StackLayout::Direction::Horizontal) {
			return { in.width, in.height };
		}
		return { in.height, in.width };
	}

	[[nodiscard]] static auto GetDirRefs(StackLayout::Direction dir, Extent const& in) noexcept
		-> StackLayoutDirRefs<u32 const>
	{
		if (dir == StackLayout::Direction::Horizontal) {
			return { in.width, in.height };
		}
		return { in.height, in.width };
	}

	[[nodiscard]] static auto GetDirRefs(StackLayout::Direction dir, Math::Vec2Int& in) noexcept
		-> StackLayoutDirRefs<decltype(in.x)>
	{
		if (dir == StackLayout::Direction::Horizontal) {
			return { in.x, in.y };
		}
		return { in.y, in.x };
	}

	[[nodiscard]] static bool GetDirExpand(StackLayout::Direction dir, SizeHint const& sizeHint) noexcept
	{
		if (dir == StackLayout::Direction::Horizontal) {
			return sizeHint.expandX;
		}
		return sizeHint.expandY;
	}

	[[nodiscard]] static Extent GetSizeHintExtentSum(
		StackLayout::Direction dir,
		Std::Span<SizeHint const> const& sizeHints) noexcept
	{
		Extent returnVal = {};

		auto returnValRefs = GetDirRefs(dir, returnVal);

		for (auto const& childSizeHint : sizeHints)
		{
			auto const childRefs = GetDirRefs(dir, childSizeHint.minimum);
			returnValRefs.dir += childRefs.dir;
			returnValRefs.nonDir = Std::Max(returnValRefs.nonDir, childRefs.nonDir);
		}

		return returnVal;
	}

	// Returns the sum length of non-expanded child-SizeHints
	[[nodiscard]] static u32 GetNonExpandedChildrenSumDirLength(
		StackLayout::Direction dir,
		Std::Span<SizeHint const> const& sizeHints) noexcept
	{
		u32 returnVal = 0;
		for (auto const& childSizeHint : sizeHints)
		{
			if (!GetDirExpand(dir, childSizeHint))
			{
				auto const childRefs = GetDirRefs(dir, childSizeHint.minimum);
				returnVal += childRefs.dir;
			}
		}
		return returnVal;
	}

	[[nodiscard]] static u32 GetExpandChildrenCount(
		StackLayout::Direction dir,
		Std::Span<SizeHint const> const& sizeHints) noexcept
	{
		u32 returnValue = 0;
		for (auto const& childSizeHint : sizeHints)
		{
			if (GetDirExpand(dir, childSizeHint)) {
				returnValue += 1;
			}
		}
		return returnValue;
	}

	struct SizeAlgorithmInput
	{
		u32 totalUsableSize;
		u32 totalLengthSum;
		u32 remainingLength;
		u32 nonExpandingLengthSum;
		u32 totalExpandingCount;
		u32 elementMinimumLength;
		bool expand;
		bool isLast;
	};

	static u32 SizeAlgorithm(SizeAlgorithmInput const& params)
	{
		// If we can fit all elements, scale them down according to
		// their sizes relative to each other.
		if (params.totalUsableSize < params.totalLengthSum)
		{
			if (params.isLast) {
				return params.remainingLength;
			} else {
				f32 scale = (f32)params.totalUsableSize / (f32)params.totalLengthSum;
				return (u32)Std::Round((f32)params.elementMinimumLength * scale);
			}
		}
		else
		{
			// We can fit all widgets with space to spare. Don't scale up
			// columns that do not want to expand.
			if (!params.expand) {
				return params.elementMinimumLength;
			} else {
				auto lengthLeftForExpanding = params.totalUsableSize - params.nonExpandingLengthSum;
				return (u32)Std::Round((f32)(lengthLeftForExpanding) / (f32)params.totalExpandingCount);
			}
		}
	};

	[[nodiscard]] static auto BuildChildRects(
		Rect const& parentRect,
		StackLayout::Direction dir,
		u32 spacing,
		Std::Span<SizeHint const> sizeHints,
		AllocRef const& transientAlloc)
	{
		auto const childCount = (int)sizeHints.Size();
		if (childCount <= 0) {
			return Std::NewVec<Rect>(transientAlloc);
		}

		auto parentExtent = GetDirRefs(dir, parentRect.extent);

		Extent usableExtent_Inner = {};
		auto usableExtent = GetDirRefs(dir, usableExtent_Inner);
		usableExtent.dir = parentExtent.dir - spacing * (childCount - 1);
		usableExtent.nonDir = parentExtent.nonDir;

		auto const sumChildMinExtent_Inner = GetSizeHintExtentSum(dir, sizeHints);
		auto const sumChildMinExtent = GetDirRefs(dir, sumChildMinExtent_Inner);
		// Holds the minimum length of non-children in the direction dir
		auto const sumNonExpandedChildMinDir = GetNonExpandedChildrenSumDirLength(dir, sizeHints);
		u32 const expandedChildCount = GetExpandChildrenCount(dir, sizeHints);

		auto returnVal = Std::NewVec<Rect>(transientAlloc);
		returnVal.Resize(childCount);

		auto remainingLength = usableExtent.dir;
		auto childPos = parentRect.position;
		auto const childPosRefs = GetDirRefs(dir, childPos);
		for (int i = 0; i < childCount; i += 1)
		{
			auto& childRect = returnVal[i];
			childRect.position = childPos;

			auto childExtent = GetDirRefs(dir, childRect.extent);
			auto const& childSizeHint_Inner = sizeHints[i];
			bool const childDirExpand = GetDirExpand(dir, childSizeHint_Inner);
			auto childSizeHintExtent = GetDirRefs(dir, childSizeHint_Inner.minimum);

			Impl::SizeAlgorithmInput temp = {};
			temp.totalUsableSize = usableExtent.dir;
			temp.totalLengthSum = sumChildMinExtent.dir;
			temp.elementMinimumLength = childSizeHintExtent.dir;
			temp.totalExpandingCount = expandedChildCount;
			temp.remainingLength = remainingLength;
			temp.nonExpandingLengthSum = sumNonExpandedChildMinDir;
			temp.isLast = i == childCount - 1;
			temp.expand = childDirExpand;

			childExtent.dir = Impl::SizeAlgorithm(temp);

			// In the non-dir direction, children are always assigned the entire length.
			childExtent.nonDir = usableExtent.nonDir;

			remainingLength -= childExtent.dir;
			childPosRefs.dir += (i32)childExtent.dir + spacing;
		}

		return returnVal;
	}

	// Returns true if any invokable sibling exists before childIndex.
	[[nodiscard]] static bool HasInvokableSiblingBefore(
		StackLayout const& stackLayout,
		RectCollection const& rectColl,
		int childIndex) noexcept
	{
		for (int i = childIndex - 1; i >= 0; i -= 1) {
			auto const& candidate = stackLayout.children[i];
			if (!candidate.Has()) {
				continue;
			}
			auto entryOpt = rectColl.GetEntry(*candidate);
			if (!entryOpt.Has()) {
				continue;
			}
			if (rectColl.GetIsInvokable(entryOpt.Value())) {
				return true;
			}
		}
		return false;
	}

	// Returns true if any invokable sibling exists after childIndex.
	[[nodiscard]] static bool HasInvokableSiblingAfter(
		StackLayout const& stackLayout,
		RectCollection const& rectColl,
		int childIndex) noexcept
	{
		for (int i = childIndex + 1; i < (int)stackLayout.children.size(); i += 1) {
			auto const& candidate = stackLayout.children[i];
			if (!candidate.Has()) {
				continue;
			}
			auto entryOpt = rectColl.GetEntry(*candidate);
			if (!entryOpt.Has()) {
				continue;
			}
			if (rectColl.GetIsInvokable(entryOpt.Value())) {
				return true;
			}
		}
		return false;
	}

	// Returns true if we should return out of the WidgetEvent_Navigation
	[[nodiscard]] static bool Navigation_HandleCase_NotConsumed_NoFocus(
		StackLayout&,
		Widget_NavigationParams const&,
		Widget_NavigationState&);
};

namespace DEngine::Gui::impl
{
	// Generates the rect for the inner StackLayout Rect with padding taken into account
	[[nodiscard]] static Rect BuildInnerRect(
		Rect const& widgetRect,
		u32 padding) noexcept
	{
		Rect returnVal = widgetRect;
		returnVal = returnVal.Reduce(padding);
		return returnVal;
	}
}

StackLayout::~StackLayout()
{
}

uSize StackLayout::ChildCount() const
{
	return (uSize)children.size();
}

Widget& StackLayout::At(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto& child = children[index];

	DENGINE_IMPL_GUI_ASSERT(child);

	return *child;
}

Widget const& StackLayout::At(uSize index) const {
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto const& child = children[index];
	DENGINE_IMPL_GUI_ASSERT(child);
	return *child;
}

void StackLayout::AddWidget(Std::Box<Widget>&& in) {
	DENGINE_IMPL_GUI_ASSERT(in);
	children.emplace_back(static_cast<Std::Box<Widget>&&>(in));
}

Std::Box<Widget> StackLayout::ExtractChild(uSize index) {
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto& item = children[index];
	auto returnVal = static_cast<Std::Box<Widget>&&>(item);
	children.erase(children.begin() + index);
	return returnVal;

}

void StackLayout::InsertWidget(uSize index, Std::Box<Widget>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	DENGINE_IMPL_GUI_ASSERT(in);

	children.insert(children.begin() + index, static_cast<Std::Box<Widget>&&>(in));
}

void StackLayout::RemoveItem(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());

	children.erase(children.begin() + index);
}

void StackLayout::ClearChildren()
{
	children.clear();
}

Widget::GetSizeHint2_ReturnT StackLayout::GetSizeHint2(GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;

	auto const childCount = (int)children.size();

	SizeHint returnVal = {};

	bool anyChildIsInvokable = false;
	auto childSizeHints = Std::NewVec<SizeHint>(params.transientAlloc);
	childSizeHints.Resize(childCount);
	for (int i = 0; i < childCount; i += 1) {
		auto childSizeHintResult = children[i]->GetSizeHint2(params);
		childSizeHints[i] = childSizeHintResult.sizeHint;
		if (!anyChildIsInvokable) {
			anyChildIsInvokable = anyChildIsInvokable || pusher.GetIsInvokable(childSizeHintResult.iter);
		}
	}

	returnVal.minimum = Impl::GetSizeHintExtentSum(
		direction,
		childSizeHints.ToSpan());

	auto returnValRefs = Impl::GetDirRefs(direction, returnVal.minimum);

	StackLayout::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}
	auto resolvedSpacing = theme.spacing.ResolvePx(params.window.dpi, params.window.contentScale);

	// Add spacing
	if (childCount > 0) {
		returnValRefs.dir += resolvedSpacing * (childCount - 1);
	}

	// Add padding
	/*
	returnValRefs.dir += padding * 2;
	returnValRefs.nonDir += padding * 2;
	*/

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnVal, anyChildIsInvokable);
	return {
		.iter = entry,
		.sizeHint = returnVal };
}

void StackLayout::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& pusher = params.pusher;
	auto& transientAlloc = params.transientAlloc;

	uSize const childCount = children.size();

	// Gather the iterator-items for our children, so we can read and write the correct
	// values into the collection.
	auto childEntries = Std::NewVec<RectCollection::Iter>(transientAlloc);
	childEntries.Reserve(childCount);
	for (auto const& child : children) {
		childEntries.PushBack(pusher.GetEntry(*child));
	}

	auto childSizeHints = Std::NewVec<SizeHint>(transientAlloc);
	childSizeHints.Reserve(childCount);
	for (auto const& childEntry : childEntries) {
		childSizeHints.PushBack(pusher.GetSizeHint(childEntry));
	}

	auto const innerRect = impl::BuildInnerRect(widgetRect, 0);

	StackLayout::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}
	auto resolvedSpacing = theme.spacing.ResolvePx(params.window.dpi, params.window.contentScale);

	auto const childRects = Impl::BuildChildRects(
		innerRect,
		direction,
		resolvedSpacing,
		childSizeHints.ToSpan(),
		transientAlloc);

	for (uSize i = 0; i < childCount; i += 1) {
		auto const& child = *children[i];
		auto const& childRect = childRects[i];
		if (childRect.GetIntersect(visibleRect).IsNothing()) {
			continue;
		}

		pusher.SetLocalRectPair(childEntries[i], { childRects[i], visibleRect });
		child.BuildChildRects(
			params,
			childRects[i],
			visibleRect,
			Std::nullOpt);
	}
}

void StackLayout::CursorExit() {
	for (auto& child : children) {
		child.Get()->CursorExit();
	}
}

bool StackLayout::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto cursorPos = params.CursorPos();

	auto& rectColl = params.rectCollection;
	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	for (auto& child : children) {
		child.Get()->CursorMove(
			params,
			{},
			occluded);
	}

	return
		occluded
		|| PointIsInAll(cursorPos, { absWidgetRect, absVisibleRect });
}

bool StackLayout::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumedIn)
{
	auto& rectColl = params.rectCollection;
	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	bool consumedOut = consumedIn;

	for (auto& child : children) {
		bool newResult = child.Get()->CursorPress2(
			params,
			{},
			consumedOut);
		consumedOut = consumedOut || newResult;
	}

	if (PointIsInAll(params.cursorPos, { absWidgetRect, absVisibleRect })) {
		consumedOut = true;
	}

	return consumedOut;
}

TouchEventConsumption StackLayout::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto& rectColl = params.rectCollection;
	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	TouchEventConsumption result = {};

	for (auto& child : children) {
		auto childResult = child.Get()->WidgetEvent_TouchMove(
			params,
			{},
			occluded);

		result.claimDragPriority = result.claimDragPriority || childResult.claimDragPriority;
	}

	result.consumed = occluded || PointIsInAll(params.event.position, { absWidgetRect, absVisibleRect });
	return result;
}

TouchEventConsumption StackLayout::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto& rectColl = params.rectCollection;
	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	TouchEventConsumption result = {};
	result.consumed = consumed;

	for (auto& child : children) {
		auto childResult = child.Get()->WidgetEvent_TouchPress(
			params,
			{},
			result.consumed);

		result.consumed = result.consumed || childResult.consumed;
		result.claimDragPriority = result.claimDragPriority || childResult.claimDragPriority;
	}

	result.consumed = consumed || PointIsInAll(params.event.position, { absWidgetRect, absVisibleRect });

	return result;
}

void StackLayout::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectCollection;
	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	// innerRect is the smaller rect we have, with our padding applied.
	auto const contentRect = impl::BuildInnerRect(absWidgetRect, 0);
	if (Rect::Intersection(contentRect, absVisibleRect).IsNothing()) {
		return;
	}

	// Only draw the quad of the layout if the alpha of the color is high enough to be visible.
	if (color.w > 0.01f) {
		params.drawEngine.PushFilledQuad(contentRect, color);
	}

	auto const childCount = children.size();
	for (uSize childIndex = 0; childIndex < childCount; childIndex += 1) {
		auto const& child = *children[childIndex];
		child.Render2(
			params,
			{});
	}
}

void StackLayout::WidgetEvent_TextInput(
	AllocRef const& transientAlloc,
	WidgetEvent_TextInputParams const& event)
{
	for (auto& child : children) {
		child.Get()->WidgetEvent_TextInput(
			transientAlloc,
			event);
	}
}

void StackLayout::WidgetEvent_TextSelection(
	AllocRef const& transientAlloc,
	WidgetEvent_TextSelectionParams const& event)
{
	for (auto& child : children) {
		child.Get()->WidgetEvent_TextSelection(
			transientAlloc,
			event);
	}
}

void StackLayout::WidgetEvent_EndTextInputSession(
	AllocRef const& transientAlloc,
	WidgetEvent_EndTextInputSessionParams const& event)
{
	for (auto& child : children) {
		child.Get()->WidgetEvent_EndTextInputSession(
			transientAlloc,
			event);
	}
}

void StackLayout::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectColl;
	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	// innerRect is the smaller rect we have, with our padding applied.
	auto const contentRect = impl::BuildInnerRect(absWidgetRect, 0);
	if (Rect::Intersection(contentRect, absVisibleRect).IsNothing()) {
		return;
	}

	for (auto const& childBox : this->children) {
		childBox.Get()->AccessibilityTest(
			params,
			{});
	}
}

bool StackLayout::Impl::Navigation_HandleCase_NotConsumed_NoFocus(
	StackLayout& stackLayout,
	Widget_NavigationParams const& params,
	Widget_NavigationState& event)
{
	auto directionalAngle = params.angle;

	// If there is currently no focus, we should select the first child

	// If there are no children, don't consume and early return.
	if (stackLayout.children.empty()) {
		return true;
	}

	if (!params.directional) {
		DENGINE_IMPL_GUI_UNREACHABLE();
		// TODO: Actually we have to go down into the children and get them to select the first one
	}

	if (stackLayout.direction == Direction::Vertical) {
		if (DirAngleIsQuadrantLeft(directionalAngle) || DirAngleIsQuadrantRight(directionalAngle)) {
			// Go down the first child.
			auto& child = *stackLayout.children.front();
			child.WidgetEvent_Navigation(params, Std::nullOpt, event);
			if (event.IsConsumed()) {
				return true;
			}
			DENGINE_IMPL_GUI_UNREACHABLE();
		}

		// Check if angle is within up/down quadrant
		if (DirAngleIsQuadrantUp(directionalAngle)) { // Pointing up
			// We are pointing up, select the bottom-most item.
			// TODO: Actually we have to go down into the children and get them to select the first one
			DENGINE_IMPL_GUI_UNREACHABLE();
		}
		if (DirAngleIsQuadrantDown(directionalAngle)) {
			// TODO: We need to determine if this is an item that can be activated. Otherwise,
			// search for the first one that can be activated.
			// TODO: Actually we have to go down into the children and get them to select the first one

			auto& child = *stackLayout.children.front();
			child.WidgetEvent_Navigation(
				params,
				Std::nullOpt,
				event);
			DENGINE_IMPL_GUI_UNREACHABLE();
		}
	} else { // Horizontal
		if (DirAngleIsQuadrantUp(directionalAngle) || DirAngleIsQuadrantDown(directionalAngle)) {
			// Go down the first child.
			auto& child = *stackLayout.children.front();
			child.WidgetEvent_Navigation(params, Std::nullOpt, event);
			if (event.IsConsumed()) {
				return true;
			}
			DENGINE_IMPL_GUI_UNREACHABLE();
		}

		// Check if angle is within up/down quadrant
		if (DirAngleIsQuadrantLeft(directionalAngle)) {
			stackLayout.children.back()->WidgetEvent_Navigation(
				params,
				Std::nullOpt,
				event);
			if (event.IsConsumed()) {
				return true;
			}
			DENGINE_IMPL_GUI_UNREACHABLE();
		}
		if (DirAngleIsQuadrantRight(directionalAngle)) {
			// TODO: We need to iterate over all children until we find one that consumes.
			for (auto& child : stackLayout.children) {
				child->WidgetEvent_Navigation(
					params,
					Std::nullOpt,
					event);
				if (event.IsConsumed()) {
					return true;
				}
			}
			return false;
		}
	}

	return false;
}

void StackLayout::WidgetEvent_Navigation(
	Widget_NavigationParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	Widget_NavigationState& event)
{
	auto const& rectColl = params.rectCollection;
	auto directionalNav = params.directional;
	auto directionalAngle = params.angle;

	if (event.State() == Widget_NavigationEventState::NotConsumed && params.CurrentFocusWidget() == nullptr) {
		auto result = Impl::Navigation_HandleCase_NotConsumed_NoFocus(*this, params, event);
		if (result) {
			return;
		}
	}

	if (event.State() == Widget_NavigationEventState::HitBoundary) {
		// We want to transition into this Layout from the focus-widget.
		// Figure out where the focus is widget is, then figure out which one of our children to pick.
		// The algorithm should take into account the center of the focus widget,
		// the direction from that focus widget projected onto our children, and then
		// pick a child based on lowest angle + distance.
		DENGINE_IMPL_ASSERT(params.currentFocusOpt.Has());
		auto const focusRect = event.sourceRectHitBoundary;
		auto const childCount = (int)this->children.size();

		auto bestNavItemOpt = Gui::FindBestNavItem2(
			focusRect,
			params.angle,
			{ childCount, [&](int i) -> Std::Opt<Rect> {
				auto const& child = this->children[i];
				if (!child.Has()) { return Std::nullOpt; }
				auto entryOpt = rectColl.GetEntry(*child);
				if (!entryOpt.Has()) { return Std::nullOpt; }
				auto const entry = entryOpt.Value();
				if (!rectColl.GetIsInvokable(entry)) { return Std::nullOpt; }
				return rectColl.GetRect(entry).widgetRect;
			}});

		if (bestNavItemOpt.Has()) {
			auto& bestChild = *this->children[bestNavItemOpt.Get().index];
			bestChild.WidgetEvent_Navigation(
				params,
				Std::nullOpt,
				event);
			if (event.IsConsumed()) {
				return;
			}
		}

		// No valid child found, propagate the boundary hit upward
		return;
	}

	// Iterate through children, then search to see if any of our children contain
	// the focused widget. If any child widgets consume the events, propagate result.
	// If a widget says it has the focused widget but hit a boundary, we try to find
	// the next suitable widget.
	// TODO: Might make sense to actually make this act like a search algorithm instead
	for (int childIndex = 0; childIndex < (int)this->children.size(); childIndex += 1) {
		if (!this->children[childIndex].Has()) {
			continue;
		}

		auto& child = *this->children[childIndex];
		child.WidgetEvent_Navigation(
			params,
			Std::nullOpt,
			event);
		if (event.IsConsumed()) {
			return;
		}
		if (!event.IsHitBoundary()) {
			continue;
		}

		// Child contains focused Widget, but encountered boundary
		if (!directionalNav) {
			DENGINE_IMPL_GUI_UNREACHABLE();
		}

		if (this->direction == Direction::Vertical) {
			bool const hasPrev = Impl::HasInvokableSiblingBefore(*this, rectColl, childIndex);
			bool const hasNext = Impl::HasInvokableSiblingAfter(*this, rectColl, childIndex);

			// Vertical layout never has horizontal siblings. Require an octant-precise push
			// so the user must be deliberate about leaving this layout sideways.
			if (DirAngleIsOctantLeft(directionalAngle) || DirAngleIsOctantRight(directionalAngle)) {
				return;
			}

			// Widen to quadrant when a sibling exists; keep octant when we are at the boundary.
			bool const isUp = hasPrev
				? DirAngleIsQuadrantUp(directionalAngle)
				: DirAngleIsOctantUp(directionalAngle);
			bool const isDown = hasNext
				? DirAngleIsQuadrantDown(directionalAngle)
				: DirAngleIsOctantDown(directionalAngle);

			if (isUp) {
				for (int childCandidateIndex = childIndex - 1; childCandidateIndex >= 0; childCandidateIndex -= 1) {
					auto& childCandidate = this->children[childCandidateIndex];
					if (!childCandidate.Has()) {
						continue;
					}

					auto childCandidateEntryOpt = rectColl.GetEntry(*childCandidate);
					DENGINE_IMPL_GUI_ASSERT(childCandidateEntryOpt.Has());
					auto childCandidateEntry = childCandidateEntryOpt.Value();

					if (!rectColl.GetIsInvokable(childCandidateEntry)) {
						continue;
					}

					childCandidate->WidgetEvent_Navigation(params, childCandidateEntry, event);
					if (event.IsConsumed()) {
						return;
					}
					DENGINE_IMPL_GUI_UNREACHABLE();
				}
				return;
			}
			if (isDown) {
				for (int childCandidateIndex = childIndex + 1; childCandidateIndex < (int)this->children.size(); childCandidateIndex += 1) {
					auto& childCandidate = this->children[childCandidateIndex];
					if (!childCandidate.Has()) {
						continue;
					}

					auto childCandidateEntryOpt = rectColl.GetEntry(*childCandidate);
					DENGINE_IMPL_GUI_ASSERT(childCandidateEntryOpt.Has());
					auto childCandidateEntry = childCandidateEntryOpt.Value();

					if (!rectColl.GetIsInvokable(childCandidateEntry)) {
						continue;
					}

					childCandidate->WidgetEvent_Navigation(params, childCandidateEntry, event);
					if (event.IsConsumed()) {
						return;
					}
					DENGINE_IMPL_GUI_UNREACHABLE();
				}
				return;
			}
			// Angle falls in a diagonal gap between octants — treat as boundary hit.
			return;
		} else { // Horizontal
			bool const hasPrev = Impl::HasInvokableSiblingBefore(*this, rectColl, childIndex);
			bool const hasNext = Impl::HasInvokableSiblingAfter(*this, rectColl, childIndex);

			// Horizontal layout never has vertical siblings.
			if (DirAngleIsOctantUp(directionalAngle) || DirAngleIsOctantDown(directionalAngle)) {
				return;
			}

			bool const isLeft = hasPrev
				? DirAngleIsQuadrantLeft(directionalAngle)
				: DirAngleIsOctantLeft(directionalAngle);
			bool const isRight = hasNext
				? DirAngleIsQuadrantRight(directionalAngle)
				: DirAngleIsOctantRight(directionalAngle);

			if (isLeft) {
				for (int childCandidateIndex = childIndex - 1; childCandidateIndex >= 0; childCandidateIndex -= 1) {
					auto& childCandidate = this->children[childCandidateIndex];
					if (!childCandidate.Has()) {
						continue;
					}

					auto childCandidateEntryOpt = rectColl.GetEntry(*childCandidate);
					DENGINE_IMPL_GUI_ASSERT(childCandidateEntryOpt.Has());
					auto childCandidateEntry = childCandidateEntryOpt.Value();

					if (!rectColl.GetIsInvokable(childCandidateEntry)) {
						continue;
					}

					childCandidate->WidgetEvent_Navigation(params, childCandidateEntry, event);
					if (event.IsConsumed()) {
						return;
					}
					DENGINE_IMPL_GUI_UNREACHABLE();
				}
				return;
			}
			if (isRight) {
				for (int childCandidateIndex = childIndex + 1; childCandidateIndex < (int)this->children.size(); childCandidateIndex += 1) {
					auto& childCandidate = this->children[childCandidateIndex];
					if (!childCandidate.Has()) {
						continue;
					}

					auto childCandidateEntryOpt = rectColl.GetEntry(*childCandidate);
					DENGINE_IMPL_GUI_ASSERT(childCandidateEntryOpt.Has());
					auto childCandidateEntry = childCandidateEntryOpt.Value();

					if (!rectColl.GetIsInvokable(childCandidateEntry)) {
						continue;
					}

					childCandidate->WidgetEvent_Navigation(params, childCandidateEntry, event);
					if (event.IsConsumed()) {
						return;
					}
					DENGINE_IMPL_GUI_UNREACHABLE();
				}
				return;
			}
			// Angle falls in a diagonal gap between octants — treat as boundary hit.
			return;
		}
	}
}
