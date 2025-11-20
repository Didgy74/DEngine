#include <DEngine/Gui/StdWidgets/Grid.hpp>



using namespace DEngine;
using namespace DEngine::Gui;

struct Grid::Impl
{
	[[nodiscard]] static constexpr int CalcLinearIndex(int x, int y, int width) noexcept
	{
		return y * width + x;
	}

	[[nodiscard]] static constexpr int ColFromIndex(int index, int width) noexcept {
		return index % width;
	}

	[[nodiscard]] static constexpr int RowFromIndex(int index, int width) noexcept {
		return index / width;
	}

	[[nodiscard]] static auto BuildColMaxWidths(
		Grid const& widget,
		Std::Span<SizeHint> childrenSizeHints,
		AllocRef const& alloc)
	{
		auto colMaxWidths = Std::NewVec<u32>(alloc);
		colMaxWidths.Resize(widget.width);
		for (auto& item : colMaxWidths)
			item = 0;

		for (int i = 0; i < childrenSizeHints.Size(); i += 1)
		{
			auto& child = widget.children[i];
			if (!child)
				continue;

			auto const& childSizeHint = childrenSizeHints[i];
			auto const col = ColFromIndex(i, widget.width);
			auto& colMax = colMaxWidths[col];
			colMax = Std::Max(colMax, childSizeHint.minimum.width);
		}

		return colMaxWidths;
	}

	[[nodiscard]] static auto BuildColExpandX(
		Grid const& widget,
		Std::Span<SizeHint> childrenSizeHints,
		AllocRef alloc)
	{
		auto returnValue = Std::NewVec<bool>(alloc);
		returnValue.Resize(widget.width);
		for (auto& item : returnValue)
			item = 0;

		for (int i = 0; i < childrenSizeHints.Size(); i += 1)
		{
			auto& child = widget.children[i];
			if (!child)
				continue;

			auto const& childSizeHint = childrenSizeHints[i];
			auto const col = ColFromIndex(i, widget.width);
			auto& expandX = returnValue[col];
			expandX = expandX || childSizeHint.expandX;
		}

		return returnValue;
	}

	[[nodiscard]] static auto BuildRowExpandY(
		Grid const& widget,
		Std::Span<SizeHint> childrenSizeHints,
		AllocRef alloc)
	{
		auto returnValue = Std::NewVec<bool>(alloc);
		returnValue.Resize(widget.height);
		for (auto& item : returnValue)
			item = 0;

		for (int i = 0; i < childrenSizeHints.Size(); i += 1)
		{
			auto& child = widget.children[i];
			if (!child)
				continue;

			auto const& childSizeHint = childrenSizeHints[i];
			auto const row = RowFromIndex(i, widget.width);
			auto& expandY = returnValue[row];
			expandY = expandY || childSizeHint.expandY;
		}

		return returnValue;
	}

	[[nodiscard]] static auto BuildRowMaxHeights(
		Grid const& widget,
		Std::Span<SizeHint> childrenSizeHints,
		AllocRef const& alloc)
	{
		auto rowMaxHeights = Std::NewVec<u32>(alloc);
		rowMaxHeights.Resize(widget.height);
		for (auto& item : rowMaxHeights)
			item = 0;

		for (int i = 0; i < childrenSizeHints.Size(); i += 1)
		{
			auto& child = widget.children[i];
			if (!child)
				continue;

			auto const& childSizeHint = childrenSizeHints[i];
			auto const row = RowFromIndex(i, widget.width);
			auto& rowMax = rowMaxHeights[row];
			rowMax = Std::Max(rowMax, childSizeHint.minimum.height);
		}
		return rowMaxHeights;
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
			if (params.isLast)
				return params.remainingLength;
			else
			{
				f32 scale = (f32)params.totalUsableSize / (f32)params.totalLengthSum;
				return (u32)Std::Round((f32)params.elementMinimumLength * scale);
			}
		}
		else
		{
			// We can fit all widgets with space to spare. Don't scale up
			// columns that do not want to expand.
			if (!params.expand)
				return params.elementMinimumLength;
			else
			{
				auto lengthLeftForExpanding = params.totalUsableSize - params.nonExpandingLengthSum;
				return (u32)Std::Round((f32)(lengthLeftForExpanding) / (f32)params.totalExpandingCount);
			}
		}
	};

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept {
		switch (in) {
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerPress_Pointer {
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};
	struct PointerPress_Params {
		RectCollection const& rectCollection;
		Grid& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
		Widget::CursorPressParams const* eventParams_cursorPress;
	};

	[[nodiscard]] static bool PointerPress(PointerPress_Params const& params);

	struct PointerMove_Pointer {
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};
	struct PointerMove_Params {
		Grid& widget;
		RectCollection const& rectCollection;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerMove_Pointer const& pointer;
		Widget::CursorMoveParams const* eventParams_cursorMove;
	};

	[[nodiscard]] static bool PointerMove(PointerMove_Params const& params);
};

void Grid::SetWidth(int newWidth)
{
	width = newWidth;

	DENGINE_IMPL_GUI_ASSERT(children.empty());
}

int Grid::PushBackRow()
{
	height += 1;

	children.resize(width * height);

	return height - 1;
}

void Grid::SetChild(int x, int y, Std::Box<Widget>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(x < width);
	DENGINE_IMPL_GUI_ASSERT(y < height);

	auto const linearIndex = Impl::CalcLinearIndex(x, y, width);
	DENGINE_IMPL_GUI_ASSERT(linearIndex < children.size());

	children[linearIndex] = Std::Move(in);
}

Widget::GetSizeHint2_ReturnT Grid::GetSizeHint2(
	GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;
	auto& window = params.window;
	auto& transientAlloc = params.transientAlloc;

	SizeHint returnValue = {};

	auto const childCount = (int)children.size();
	auto childrenSizeHints = Std::NewVec<SizeHint>(transientAlloc);
	childrenSizeHints.Resize(childCount);
	for (int i = 0; i < childCount; i += 1) {
		auto& child = children[i];
		if (child)
			childrenSizeHints[i] = child->GetSizeHint2(params).sizeHint;
	}

	auto colMaxWidths = Impl::BuildColMaxWidths(
		*this,
		childrenSizeHints.ToSpan(),
		transientAlloc);
	u32 widthSum = 0;

	returnValue.minimum.width = widthSum;

	auto rowMaxHeights = Impl::BuildRowMaxHeights(
		*this,
		childrenSizeHints.ToSpan(),
		transientAlloc);
	u32 heightSum = 0;
	for (auto const& item : rowMaxHeights)
		heightSum += item;

	returnValue.minimum.height = heightSum;

	Grid::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}
	auto spacingPx = theme.spacing.ResolvePx(window.dpi, window.contentScale);

	if (width > 0 && height > 0) {
		returnValue.minimum.width += spacingPx * (width - 1);
		returnValue.minimum.height += spacingPx * (height - 1);
	}

	returnValue.expandX = true;
	returnValue.expandY = true;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnValue, false);
	return {
		.iter = entry,
		.sizeHint = returnValue };
}

