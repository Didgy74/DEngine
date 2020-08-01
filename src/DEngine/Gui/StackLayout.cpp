#include "DEngine/Gui/StackLayout.hpp"

#include "DEngine/Math/Common.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

// Callable needs to have signature void(Stack::LayoutItem const& child, Rect childRect)
template<typename T, typename Callable>
static void Test(
	T& layout,
	Rect widgetRect,
	Callable callable)
{
	// The internal extent of the parent widget with padding and spacing applied.
	Extent innerExtent = {
		widgetRect.extent.width - layout.padding * 2,
		widgetRect.extent.height - layout.padding * 2 };
	u32 const innerExtentDirection = (layout.direction == StackLayout::Direction::Horizontal ?
		innerExtent.width : 
		innerExtent.height) - (layout.spacing * ((u32)layout.children.size() - 1));

	Rect childRect = widgetRect;
	childRect.position.x += layout.padding;
	childRect.position.y += layout.padding;
	i32& childPosDirection = layout.direction == StackLayout::Direction::Horizontal ? childRect.position.x : childRect.position.y;

	// The last widget might not perfectly align with the total widget size,
// so we need the last one to use all the remaining space.
// This means the final widget will be at most 1 pixel too wide/narrow.
	u32 remainingSpace = innerExtentDirection;
	for (uSize i = 0; i < layout.children.size(); i++)
	{
		auto& child = layout.children[i];
		childRect.extent = innerExtent;
		u32& childExtentDirection = layout.direction == StackLayout::Direction::Horizontal ? childRect.extent.width : childRect.extent.height;
		// We check that we are not at the final item.
		if (i < layout.children.size() - 1)
			childExtentDirection = (u32)Math::Round(innerExtentDirection * child.relativeSize);
		else
			childExtentDirection = remainingSpace;
		remainingSpace -= childExtentDirection;

		callable(child, childRect);

		childPosDirection += childExtentDirection + layout.spacing;
	}
}

void StackLayout::Render(
	Context& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	DrawInfo& drawInfo) const
{
	// Only draw the quad of the layout if the alpha of the color is high enough to be visible.
	if (color.w > 0.01f)
	{
		Gfx::GuiDrawCmd cmd;
		cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
		cmd.filledMesh.color = color;
		cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
		cmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
		cmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
		cmd.rectPosition.x = (f32)widgetRect.extent.width / framebufferExtent.width;
		cmd.rectPosition.y = (f32)widgetRect.extent.height / framebufferExtent.height;
		drawInfo.drawCmds.push_back(cmd);
	}

	if (children.empty())
		return;

	Test(
		*this,
		widgetRect,
		[=, &ctx, &drawInfo](LayoutItem const& child, Rect childRect)
		{
			if (child.type == LayoutItem::ItemType::Layout)
				child.layout->Render(
					ctx,
					framebufferExtent,
					childRect,
					drawInfo);
			else
				child.widget->Render(
					ctx,
					framebufferExtent,
					childRect,
					drawInfo);
		});

	/*
	// The internal extent of the parent widget with padding and spacing applied.
	Extent innerExtent = {
		widgetRect.extent.width - padding * 2,
		widgetRect.extent.height - padding * 2 };
	u32 const innerExtentDirection = (direction == Direction::Horizontal ? innerExtent.width : innerExtent.height) - 
							   (spacing * ((u32)children.size() - 1));

	Rect childRect = widgetRect;
	childRect.position.x += padding;
	childRect.position.y += padding;
	i32& childPosDirection = direction == Direction::Horizontal ? childRect.position.x : childRect.position.y;


	// The last widget might not perfectly align with the total widget size,
	// so we need the last one to use all the remaining space.
	// This means the final widget will be at most 1 pixel too wide/narrow.
	u32 remainingSpace = innerExtentDirection;
	for (uSize i = 0; i < children.size(); i++)
	{
		auto const& child = children[i];
		childRect.extent = innerExtent;
		u32& childExtentDirection = direction == Direction::Horizontal ? childRect.extent.width : childRect.extent.height;
		// We check that we are not at the final item.
		if (i < children.size() - 1)
			childExtentDirection = (u32)Math::Round(innerExtentDirection * child.relativeSize);
		else
			childExtentDirection = remainingSpace;
		remainingSpace -= childExtentDirection;

		if (child.type == LayoutItem::ItemType::Layout)
			child.layout->Render(
				ctx,
				framebufferExtent,
				childRect,
				drawInfo);
		else
			child.widget->Render(
				ctx,
				framebufferExtent,
				childRect,
				drawInfo);

		childPosDirection += childExtentDirection + spacing;
	}
	*/
}

