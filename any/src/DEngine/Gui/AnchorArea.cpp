#include <DEngine/Gui/AnchorArea.hpp>

#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Trait.hpp>

namespace DEngine::Gui::impl
{
    [[nodiscard]] static Rect BuildNodeRect(
        AnchorArea::Node const& node,
        Rect const& widgetRect) noexcept
    {
        Rect nodeRect = {};

		nodeRect.extent = node.extent;
		nodeRect.position = widgetRect.position;
		switch (node.anchorX)
		{
			case AnchorArea::AnchorX::Left:
				break;
			case AnchorArea::AnchorX::Center:
				nodeRect.position.x += (i32)widgetRect.extent.width / 2;
				nodeRect.position.x -= (i32)nodeRect.extent.width / 2;
				break;
			case AnchorArea::AnchorX::Right:
				nodeRect.position.x += (i32)widgetRect.extent.width;
				nodeRect.position.x -= (i32)nodeRect.extent.width;
				break;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}
		switch (node.anchorY)
		{
			case AnchorArea::AnchorY::Top:
				break;
			case AnchorArea::AnchorY::Center:
				nodeRect.position.y += (i32)widgetRect.extent.height / 2;
				nodeRect.position.y -= (i32)nodeRect.extent.height / 2;
				break;
			case AnchorArea::AnchorY::Bottom:
				nodeRect.position.y += (i32)widgetRect.extent.height;
				nodeRect.position.y -= (i32)nodeRect.extent.height;
				break;
			default:
				DENGINE_IMPL_UNREACHABLE();
				break;
		}

        return nodeRect;
    }

    struct PointerMove_Pointer
    {
        Math::Vec2 pos;
        bool occluded;
    };
    template<class T>
    [[nodiscard]] static bool PointerMove(
        AnchorArea& widget,
        Context& ctx,
        WindowID windowId,
        Rect const& widgetRect,
        Rect const& visibleRect,
        PointerMove_Pointer const& pointer,
        T const& event)
    {
        bool const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
        
        bool innerOccluded = false;
        for (uSize i = 0; i < widget.nodes.size(); i += 1)
        {
            auto& node = widget.nodes[i];
            DENGINE_IMPL_GUI_ASSERT(node.widget);

            /*auto const nodeRect = impl::GetNodeRect(
                { widget.nodes.data(), widget.nodes.size() },
                widgetRect,
                i);*/
			auto const nodeRect = Rect();

            bool newOccluded = false;
            using Type = Std::Trait::RemoveCVRef<decltype(event)>;
            if constexpr (Std::Trait::isSame<Type, CursorMoveEvent>)
            {
                newOccluded = node.widget->CursorMove(
                    ctx,
                    windowId,
                    nodeRect,
                    visibleRect,
                    event,
                    pointer.occluded || innerOccluded);
            }
            else if constexpr (Std::Trait::isSame<Type, TouchMoveEvent>)
            {
                newOccluded = node.widget->TouchMoveEvent(
                    ctx,
                    windowId,
                    nodeRect,
                    visibleRect,
                    event,
                    pointer.occluded || innerOccluded);
            }

            if (newOccluded)
                innerOccluded = true;
        }

        return pointerInside;
    }

    struct PointerPress_Pointer
    {
        Math::Vec2 pos;
        bool pressed;
		bool consumed;
    };
    