void Grid::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& pusher = params.pusher;
	auto& window = params.window;
	auto& transientAlloc = params.transientAlloc;

	Grid::Theme theme;
	if (this->getThemeFn) {
		theme = this->getThemeFn({ .widget = *this, .appData = params.appData });
	}
	auto spacingPx = theme.spacing.ResolvePx(window.dpi, window.contentScale);

	auto const childCount = (int)children.size();

	// Gather all the RectCollection entries for the children
	auto childrenEntries = Std::NewVec<RectCollection::Iter>(transientAlloc);
	childrenEntries.Resize(childCount);
	for (int i = 0; i < childCount; i++) {
		auto& child = children[i];
		if (child)
			childrenEntries[i] = pusher.GetEntry(*child);
	}

	// Gather all the size-hints for out children.
	auto childrenSizeHints = Std::NewVec<SizeHint>(transientAlloc);
	childrenSizeHints.Resize(childCount);
	for (int i = 0; i < childCount; i += 1) {
		auto& child = children[i];
		if (child)
			childrenSizeHints[i] = pusher.GetSizeHint(childrenEntries[i]);
	}

	auto colMaxWidths = Impl::BuildColMaxWidths(
		*this,
		childrenSizeHints.ToSpan(),
		transientAlloc);
	auto colExpands = Impl::BuildColExpandX(
		*this,
		childrenSizeHints.ToSpan(),
		transientAlloc);

	auto rowMaxHeights = Impl::BuildRowMaxHeights(
		*this,
		childrenSizeHints.ToSpan(),
		transientAlloc);
	auto rowExpands = Impl::BuildRowExpandY(
		*this,
		childrenSizeHints.ToSpan(),
		transientAlloc);

	decltype(colMaxWidths) colRowMaxLengths[2] = { Std::Move(colMaxWidths), Std::Move(rowMaxHeights) };
	decltype(colExpands) colRowExpands[2] = { Std::Move(colExpands), Std::Move(rowExpands) };
	int const dimensions[2] = { width, height };
	Extent usableExtent = {};
	u32 totalSums[2] = {};
	u32 nonExpandingColRowSum[2] = {};
	u32 expandingColRowCount[2] = {};
	for (int i = 0; i < 2; i += 1) {
		usableExtent[i] = (u32)(widgetRect.extent[i] - spacingPx * (dimensions[i] - 1));
		for (int j = 0; j < dimensions[i]; j += 1)
		{
			totalSums[i] += colRowMaxLengths[i][j];
			if (colRowExpands[i][j])
				expandingColRowCount[i] += 1;
			else
				nonExpandingColRowSum[i] += colRowMaxLengths[i][j];
		}
	}

	u32 remainingHeight = usableExtent.height;
	i32 posOffsetY = 0;
	for (int currRow = 0; currRow < height; currRow += 1)
	{
		u32 remainingWidth = usableExtent.width;
		i32 posOffsetX = 0;
		u32 rowHeight = 0;
		for (int currCol = 0; currCol < width; currCol += 1)
		{
			Rect childRect = {};
			childRect.position = widgetRect.position;
			childRect.position.x += posOffsetX;
			childRect.position.y += posOffsetY;
			// We use a for loop, one for each extent direction.
			for (int i = 0; i < 2; i += 1)
			{
				auto const& j = i == 0 ? currCol : currRow;
				u32 remainingLengths[] = { remainingWidth, remainingHeight };

				Impl::SizeAlgorithmInput temp = {};
				temp.elementMinimumLength = colRowMaxLengths[i][j];
				temp.expand = colRowExpands[i][j];
				temp.isLast = j == dimensions[i] - 1;
				temp.nonExpandingLengthSum = nonExpandingColRowSum[i];
				temp.remainingLength = remainingLengths[i];
				temp.totalUsableSize = usableExtent[i];
				temp.totalLengthSum = totalSums[i];
				temp.totalExpandingCount = expandingColRowCount[i];

				childRect.extent[i] = Impl::SizeAlgorithm(temp);
			}

			auto const linearIndex = Impl::CalcLinearIndex(currCol, currRow, width);
			auto& child = children[linearIndex];
			if (child) {
				pusher.SetLocalRectPair(childrenEntries[linearIndex], { childRect, visibleRect });
				child->BuildChildRects(
					params,
					childRect,
					visibleRect,
					Std::nullOpt);
			}

			posOffsetX += (i32)childRect.extent.width;
			posOffsetX += (i32)spacingPx;
			remainingWidth -= childRect.extent.width;

			rowHeight = childRect.extent.height;
		}
		posOffsetY += (i32)rowHeight;
		posOffsetY += (i32)spacingPx;
		remainingHeight -= rowHeight;
	}
}