void StackLayout::SetChildRelativeSize(u32 index, f32 input, bool locked)
{
	DENGINE_DETAIL_ASSERT(index < children.size());
	auto& targetChild = children[index];

	f32 sumLockedSizes = 0.f;
	uSize unlockedChildrenCount = children.size();
	for (LayoutItem& child : children)
	{
		if (child.sizeLocked)
		{
			sumLockedSizes += child.relativeSize;
			unlockedChildrenCount -= 1;
		}
	}

	f32 previousAvailableSize = 1.f - sumLockedSizes - targetChild.relativeSize;
	f32 childSizeDifference = input - targetChild.relativeSize;
	f32 newAvailableSize = previousAvailableSize - childSizeDifference;
	f32 availableSizeFactor = newAvailableSize / previousAvailableSize;

	for (LayoutItem& child : children)
	{
		if (!child.sizeLocked)
			child.relativeSize *= availableSizeFactor;
	}
	
	targetChild.relativeSize = input;
	targetChild.sizeLocked = locked;
}

void StackLayout::AddWidget2(Std::Box<Widget>&& in)
{
	// Check if Box is not nullptr, then crash.

	f32 sumLockedSizes = 0.f;
	// We do +1 to account for the child we're about to insert.
	uSize unlockedChildrenCount = children.size() + 1;
	for (LayoutItem& child : children)
	{
		if (child.sizeLocked)
		{
			sumLockedSizes += child.relativeSize;
			unlockedChildrenCount -= 1;
		}
	}

	f32 previousAvailableSize = 1.f - sumLockedSizes;
	f32 newChildSize = previousAvailableSize / unlockedChildrenCount;
	f32 newAvailableSize = previousAvailableSize - newChildSize;
	f32 availableSizeFactor = newAvailableSize / previousAvailableSize;

	for (LayoutItem& child : children)
	{
		if (!child.sizeLocked)
			child.relativeSize *= availableSizeFactor;
	}

	LayoutItem newItem;
	newItem.type = LayoutItem::ItemType::Widget;
	newItem.widget = static_cast<Std::Box<Widget>&&>(in);
	newItem.sizeLocked = false;
	newItem.relativeSize = newChildSize;
	children.emplace_back(static_cast<LayoutItem&&>(newItem));
}

void StackLayout::AddLayout2(Std::Box<Layout>&& newLayout)
{
	f32 sumLockedSizes = 0.f;
	// We do +1 to account for the child we're about to insert.
	uSize unlockedChildrenCount = children.size() + 1;
	for (LayoutItem& child : children)
	{
		if (child.sizeLocked)
		{
			sumLockedSizes += child.relativeSize;
			unlockedChildrenCount -= 1;
		}
	}

	f32 previousAvailableSize = 1.f - sumLockedSizes;
	f32 newChildSize = previousAvailableSize / unlockedChildrenCount;
	f32 newAvailableSize = previousAvailableSize - newChildSize;
	f32 availableSizeFactor = newAvailableSize / previousAvailableSize;

	for (LayoutItem& child : children)
	{
		if (!child.sizeLocked)
			child.relativeSize *= availableSizeFactor;
	}

	LayoutItem newItem;
	newItem.type = LayoutItem::ItemType::Layout;
	newItem.layout = static_cast<Std::Box<Layout>&&>(newLayout);
	newItem.sizeLocked = false;
	newItem.relativeSize = newChildSize;
	children.emplace_back(static_cast<LayoutItem&&>(newItem));
}

void StackLayout::InsertLayout(u32 index, Std::Box<Layout>&& newLayout)
{
	f32 sumLockedSizes = 0.f;
	// We do +1 to account for the child we're about to insert.
	uSize unlockedChildrenCount = children.size() + 1;
	for (LayoutItem& child : children)
	{
		if (child.sizeLocked)
		{
			sumLockedSizes += child.relativeSize;
			unlockedChildrenCount -= 1;
		}
	}

	f32 previousAvailableSize = 1.f - sumLockedSizes;
	f32 newChildSize = previousAvailableSize / unlockedChildrenCount;
	f32 newAvailableSize = previousAvailableSize - newChildSize;
	f32 availableSizeFactor = newAvailableSize / previousAvailableSize;

	for (LayoutItem& child : children)
	{
		if (!child.sizeLocked)
			child.relativeSize *= availableSizeFactor;
	}

	LayoutItem newItem;
	newItem.type = LayoutItem::ItemType::Layout;
	newItem.layout = static_cast<Std::Box<Layout>&&>(newLayout);
	newItem.sizeLocked = false;
	newItem.relativeSize = newChildSize;
	children.insert(children.begin() + index, static_cast<LayoutItem&&>(newItem));
}

