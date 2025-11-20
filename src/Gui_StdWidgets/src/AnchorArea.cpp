#include <DEngine/Gui/StdWidgets/AnchorArea.hpp>

#include <DEngine/Std/Containers/FnRef.hpp>

namespace DEngine::Gui::impl
{
	[[nodiscard]] static Rect BuildNodeRect(
		AnchorArea::Node const& node,
		SizeHint const& sizeHint,
		Rect const& widgetRect,
		f32 dpi,
		f32 contentScale) noexcept
	{
		Rect nodeRect = {};

		nodeRect.extent = sizeHint.minimum;
		nodeRect.position = widgetRect.position;

		// Resolved here rather than at construction so the offset tracks DPI and content-scale.
		auto const offsetX = (i32)node.offsetX.ResolvePx(dpi, contentScale);
		switch (node.anchorX)
		{
			case AnchorArea::AnchorX::Left:
				nodeRect.position.x += offsetX;
				break;
			case AnchorArea::AnchorX::Center:
				nodeRect.position.x += (i32)widgetRect.extent.width / 2;
				nodeRect.position.x -= (i32)nodeRect.extent.width / 2;
				break;
			case AnchorArea::AnchorX::Right:
				nodeRect.position.x += (i32)widgetRect.extent.width;
				nodeRect.position.x -= (i32)nodeRect.extent.width;
				nodeRect.position.x -= offsetX;
				break;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}

		auto const offsetY = (i32)node.offsetY.ResolvePx(dpi, contentScale);
		switch (node.anchorY)
		{
			case AnchorArea::AnchorY::Top:
				nodeRect.position.y += offsetY;
				break;
			case AnchorArea::AnchorY::Center:
				nodeRect.position.y += (i32)widgetRect.extent.height / 2;
				nodeRect.position.y -= (i32)nodeRect.extent.height / 2;
				break;
			case AnchorArea::AnchorY::Bottom:
				nodeRect.position.y += (i32)widgetRect.extent.height;
				nodeRect.position.y -= (i32)nodeRect.extent.height;
				nodeRect.position.y -= offsetY;
				break;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}

		return nodeRect;
	}

	struct AA_NodeEndIt {};

	// The templates are only for 'AnchorArea' versus 'AnchorArea const'
	template<class AnchorAreaT>
	struct AA_NodeItResult
	{
		using ChildT = Std::Trait::Cond<Std::Trait::isConst<AnchorAreaT>, Widget const, Widget>;
		ChildT& child;
		uSize index = 0;
		uSize childCount = 0;

		[[nodiscard]] Rect BuildNodeItRect(
			AnchorArea const& anchorArea,
			SizeHint const& sizeHint,
			Rect const& widgetRect,
			f32 dpi,
			f32 contentScale) const
		{
			if (index == childCount)
				return widgetRect;
			else
				return impl::BuildNodeRect(anchorArea.nodes[index], sizeHint, widgetRect, dpi, contentScale);
		}

		[[nodiscard]] Rect GetNodeItRect(
			AnchorArea const& anchorArea,
			Rect const& widgetRect,
			RectCollection const& rectCollection) const
		{
			if (index == childCount)
				return widgetRect;
			else
				return rectCollection.GetRect(child)->widgetRect;
		}
	};

	template<class AnchorAreaT>
	struct AA_NodeIt {
		AnchorAreaT& anchorArea;
		// This is off by one when reverse iterating, so use
		// GetActuaIndex().
		int currIndex = 0;
		int endIndex = 0;
		uSize childCount = 0;
		bool reverse = false;

		[[nodiscard]] int GetActualIndex() const { return !reverse ? currIndex : currIndex - 1; }

		[[nodiscard]] auto operator*() const noexcept
		{
			int actualIndex = GetActualIndex();
			auto* childPtr =
				actualIndex == childCount ?
					anchorArea.backgroundWidget.Get() :
					anchorArea.nodes[actualIndex].widget.Get();
			DENGINE_IMPL_GUI_ASSERT(childPtr);
			AA_NodeItResult<AnchorAreaT> returnValue = {
				.child = *childPtr,
			};
			returnValue.index = actualIndex;
			returnValue.childCount = childCount;

			return returnValue;
		}

		[[nodiscard]] bool operator!=(AA_NodeEndIt const& other) const noexcept
		{
			return currIndex != endIndex;
		}

		AA_NodeIt& operator++() noexcept
		{
			if (!reverse)
				currIndex += 1;
			else
				currIndex -= 1;
			return *this;
		}
	};

	template<class AnchorAreaT>
	struct AA_NodeItPair
	{
		AnchorAreaT& anchorArea;
		int startIndex = 0;
		int endIndex = 0;
		bool reverse = false;

		[[nodiscard]] AA_NodeItPair Reverse() const noexcept
		{
			auto returnValue = *this;
			returnValue.startIndex = endIndex;
			returnValue.endIndex = startIndex;
			returnValue.reverse = !returnValue.reverse;
			return returnValue;
		}

