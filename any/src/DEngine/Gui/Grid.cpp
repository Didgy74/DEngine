#include <DEngine/Gui/Grid.hpp>


#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/Vec.hpp>

#include <DEngine/Math/Common.hpp>

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
			colMax = Math::Max(colMax, childSizeHint.minimum.width);
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
			rowMax = Math::Max(rowMax, childSizeHint.minimum.height);
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
				return (u32)Math::Round((f32)params.elementMinimumLength * scale);
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
				return (u32)Math::Round((f32)(lengthLeftForExpanding) / (f32)params.totalExpandingCount);
			}
		}
	};

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(CursorButton in) noexcept
	{
		switch (in)
		{
			case CursorButton::Primary: return PointerType::Primary;
			case CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}

	static constexpr u8 cursorPointerId = (u8)-1;

	struct PointerPress_Pointer
	{
		u8 id;
		PointerType type;
		Math::Vec2 pos;
		bool pressed;
		bool consumed;
	};
	struct PointerPress_Params
	{
		Context& ctx;
		RectCollection const& rectCollection;
		Grid& widget;
		Rect const& widgetRect;
		Rect const& visibleRect;
		PointerPress_Pointer const& pointer;
		Widget::CursorPressParams const* eventParams_cursorPress;
	};

	[[nodiscard]] static bool PointerPress(PointerPress_Params const& params);

	struct PointerMove_Pointer
	{
		u8 id;
		Math::Vec2 pos;
		bool occluded;
	};
	struct PointerMove_Params
	{
		Context& ctx;
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

SizeHint Grid::GetSizeHint2(
	GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;
	auto& transientAlloc = params.transientAlloc;

	SizeHint returnValue = {};

	auto const childCount = (int)children.size();
	auto childrenSizeHints = Std::NewVec<SizeHint>(transientAlloc);
	childrenSizeHints.Resize(childCount);
	for (int i = 0; i < childCount; i += 1)
	{
		auto& child = children[i];
		if (child)
			childrenSizeHints[i] = child->GetSizeHint2(params);
	}

	auto colMaxWidths = Impl::BuildColMaxWidths(
		*this,
		childrenSizeHints.ToSpan(),
		transientAlloc);
	u32 widthSum = 0;
	for (auto const& item : colMaxWidths)
		widthSum += 0;

	returnValue.minimum.width = widthSum;


	auto rowMaxHeights = Impl::BuildRowMaxHeights(
		*this,
		childrenSizeHints.ToSpan(),
		transientAlloc);
	u32 heightSum = 0;
	for (auto const& item : rowMaxHeights)
		heightSum += item;

	returnValue.minimum.height = heightSum;

	if (width > 0 && height > 0)
	{
		returnValue.minimum.width += spacing * (width - 1);
		returnValue.minimum.height += spacing * (height - 1);
	}

	returnValue.expandX = true;
	returnValue.expandY = true;

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnValue);
	return returnValue;
}

void Grid::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;
	auto& transientAlloc = params.transientAlloc;

	auto const childCount = (int)children.size();

	// Gather all the RectCollection entries for the children
	auto childrenEntries = Std::NewVec<RectCollection::It>(transientAlloc);
	childrenEntries.Resize(childCount);
	for (int i = 0; i < childCount; i++) {
		auto& child = children[i];
		if (child)
			childrenEntries[i] = pusher.GetEntry(*child);
	}

	// Gather all the size-hints for out children.
	auto childrenSizeHints = Std::NewVec<SizeHint>(transientAlloc);
	childrenSizeHints.Resize(childCount);
	for (int i = 0; i < childCount; i += 1)
	{
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
	for (int i = 0; i < 2; i += 1)
	{
		usableExtent[i] = (u32)(widgetRect.extent[i] - spacing * (dimensions[i] - 1));
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
			if (child)
			{
				pusher.SetRectPair(childrenEntries[linearIndex], { childRect, visibleRect });
				child->BuildChildRects(
					params,
					childRect,
					visibleRect);
			}

			posOffsetX += (i32)childRect.extent.width;
			posOffsetX += (i32)spacing;
			remainingWidth -= childRect.extent.width;

			rowHeight = childRect.extent.height;
		}
		posOffsetY += (i32)rowHeight;
		posOffsetY += (i32)spacing;
		remainingHeight -= rowHeight;
	}
}