void StackLayout::RemoveItem(u32 index)
{
	// Assert index < children.size()

	LayoutItem& item = children[index];

	f32 sumLockedSizes = 0.f;
	uSize unlockedChildrenCount = children.size();
	for (LayoutItem& child : children)
	{
		if (child.sizeLocked)
		{
			sumLockedSizes += child.relativeSize;
			unlockedChildrenCount -= 1;
		}
	}

	f32 previousAvailableSize = 1.f - sumLockedSizes - item.relativeSize;
	f32 newAvailableSize = previousAvailableSize + item.relativeSize;
	f32 availableSizeFactor = newAvailableSize / previousAvailableSize;
	for (LayoutItem& child : children)
	{
		if (!child.sizeLocked)
			child.relativeSize *= availableSizeFactor;
	}

	children.erase(children.begin() + index);
}

void StackLayout::ClearChildren()
{
	children.clear();
}

void StackLayout::CharEvent(Context& ctx, u32 utfValue)
{
	for (auto& child : children)
	{
		if (child.type == LayoutItem::ItemType::Layout)
		{
			child.layout->CharEvent(ctx, utfValue);
		}
		else
		{
			child.widget->CharEvent(ctx, utfValue);
		}
	}
}

void StackLayout::CharRemoveEvent(Context& ctx)
{
	for (auto& child : children)
	{
		if (child.type == LayoutItem::ItemType::Layout)
		{
			child.layout->CharRemoveEvent(ctx);
		}
		else
		{
			child.widget->CharRemoveEvent(ctx);
		}
	}
}

void StackLayout::CursorMove(
	Rect widgetRect,
	CursorMoveEvent event)
{
	if (children.empty())
		return;

	Test(
		*this,
		widgetRect,
		[=](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::ItemType::Layout)
				child.layout->CursorMove(
					childRect,
					event);
			else
				child.widget->CursorMove(
					childRect,
					event);
		});

	/*
	// The internal extent of the parent widget with padding and spacing applied.
	Extent innerExtent = {
		widgetRect.extent.width - padding * 2,
		widgetRect.extent.height - padding * 2 };
	u32 const innerExtentDirection = (direction == Direction::Horizontal ? innerExtent.width : innerExtent.height) -
		(spacing * ((u32)children.size() - 1));

	Rect childRect = widgetRect;
	childRect.position.x += padding;
	childRect.position.y += padding;
	i32& childPosDirection = direction == Direction::Horizontal ? childRect.position.x : childRect.position.y;

	// The last widget might not perfectly align with the total widget size,
	// so we need the last one to use all the remaining space.
	// This means the final widget will be at most 1 pixel too wide/narrow.
	u32 remainingSpace = innerExtentDirection;
	for (uSize i = 0; i < children.size(); i++)
	{
		auto const& child = children[i];
		childRect.extent = innerExtent;
		u32& childExtentDirection = direction == Direction::Horizontal ? childRect.extent.width : childRect.extent.height;
		// We check that we are not at the final item.
		if (i < children.size() - 1)
			childExtentDirection = (u32)Math::Round(innerExtentDirection * child.relativeSize);
		else
			childExtentDirection = remainingSpace;
		remainingSpace -= childExtentDirection;

		if (child.type == LayoutItem::ItemType::Layout)
			child.layout->CursorMove(
				childRect,
				event);
		else if (child.type == LayoutItem::ItemType::Widget)
			child.widget->CursorMove(
				childRect,
				event);

		childPosDirection += childExtentDirection + spacing;
	}
	*/
}

void StackLayout::CursorClick(
	Rect widgetRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	if (children.empty())
		return;

	Test(
		*this,
		widgetRect,
		[=](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::ItemType::Layout)
				child.layout->CursorClick(
					childRect,
					cursorPos,
					event);
			else
				child.widget->CursorClick(
					childRect,
					cursorPos,
					event);
		});

	/*

	// The internal extent of the parent widget with padding and spacing applied.
	Extent innerExtent = {
		widgetRect.extent.width - padding * 2,
		widgetRect.extent.height - padding * 2 };
	u32 const innerExtentDirection = (direction == Direction::Horizontal ? innerExtent.width : innerExtent.height) -
		(spacing * ((u32)children.size() - 1));

	Rect childRect = widgetRect;
	childRect.position.x += padding;
	childRect.position.y += padding;
	i32& childPosDirection = direction == Direction::Horizontal ? childRect.position.x : childRect.position.y;

	// The last widget might not perfectly align with the total widget size,
	// so we need the last one to use all the remaining space.
	// This means the final widget will be at most 1 pixel too wide/narrow.
	u32 remainingSpace = innerExtentDirection;
	for (uSize i = 0; i < children.size(); i++)
	{
		auto const& child = children[i];
		childRect.extent = innerExtent;
		u32& childExtentDirection = direction == Direction::Horizontal ? childRect.extent.width : childRect.extent.height;
		// We check that we are not at the final item.
		if (i < children.size() - 1)
			childExtentDirection = (u32)Math::Round(innerExtentDirection * child.relativeSize);
		else
			childExtentDirection = remainingSpace;
		remainingSpace -= childExtentDirection;

		if (child.type == LayoutItem::ItemType::Layout)
			child.layout->CursorClick(
				childRect,
				cursorPos,
				event);
		else if (child.type == LayoutItem::ItemType::Widget)
			child.widget->CursorClick(
				childRect,
				cursorPos,
				event);

		childPosDirection += childExtentDirection + spacing;
	}
	*/
}

