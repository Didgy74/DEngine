#include <DEngine/Gui/AnchorArea.hpp>

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

        auto nodeRect = impl::GetNodeRect(
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
    for (uSize i = 0; i < nodes.size(); i += 1)
    {
        auto& node = nodes[i];
        DENGINE_IMPL_GUI_ASSERT(node.widget);

        auto nodeRect = impl::GetNodeRect(
            { nodes.data(), nodes.size() },
            widgetRect,
            i);

        node.widget->CursorMove(
            ctx,
            windowId,
            nodeRect,
            visibleRect,
            event,
            occluded);
    }

    return false;
}

bool AnchorArea::CursorPress(
    Context& ctx, 
    WindowID windowId, 
    Rect widgetRect, 
    Rect visibleRect, 
    Math::Vec2Int cursorPos, 
    CursorClickEvent event)
{
    bool cursorIsInside = widgetRect.PointIsInside(cursorPos) && visibleRect.PointIsInside(cursorPos);
    if (!cursorIsInside)
        return false;

    bool consumed = false;
    for (uSize i = 0; i < nodes.size(); i += 1)
    {
        auto& node = nodes[i];
        DENGINE_IMPL_GUI_ASSERT(node.widget);

        auto nodeRect = impl::GetNodeRect(
            { nodes.data(), nodes.size() },
            widgetRect,
            i);

        consumed = node.widget->CursorPress(
            ctx,
            windowId,
            nodeRect,
            visibleRect,
            cursorPos,
            event);
        if (consumed)
            break;
    }

    // If we have a background, we want to consume the event always.
    return consumed;
}

void AnchorArea::TouchEvent(
    Context& ctx, 
    WindowID windowId, 
    Rect widgetRect, 
    Rect visibleRect, 
    Gui::TouchEvent event,
    bool occluded)
{
}