		[[nodiscard]] auto begin() const noexcept
		{
			AA_NodeIt<AnchorAreaT> returnValue = {
				.anchorArea = anchorArea, };
			returnValue.currIndex = startIndex;
			returnValue.childCount = anchorArea.nodes.size();
			returnValue.endIndex = endIndex;
			returnValue.reverse = reverse;
			return returnValue;
		}

		[[nodiscard]] auto end() const noexcept
		{
			return AA_NodeEndIt{};
		}
	};

	[[nodiscard]] auto BuildNodeItPair(
		AnchorArea& anchorArea) noexcept
	{
		AA_NodeItPair<AnchorArea> returnValue = {
			.anchorArea = anchorArea };
		returnValue.startIndex = 0;
		returnValue.endIndex = (int)anchorArea.nodes.size() + 1;
		if (!anchorArea.backgroundWidget)
			returnValue.endIndex -= 1;
		return returnValue;
	}

	[[nodiscard]] auto BuildNodeItPair(
		AnchorArea const& anchorArea) noexcept
	{
		AA_NodeItPair<AnchorArea const> returnValue = {
			.anchorArea = anchorArea };
		returnValue.startIndex = 0;
		returnValue.endIndex = (int)anchorArea.nodes.size() + 1;
		if (!anchorArea.backgroundWidget)
			returnValue.endIndex -= 1;
		return returnValue;
	}
}

namespace DEngine::Gui::impl
{
    struct PointerMove_Pointer {
        Math::Vec2 pos;
        bool occluded;
    };
	using PointerMove_DispatchFnT = Std::FnRef<TouchEventConsumption(
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool occluded)>;
	struct PointerMove_Params {
		AnchorArea& anchorArea;
		RectCollection const& rectCollection;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		PointerMove_Pointer const& pointer;
		PointerMove_DispatchFnT const& dispatchFn;
	};
    [[nodiscard]] static TouchEventConsumption PointerMove(PointerMove_Params const& params) {
    	auto& anchorArea = params.anchorArea;
    	auto& rectColl = params.rectCollection;
    	auto& rectCollIter = params.rectCollIter;
    	auto& pointer = params.pointer;
    	auto& dispatchFn = params.dispatchFn;

    	auto rectPairOpt = rectColl.GetRect(anchorArea, rectCollIter);
    	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
    	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
    	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

		bool const pointerInside = PointIsInAll(pointer.pos, { absWidgetRect, absVisibleRect });

        bool innerOccluded = false;
        TouchEventConsumption result = {};
        for (auto const& node : impl::BuildNodeItPair(anchorArea)) {
            auto childResult = dispatchFn(node.child, Std::nullOpt, innerOccluded);
            if (childResult.consumed)
                innerOccluded = true;
            result.consumed = result.consumed || childResult.consumed;
            result.claimDragPriority = result.claimDragPriority || childResult.claimDragPriority;
        }

        result.consumed = result.consumed || pointerInside;
        return result;
    }

    struct PointerPress_Pointer {
        Math::Vec2 pos;
        bool pressed;
    };
	using PointerPress_DispatchFnT = Std::FnRef<TouchEventConsumption(
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childConsumed)>;
	struct PointerPress_Params {
		AnchorArea& anchorArea;
		RectCollection const& rectCollection;
		Std::Opt<RectCollection::Iter> const& rectCollIter;
		PointerPress_Pointer const& pointer;
		bool eventConsumed;
		PointerPress_DispatchFnT const& dispatchFn;
	};

    [[nodiscard]] static TouchEventConsumption PointerPress(PointerPress_Params const& params) {
    	auto& anchorArea = params.anchorArea;
    	auto& rectColl = params.rectCollection;
    	auto& rectCollIter = params.rectCollIter;
    	auto& pointer = params.pointer;
    	auto& dispatchFn = params.dispatchFn;

    	auto rectPairOpt = rectColl.GetRect(anchorArea, rectCollIter);
    	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
    	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
    	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

    	bool const pointerInside = PointIsInAll(pointer.pos, { absWidgetRect, absVisibleRect });
        if (!pointerInside && pointer.pressed)
            return {};

    	TouchEventConsumption result = { .consumed = params.eventConsumed };

        for (auto const& node : impl::BuildNodeItPair(anchorArea)) {
        	auto childResult = dispatchFn(
        		node.child,
        		Std::nullOpt,
        		result.consumed);
        	result.consumed = result.consumed || childResult.consumed;
        	result.claimDragPriority = result.claimDragPriority || childResult.claimDragPriority;
        }

        result.consumed = result.consumed || pointerInside;
        return result;
    }
}

using namespace DEngine;
using namespace DEngine::Gui;

AnchorArea::AnchorArea() = default;

Widget::GetSizeHint2_ReturnT AnchorArea::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;

	for (auto const& node : impl::BuildNodeItPair(*this)) {
		auto const& child = node.child;
		[[maybe_unused]] auto unused = child.GetSizeHint2(params);
	}

	SizeHint returnValue = {};
	returnValue.expandX = true;
	returnValue.expandY = true;
	returnValue.minimum = { 150, 150 };

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnValue, false);
	return {
		.iter = entry,
		.sizeHint = returnValue };
}

