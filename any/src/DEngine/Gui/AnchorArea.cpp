#include <DEngine/Gui/AnchorArea.hpp>

#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Trait.hpp>

namespace DEngine::Gui::impl
{
    [[nodiscard]] static Rect GetNodeRect(
        Std::Span<AnchorArea::Node const> nodes,
        Rect widgetRect,
        uSize index) noexcept
    {
        DENGINE_IMPL_GUI_ASSERT(index < nodes.Size());
        Rect nodeRect = widgetRect;

        if (index != nodes.Size() - 1)
        {
            auto const& node = nodes[index];
            nodeRect.extent = node.extent;
            nodeRect.position = widgetRect.position;
            switch (node.anchorX)
            {
            case AnchorArea::AnchorX::Left:
                break;
            case AnchorArea::AnchorX::Center:
                nodeRect.position.x += widgetRect.extent.width / 2;
                nodeRect.position.x -= nodeRect.extent.width / 2;
                break;
            case AnchorArea::AnchorX::Right:
                nodeRect.position.x += widgetRect.extent.width;
                nodeRect.position.x -= nodeRect.extent.width;
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
                nodeRect.position.y += widgetRect.extent.height / 2;
                nodeRect.position.y -= nodeRect.extent.height / 2;
                break;
            case AnchorArea::AnchorY::Bottom:
                nodeRect.position.y += widgetRect.extent.height;
                nodeRect.position.y -= nodeRect.extent.height;
                break;
            default:
                DENGINE_IMPL_UNREACHABLE();
                break;
            }
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

            auto const nodeRect = impl::GetNodeRect(
                { widget.nodes.data(), widget.nodes.size() },
                widgetRect,
                i);

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

            auto const nodeRect = impl::GetNodeRect(
                { widget.nodes.data(), widget.nodes.size() },
                widgetRect,
                i);

            bool eventConsumed = false;
            
            using Type = Std::Trait::RemoveCVRef<decltype(event)>;
            if constexpr (Std::Trait::isSame<Type, CursorClickEvent>)
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

SizeHint AnchorArea::GetSizeHint(Context const& ctx) const
{
    SizeHint returnVal = {};
    returnVal.expandX = true;
    returnVal.expandY = true;
    returnVal.preferred = { 150, 150 };
    return returnVal;
}

void AnchorArea::Render(
    Context const& ctx, 
    Extent framebufferExtent, 
    Rect widgetRect, 
    Rect visibleRect,
    DrawInfo& drawInfo) const
{
    // Iterate in reverse
    for (uSize j = nodes.size(); j > 0; j -= 1)
    {
        uSize i = j - 1;

        auto const& node = nodes[i];
        DENGINE_IMPL_GUI_ASSERT(node.widget);

        auto const nodeRect = impl::GetNodeRect(
            { nodes.data(), nodes.size() },
            widgetRect,
            i);

        node.widget->Render(
            ctx,
            framebufferExtent,
            nodeRect,
            visibleRect,
            drawInfo);
    }
}

void AnchorArea::CharEnterEvent(Context& ctx)
{
}

void AnchorArea::CharEvent(
    Context& ctx,
    u32 utfValue)
{
}

void AnchorArea::CharRemoveEvent(Context& ctx)
{
}

void AnchorArea::InputConnectionLost()
{
}

bool AnchorArea::CursorMove(
    Context& ctx, 
    WindowID windowId, 
    Rect widgetRect, 
    Rect visibleRect, 
    CursorMoveEvent event, 
    bool occluded)
{
    impl::PointerMove_Pointer pointer = {};
    pointer.occluded = occluded;
    pointer.pos = { (f32)event.position.x, (f32)event.position.y };
    return impl::PointerMove(
        *this,
        ctx,
        windowId,
        widgetRect,
        visibleRect,
        pointer,
        event);
}

bool AnchorArea::CursorPress(
    Context& ctx, 
    WindowID windowId, 
    Rect widgetRect, 
    Rect visibleRect, 
    Math::Vec2Int cursorPos, 
    CursorClickEvent event)
{
    impl::PointerPress_Pointer pointer = {};
    pointer.pos = { (f32)cursorPos.x, (f32)cursorPos.y };
    pointer.pressed = event.clicked;
    return impl::PointerPress(
        *this,
        ctx,
        windowId,
        widgetRect,
        visibleRect,
        pointer,
        event);
}

bool AnchorArea::TouchMoveEvent(
    Context& ctx,
    WindowID windowId,
    Rect widgetRect,
    Rect visibleRect,
    Gui::TouchMoveEvent event,
    bool occluded)
{
    impl::PointerMove_Pointer pointer = {};
    pointer.occluded = occluded;
    pointer.pos = event.position;
    return impl::PointerMove(
        *this,
        ctx,
        windowId,
        widgetRect,
        visibleRect,
        pointer,
        event);
}

bool AnchorArea::TouchPressEvent(
    Context& ctx,
    WindowID windowId,
    Rect widgetRect,
    Rect visibleRect,
    Gui::TouchPressEvent event)
{
    impl::PointerPress_Pointer pointer = {};
    pointer.pos = event.position;
    pointer.pressed = event.pressed;
    return impl::PointerPress(
        *this,
        ctx,
        windowId,
        widgetRect,
        visibleRect,
        pointer,
        event);
}