    template<class T>
    [[nodiscard]] static bool PointerPress(
        AnchorArea& widget,
        Context& ctx,
        WindowID windowId,
        Rect const& widgetRect,
        Rect const& visibleRect,
        PointerPress_Pointer const& pointer,
        T const& event)
    {
        bool const pointerInside = widgetRect.PointIsInside(pointer.pos) && visibleRect.PointIsInside(pointer.pos);
        if (!pointerInside && pointer.pressed)
            return false;

        for (uSize i = 0; i < widget.nodes.size(); i += 1)
        {
            auto& node = widget.nodes[i];
            DENGINE_IMPL_GUI_ASSERT(node.widget);



            /*auto const nodeRect = impl::GetNodeRect(
                { widget.nodes.data(), widget.nodes.size() },
                widgetRect,
                i);*/
			auto const nodeRect = Rect();

            bool eventConsumed = false;
            
            using Type = Std::Trait::RemoveCVRef<decltype(event)>;
            if constexpr (Std::Trait::isSame<Type, CursorPressEvent>)
            {
                eventConsumed = node.widget->CursorPress(
                    ctx,
                    windowId,
                    nodeRect,
                    visibleRect,
                    { (i32)pointer.pos.x, (i32)pointer.pos.y },
                    event);
            }
            else if constexpr (Std::Trait::isSame<Type, TouchPressEvent>)
            {
                
                eventConsumed = node.widget->TouchPressEvent(
                    ctx,
                    windowId,
                    nodeRect,
                    visibleRect,
                    event);
            }

            if (eventConsumed && pointer.pressed)
                break;
        }

        return pointerInside;
    }
}

using namespace DEngine;
using namespace DEngine::Gui;

AnchorArea::AnchorArea()
{
}

namespace DEngine::Gui::impl
{
	struct AA_NodeEndIt
	{
	};

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
			Rect const& widgetRect) const
		{
			if (index == childCount)
				return widgetRect;
			else
				return impl::BuildNodeRect(anchorArea.nodes[index], widgetRect);
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
	struct AA_NodeIt
	{
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

SizeHint AnchorArea::GetSizeHint2(Widget::GetSizeHint2_Params const& params) const
{
	auto& pusher = params.pusher;

	for (auto const& node : impl::BuildNodeItPair(*this)) {
		auto const& child = node.child;
		child.GetSizeHint2(params);
	}

	SizeHint returnValue = {};
	returnValue.expandX = true;
	returnValue.expandY = true;
	returnValue.minimum = { 150, 150 };

	auto entry = pusher.AddEntry(*this);
	pusher.SetSizeHint(entry, returnValue);
	return returnValue;
}

void AnchorArea::BuildChildRects(
	Widget::BuildChildRects_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto& pusher = params.pusher;

	for (auto const& node : impl::BuildNodeItPair(*this))
	{
		auto const& child = node.child;
		auto const childRect = node.BuildNodeItRect(*this, widgetRect);

		auto childEntry = pusher.GetEntry(child);
		pusher.SetRectPair(childEntry, { childRect, visibleRect });
		child.BuildChildRects(
			params,
			childRect,
			visibleRect);
	}
}

void AnchorArea::Render2(
	Widget::Render_Params const& params,
	Rect const& widgetRect,
	Rect const& visibleRect) const
{
	auto const& rectCollection = params.rectCollection;

	for (auto const& node : impl::BuildNodeItPair(*this).Reverse())
	{
		auto const& child = node.child;
		auto const childRect = node.GetNodeItRect(*this, widgetRect, rectCollection);

		child.Render2(
			params,
			childRect,
			visibleRect);
	}
}

bool AnchorArea::CursorPress2(
	Widget::CursorPressParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool consumed)
{
	auto const& rectColl = params.rectCollection;

	bool newConsumed = consumed;

	for (auto node : impl::BuildNodeItPair(*this))
	{
		auto& child = node.child;
		auto const childRect = node.GetNodeItRect(*this, widgetRect, rectColl);

		bool childConsumed = child.CursorPress2(
			params,
			childRect,
			visibleRect,
			newConsumed);

		newConsumed = newConsumed || childConsumed;
	}

	return newConsumed;
}

bool AnchorArea::CursorMove(
	Widget::CursorMoveParams const& params,
	Rect const& widgetRect,
	Rect const& visibleRect,
	bool occluded)
{
	auto const& rectColl = params.rectCollection;

	bool newOccluded = occluded;

	for (auto node : impl::BuildNodeItPair(*this))
	{
		auto& child = node.child;
		auto const childRect = node.GetNodeItRect(*this, widgetRect, rectColl);

		bool childOccluded = child.CursorMove(
			params,
			childRect,
			visibleRect,
			newOccluded);
		newOccluded = newOccluded || childOccluded;
	}

	return newOccluded;
}