void AnchorArea::BuildChildRects(
	BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto& pusher = params.pusher;

	for (auto const& node : impl::BuildNodeItPair(*this)) {
		auto const& child = node.child;
		auto childEntry = pusher.GetEntry(child);
		auto childSizeHint = pusher.GetSizeHint(childEntry);

		auto const childRect = node.BuildNodeItRect(
			*this,
			childSizeHint,
			widgetRect,
			params.window.dpi,
			params.window.contentScale);

		pusher.SetLocalRectPair(childEntry, { childRect, visibleRect });
		child.BuildChildRects(
			params,
			childRect,
			visibleRect,
			Std::nullOpt);
	}
}

void AnchorArea::Render2(
	Render_Params const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter) const
{
	auto const& rectColl = params.rectCollection;

	auto rectPairOpt = rectColl.GetRect(*this, rectCollIter);
	DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
	auto const& absWidgetRect = rectPairOpt.Value().widgetRect;
	auto const& absVisibleRect = rectPairOpt.Value().visibleRect;

	if (Rect::Intersection({ absWidgetRect, absVisibleRect }).IsNothing()) {
		return;
	}

	for (auto const& node : impl::BuildNodeItPair(*this).Reverse()) {
		auto const& child = node.child;
		auto childRectCollEntryOpt = rectColl.GetEntry(child);
		DENGINE_IMPL_GUI_ASSERT(childRectCollEntryOpt.Has());
		auto childRectCollEntry = childRectCollEntryOpt.Value();

		auto childRectPair = rectColl.GetRect(childRectCollEntry);
		auto const& childAbsWidgetRect = childRectPair.widgetRect;
		auto const& childAbsVisibleRect = childRectPair.visibleRect;
		if (Rect::Intersection({ childAbsWidgetRect, childAbsVisibleRect }).IsNothing()) {
			continue;
		}

		child.Render2(
			params,
			childRectCollEntry);
	}
}

bool AnchorArea::CursorPress2(
	CursorPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto const& rectColl = params.rectCollection;
	auto const& cursorPos = params.cursorPos;

	impl::PointerPress_Pointer pointer = {
		.pos = { (f32)cursorPos.x, (f32)cursorPos.y },
		.pressed = params.CursorPressed(), };

	auto childDispatchFn = [&params](
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childConsumed)
	{
		return TouchEventConsumption{
			.consumed = child.CursorPress2(
				params,
				childRectCollIter,
				childConsumed),
			};
	};

	impl::PointerPress_Params eventParams = {
		.anchorArea = *this,
		.rectCollection = rectColl,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.eventConsumed = consumed,
		.dispatchFn = childDispatchFn, };
	auto temp = impl::PointerPress(eventParams);
	return temp.consumed;
}

bool AnchorArea::CursorMove(
	CursorMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto const& rectColl = params.rectCollection;
	auto const& cursorPos = params.CursorPos();

	impl::PointerMove_Pointer pointer = {
		.pos = { (f32)cursorPos.x, (f32)cursorPos.y },
		.occluded = occluded };

	auto childDispatchFn = [&params](
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childOccluded)
	{
		return TouchEventConsumption{
			.consumed = child.CursorMove(
				params,
				childRectCollIter,
				childOccluded),
			};
	};
	impl::PointerMove_Params eventParams = {
		.anchorArea = *this,
		.rectCollection = rectColl,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.dispatchFn = childDispatchFn, };

	auto temp = impl::PointerMove(eventParams);
	return temp.consumed;
}

TouchEventConsumption AnchorArea::WidgetEvent_TouchMove(
	WidgetEvent_TouchMoveParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool occluded)
{
	auto const& rectColl = params.rectCollection;
	auto const& event = params.event;

	impl::PointerMove_Pointer pointer = {
		.pos = { (f32)event.position.x, (f32)event.position.y },
		.occluded = occluded };

	auto childDispatchFn = [&params](
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childOccluded)
	{
		return child.WidgetEvent_TouchMove(
			params,
			childRectCollIter,
			childOccluded);
	};

	impl::PointerMove_Params eventParams = {
		.anchorArea = *this,
		.rectCollection = rectColl,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.dispatchFn = childDispatchFn, };

	return impl::PointerMove(eventParams);
}
TouchEventConsumption AnchorArea::WidgetEvent_TouchPress(
	WidgetEvent_TouchPressParams const& params,
	Std::Opt<RectCollection::Iter> const& rectCollIter,
	bool consumed)
{
	auto const& rectColl = params.rectCollection;
	auto const& event = params.event;

	impl::PointerPress_Pointer pointer = {
		.pos = event.position,
		.pressed = event.pressed, };

	auto childDispatchFn = [&params](
		Widget& child,
		Std::Opt<RectCollection::Iter> const& childRectCollIter,
		bool childConsumed)
	{
		return child.WidgetEvent_TouchPress(
			params,
			childRectCollIter,
			childConsumed);
	};

	impl::PointerPress_Params eventParams = {
		.anchorArea = *this,
		.rectCollection = rectColl,
		.rectCollIter = rectCollIter,
		.pointer = pointer,
		.eventConsumed = consumed,
		.dispatchFn = childDispatchFn, };

	return impl::PointerPress(eventParams);
}