void Grid::Render2(
	Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& rectCollection = params.rectCollection;

	for (auto& child : children) {
		if (child) {
			auto const* childRectPairPtr = rectCollection.GetRect(*child);
			DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
			child->Render2(
				params,
				childRectPairPtr->widgetRect,
				childRectPairPtr->visibleRect);
		}
	}
}

bool Grid::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	auto& rectCollection = params.rectCollection;
	bool newOccluded = occluded;
	for (auto& child : children) {
		if (child) {
			auto const* childRectPairPtr = rectCollection.GetRect(*child);
			DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
			bool childOccludedReturn = child->CursorMove(
				params,
				childRectPairPtr->widgetRect,
				childRectPairPtr->visibleRect,
				newOccluded);
			newOccluded = newOccluded || childOccludedReturn;
		}
	}
	auto cursorInside = PointIsInAll(params.event.position, { widgetRect, visibleRect });
	return newOccluded || cursorInside;
}

bool Grid::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	auto& rectCollection = params.rectCollection;
	bool newConsumed = consumed;
	for (auto& child : children) {
		if (child) {
			auto const* childRectPairPtr = rectCollection.GetRect(*child);
			DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
			bool childConsumedReturn = child->CursorPress2(
				params,
				childRectPairPtr->widgetRect,
				childRectPairPtr->visibleRect,
				newConsumed);
			newConsumed = newConsumed || childConsumedReturn;
		}
	}
	bool cursorInside =
		widgetRect.PointIsInside(params.cursorPos) &&
		visibleRect.PointIsInside(params.cursorPos);
	return newConsumed || cursorInside;
}

bool Grid::TouchMove2(
	TouchMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	auto& rectCollection = params.rectCollection;
	bool newOccluded = occluded;
	for (auto& child : children) {
		if (child) {
			auto const* childRectPairPtr = rectCollection.GetRect(*child);
			DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
			bool childOccludedReturn = child->TouchMove2(
				params,
				childRectPairPtr->widgetRect,
				childRectPairPtr->visibleRect,
				newOccluded);
			newOccluded = newOccluded || childOccludedReturn;
		}
	}
	auto cursorInside = PointIsInAll(params.event.position, { widgetRect, visibleRect });
	return newOccluded || cursorInside;
}

bool Grid::TouchPress2(
	TouchPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	auto& rectCollection = params.rectCollection;
	bool newConsumed = consumed;
	for (auto& child : children) {
		if (child) {
			auto const* childRectPairPtr = rectCollection.GetRect(*child);
			DENGINE_IMPL_GUI_ASSERT(childRectPairPtr);
			bool childConsumedReturn = child->TouchPress2(
				params,
				childRectPairPtr->widgetRect,
				childRectPairPtr->visibleRect,
				newConsumed);
			newConsumed = newConsumed || childConsumedReturn;
		}
	}
	auto cursorInside = PointIsInAll(params.event.position, { widgetRect, visibleRect });
	return newConsumed || cursorInside;
}

void Grid::TextInput(
	Context& ctx,
	AllocRef const& transientAlloc,
	TextInputEvent const& event)
{
	for (auto& child : children)
	{
		if (child)
		{
			child->TextInput(ctx, transientAlloc, event);
		}
	}
}

void Grid::EndTextInputSession(
	Context& ctx,
	AllocRef const& transientAlloc,
	EndTextInputSessionEvent const& event)
{
	for (auto& child : children)
	{
		if (child)
		{
			child->EndTextInputSession(ctx, transientAlloc, event);
		}
	}
}