void StackLayout::TouchEvent(
	Rect widgetRect,
	Gui::TouchEvent touch)
{
	if (children.empty())
		return;

	Test(
		*this,
		widgetRect,
		[=](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::ItemType::Layout)
				child.layout->TouchEvent(
					childRect,
					touch);
			else
				child.widget->TouchEvent(
					childRect,
					touch);
		});

	/*
	// The internal extent of the parent widget with padding and spacing applied.
	Extent innerExtent = {
		widgetRect.extent.width - padding * 2,
		widgetRect.extent.height - padding * 2 };
	u32 const innerExtentDirection = (direction == Direction::Horizontal ? innerExtent.width : innerExtent.height) -
		(spacing * ((u32)children.size() - 1));

	Rect childRect = widgetRect;
	childRect.position.x += padding;
	childRect.position.y += padding;
	i32& childPosDirection = direction == Direction::Horizontal ? childRect.position.x : childRect.position.y;

	// The last widget might not perfectly align with the total widget size,
	// so we need the last one to use all the remaining space.
	// This means the final widget will be at most 1 pixel too wide/narrow.
	u32 remainingSpace = innerExtentDirection;
	for (uSize i = 0; i < children.size(); i++)
	{
		auto const& child = children[i];
		childRect.extent = innerExtent;
		u32& childExtentDirection = direction == Direction::Horizontal ? childRect.extent.width : childRect.extent.height;
		// We check that we are not at the final item.
		if (i < children.size() - 1)
			childExtentDirection = (u32)Math::Round(innerExtentDirection * child.relativeSize);
		else
			childExtentDirection = remainingSpace;
		remainingSpace -= childExtentDirection;

		if (child.type == LayoutItem::ItemType::Layout)
			child.layout->TouchEvent(
				childRect,
				touch);
		else if (child.type == LayoutItem::ItemType::Widget)
			child.widget->TouchEvent(
				childRect,
				touch);

		childPosDirection += childExtentDirection + spacing;
	}
	*/
}

void StackLayout::Tick(
	Context& ctx,
	Rect widgetRect)
{
	if (children.empty())
		return;

	Test(
		*this,
		widgetRect,
		[=, &ctx](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::ItemType::Layout)
				child.layout->Tick(
					ctx,
					childRect);
			else
				child.widget->Tick(
					ctx,
					childRect);
		});

	/*
	// The internal extent of the parent widget with padding and spacing applied.
	Extent innerExtent = {
		widgetRect.extent.width - padding * 2,
		widgetRect.extent.height - padding * 2 };
	u32 const innerExtentDirection = (direction == Direction::Horizontal ? innerExtent.width : innerExtent.height) -
		(spacing * ((u32)children.size() - 1));

	Rect childRect = widgetRect;
	childRect.position.x += padding;
	childRect.position.y += padding;
	i32& childPosDirection = direction == Direction::Horizontal ? childRect.position.x : childRect.position.y;

	// The last widget might not perfectly align with the total widget size,
	// so we need the last one to use all the remaining space.
	// This means the final widget will be at most 1 pixel too wide/narrow.
	u32 remainingSpace = innerExtentDirection;
	for (uSize i = 0; i < children.size(); i++)
	{
		auto const& child = children[i];
		childRect.extent = innerExtent;
		u32& childExtentDirection = direction == Direction::Horizontal ? childRect.extent.width : childRect.extent.height;
		// We check that we are not at the final item.
		if (i < children.size() - 1)
			childExtentDirection = (u32)Math::Round(innerExtentDirection * child.relativeSize);
		else
			childExtentDirection = remainingSpace;
		remainingSpace -= childExtentDirection;

		if (child.type == LayoutItem::ItemType::Layout)
			child.layout->Tick(
				ctx,
				childRect);
		else if (child.type == LayoutItem::ItemType::Widget)
			child.widget->Tick(
				ctx,
				childRect);

		childPosDirection += childExtentDirection + spacing;
	}
	*/
}