bool Grid::CursorMove(
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

	bool newOccluded = occluded;
	for (auto& child : children) {
		if (child) {
			bool childOccludedReturn = child->CursorMove(
				params,
				Std::nullOpt,
				newOccluded);
			newOccluded = newOccluded || childOccludedReturn;
		}
	}

	auto cursorInside = PointIsInAll(cursorPos, { absWidgetRect, absVisibleRect });
	return newOccluded || cursorInside;
}

bool Grid::CursorPress2(
	Widget::CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto& rectColl = params.rectCollection;
	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	bool newConsumed = consumed;
	for (auto& child : children) {
		if (child) {
			bool childConsumedReturn = child->CursorPress2(
				params,
				Std::nullOpt,
				newConsumed);
			newConsumed = newConsumed || childConsumedReturn;
		}
	}

	bool cursorInside =
		absWidgetRect.PointIsInside(params.cursorPos) &&
		absVisibleRect.PointIsInside(params.cursorPos);
	return newConsumed || cursorInside;
}

TouchEventConsumption Grid::WidgetEvent_TouchMove(
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
	result.consumed = occluded;

	for (auto& child : children) {
		if (!child)
			continue;
		auto childResult = child->WidgetEvent_TouchMove(
			params,
			Std::nullOpt,
			occluded);
		result.consumed = result.consumed || childResult.consumed;
		result.claimDragPriority = result.claimDragPriority || childResult.claimDragPriority;
	}

	auto cursorInside = PointIsInAll(params.event.position, { absWidgetRect, absVisibleRect });
	result.consumed = result.consumed || cursorInside;
	return result;
}

TouchEventConsumption Grid::WidgetEvent_TouchPress(
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
		if (!child)
			continue;
		auto childResult = child->WidgetEvent_TouchPress(
			params,
			Std::nullOpt,
			result.consumed);
		result.consumed = result.consumed || childResult.consumed;
		result.claimDragPriority = result.claimDragPriority || childResult.claimDragPriority;
	}

	auto cursorInside = PointIsInAll(params.event.position, { absWidgetRect, absVisibleRect });
	result.consumed = result.consumed || cursorInside;
	return result;
}

void Grid::WidgetEvent_TextInput(
	AllocRef const& transientAlloc,
	WidgetEvent_TextInputParams const& event)
{
	for (auto& child : children) {
		if (child) {
			child->WidgetEvent_TextInput(transientAlloc, event);
		}
	}
}

void Grid::WidgetEvent_EndTextInputSession(
	AllocRef const& transientAlloc,
	WidgetEvent_EndTextInputSessionParams const& event)
{
	for (auto& child : children) {
		if (child) {
			child->WidgetEvent_EndTextInputSession(transientAlloc, event);
		}
	}
}

void Grid::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectCollection = params.rectCollection;

	for (auto& child : children) {
		if (child) {
			child->Render2(
				params,
				Std::nullOpt);
		}
	}
}

void Grid::AccessibilityTest(
	AccessibilityTest_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& rectColl = params.rectColl;

	for (auto& childBox : children) {
		if (!childBox.Has())
			continue;
		auto const& child = *childBox.Get();

		auto const& childItOpt = rectColl.GetEntry(child);
		DENGINE_IMPL_GUI_ASSERT(childItOpt.Has());
		auto const& childIt = childItOpt.Value();

		auto const childRectPair = rectColl.GetRect(childIt);
		if (childRectPair.visibleRect.IsNothing())
			continue;
		child.AccessibilityTest(
			params,
			childIt);
	}
}
