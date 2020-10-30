#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/impl/Assert.hpp>

#include <DEngine/Math/Common.hpp>

namespace DEngine::Gui::impl
{
	template<typename T, typename Callable>
	void StackLayout_IterateOverChildren(
		Context const& ctx,
		T& layout,
		Rect widgetRect,
		Callable const& callable)
	{
		static_assert(Std::Trait::IsSame<decltype(layout), StackLayout&> || Std::Trait::IsSame<decltype(layout), StackLayout const&>);
		
		uSize childCount = (uSize)layout.children.size();
		if (childCount == 0)
			return;

		std::vector<SizeHint> childSizeHints(childCount);

		// The internal extent of the parent widget with padding and spacing applied.
		Extent const innerExtent = {
			widgetRect.extent.width - layout.padding * 2,
			widgetRect.extent.height - layout.padding * 2 };
		
		// Go over each cilds size hint
		// Find the biggest size in non-direction
		// Find the sum of sizes in direction
		Extent sumChildPreferredSize{};
		
		for (uSize i = 0; i < childCount; i += 1)
		{
			auto& child = layout.children[i];
			auto& childSizeHint = childSizeHints[i];
			// Call SizeHint on child.
			if (child.type == StackLayout::LayoutItem::Type::Layout)
				childSizeHint = child.layout->GetSizeHint(ctx);
			else
				childSizeHint = child.widget->GetSizeHint(ctx);

			// Add to the sum size.
			if (!childSizeHint.expandX)
			{
				u32& directionLength = layout.direction == StackLayout::Direction::Horizontal ? 
					sumChildPreferredSize.width : 
					sumChildPreferredSize.height;
				u32 const& childDirectionLength = layout.direction == StackLayout::Direction::Horizontal ? 
					childSizeHint.preferred.width : 
					childSizeHint.preferred.height;
				directionLength += childDirectionLength;

				u32& nonDirectionLength = layout.direction == StackLayout::Direction::Vertical ? sumChildPreferredSize.width : sumChildPreferredSize.height;
				u32 const& childNonDirectionLength = layout.direction == StackLayout::Direction::Vertical ? 
					childSizeHint.preferred.width : 
					childSizeHint.preferred.height;
				nonDirectionLength = Math::Max(nonDirectionLength, childNonDirectionLength);
			}
		}

		i32 remainingDirectionSpace = 0;
		if (layout.direction == StackLayout::Direction::Horizontal)
			remainingDirectionSpace = innerExtent.width - sumChildPreferredSize.width;
		else
			remainingDirectionSpace = innerExtent.height - sumChildPreferredSize.height;

		// Set the StackLayout's internal state to currently iterating
		layout.currentlyIterating = true;

		Rect childRect{};
		childRect.position.x += widgetRect.position.x + layout.padding;
		childRect.position.y += widgetRect.position.y + layout.padding;
		if (layout.direction == StackLayout::Direction::Horizontal)
			childRect.extent.height = innerExtent.height;
		else
			childRect.extent.width = innerExtent.width;
		for (uSize i = 0; i < childCount; i++)
		{
			SizeHint const childSizeHint = childSizeHints[i];
			if (childSizeHint.expandX)
			{
				if (layout.direction == StackLayout::Direction::Horizontal)
					childRect.extent.width = remainingDirectionSpace;
				else
					childRect.extent.height = remainingDirectionSpace;
			}
			else
			{
				if (layout.direction == StackLayout::Direction::Horizontal)
					childRect.extent.width = childSizeHint.preferred.width;
				else
					childRect.extent.height = childSizeHint.preferred.height;
			}

			// Translate the i in the SizeHint array back to the Children array's corresponding element.
			// Note! This code is likely bugged in some cases still. Prepare to fix.
			uSize modifiedIndex = i;
			bool childExists = true;
			for (StackLayout::InsertRemoveJob job : layout.insertionJobs)
			{
				switch (job.type)
				{
				case StackLayout::InsertRemoveJob::Type::Insert:
					if (job.insert.index <= modifiedIndex)
						modifiedIndex += 1;
					break;
				case StackLayout::InsertRemoveJob::Type::Remove:
					if (job.remove.index == modifiedIndex)
						childExists = false;
					else if (job.remove.index < modifiedIndex)
						modifiedIndex -= 1;
					break;
				}
			}
			if (childExists)
			{
				auto& child = layout.children[modifiedIndex];
				if (child.widget || child.layout)
					callable(child, childRect);
			}


			i32& childPosDirection = layout.direction == StackLayout::Direction::Horizontal ? childRect.position.x : childRect.position.y;
			u32 const& childExtentDirection = layout.direction == StackLayout::Direction::Horizontal ? childRect.extent.width : childRect.extent.height;
			childPosDirection += childExtentDirection + layout.spacing;
		}

		// Reset the "changed indices stuff"
		layout.currentlyIterating = false;
		layout.insertionJobs.clear();
	}

}

using namespace DEngine;
using namespace DEngine::Gui;

StackLayout::~StackLayout()
{
}

uSize StackLayout::ChildCount() const
{
	return (uSize)children.size();
}

StackLayout::Child StackLayout::At(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto& child = children[index];
	
	DENGINE_IMPL_GUI_ASSERT(child.layout || child.widget);

	Child returnVal{};
	if (child.type == LayoutItem::Type::Layout)
	{
		returnVal.type = Child::Type::Layout;
		returnVal.layout = child.layout.Get();
	}
	else
	{
		returnVal.type = Child::Type::Widget;
		returnVal.widget = child.widget.Get();
	}

	return returnVal;
}

StackLayout::ConstChild StackLayout::At(uSize index) const
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());

	auto& child = children[index];

	DENGINE_IMPL_GUI_ASSERT(child.layout || child.widget);

	ConstChild returnVal{};
	if (child.type == LayoutItem::Type::Layout)
	{
		returnVal.type = ConstChild::Type::Layout;
		returnVal.layout = child.layout.Get();
	}
	else
	{
		returnVal.type = ConstChild::Type::Widget;
		returnVal.widget = child.widget.Get();
	}
	return returnVal;
}

void StackLayout::AddWidget2(Std::Box<Widget>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(in);

	LayoutItem newItem{};
	newItem.type = LayoutItem::Type::Widget;
	newItem.widget = static_cast<Std::Box<Widget>&&>(in);
	children.emplace_back(static_cast<LayoutItem&&>(newItem));

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Insert;
		newJob.insert.index = InsertRemoveJob::Insert::pushBackIndex;
		insertionJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
	}
}

void StackLayout::AddLayout2(Std::Box<Layout>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(in);

	LayoutItem newItem{};
	newItem.type = LayoutItem::Type::Layout;
	newItem.layout = static_cast<Std::Box<Layout>&&>(in);
	children.emplace_back(static_cast<LayoutItem&&>(newItem));

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Insert;
		newJob.insert.index = InsertRemoveJob::Insert::pushBackIndex;
		insertionJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
	}
}

void StackLayout::InsertLayout(uSize index, Std::Box<Layout>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	DENGINE_IMPL_GUI_ASSERT(in);

	LayoutItem newItem{};
	newItem.type = LayoutItem::Type::Layout;
	newItem.layout = static_cast<Std::Box<Layout>&&>(in);
	children.insert(children.begin() + index, static_cast<LayoutItem&&>(newItem));

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Insert;
		newJob.insert.index = index;
		insertionJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
	}
}

void StackLayout::RemoveItem(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());

	children.erase(children.begin() + index);

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Remove;
		newJob.remove.index = index;
		insertionJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
	}
}

void StackLayout::ClearChildren()
{
	children.clear();
}

SizeHint StackLayout::GetSizeHint(
	Context const& ctx) const
{
	SizeHint returnVal{};
	u32 childCount = 0;
	for (auto const& child : children)
	{
		if (!child.widget && !child.layout)
			continue;

		childCount += 1;

		SizeHint childSizeHint{};
		if (child.type == LayoutItem::Type::Layout)
			childSizeHint = child.layout->GetSizeHint(ctx);
		else
			childSizeHint = child.widget->GetSizeHint(ctx);

		u32& directionLength = direction == Direction::Horizontal ? returnVal.preferred.width : returnVal.preferred.height;
		u32& childDirectionLength = direction == Direction::Horizontal ? childSizeHint.preferred.width : childSizeHint.preferred.height;
		directionLength += childDirectionLength;

		u32& nonDirectionLength = direction == Direction::Horizontal ? returnVal.preferred.height : returnVal.preferred.width;
		u32& childNonDirectionLength = direction == Direction::Horizontal ? childSizeHint.preferred.height : childSizeHint.preferred.width;
		nonDirectionLength = Math::Max(nonDirectionLength, childNonDirectionLength);
	}
	// Add spacing
	if (childCount > 1)
	{
		if (direction == Direction::Horizontal)
		{
			returnVal.preferred.width += spacing * (childCount - 1);
		}
		else
		{
			returnVal.preferred.height += spacing * (childCount - 1);
		}
	}

	if (test_expand)
	{
		returnVal.expandX = true;
		returnVal.expandY = true;
	}
		
	// Add padding
	returnVal.preferred.width += padding * 2;
	returnVal.preferred.height += padding * 2;

	return returnVal;
}

void StackLayout::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	ParentType::Render(
		ctx,
		framebufferExtent,
		widgetRect,
		visibleRect,
		drawInfo);

	// Only draw the quad of the layout if the alpha of the color is high enough to be visible.
	if (color.w > 0.01f)
	{
		Gfx::GuiDrawCmd cmd{};
		cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
		cmd.filledMesh.color = color;
		cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
		cmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
		cmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
		cmd.rectExtent.x = (f32)widgetRect.extent.width / framebufferExtent.width;
		cmd.rectExtent.y = (f32)widgetRect.extent.height / framebufferExtent.height;
		drawInfo.drawCmds.push_back(cmd);
	}

	impl::StackLayout_IterateOverChildren(
		ctx,
		*this,
		widgetRect,
		[&ctx, framebufferExtent, &drawInfo, visibleRect](LayoutItem const& child, Rect childRect)
		{
			Rect childVisibleRect = Rect::Intersection(childRect, visibleRect);
			if (childVisibleRect.IsNothing())
				return;

			if (child.type == LayoutItem::Type::Layout)
				child.layout->Render(
					ctx,
					framebufferExtent,
					childRect,
					childVisibleRect,
					drawInfo);
			else
				child.widget->Render(
					ctx,
					framebufferExtent,
					childRect,
					childVisibleRect,
					drawInfo);
		});
}

void StackLayout::CharEnterEvent(Context& ctx)
{
	ParentType::CharEnterEvent(ctx);

	for (auto& child : children)
	{
		if (child.type == LayoutItem::Type::Layout)
		{
			child.layout->CharEnterEvent(ctx);
		}
		else
		{
			child.widget->CharEnterEvent(ctx);
		}
	}
}

void StackLayout::CharEvent(
	Context& ctx, 
	u32 utfValue)
{
	ParentType::CharEvent(
		ctx,
		utfValue);

	for (auto& child : children)
	{
		if (child.type == LayoutItem::Type::Layout)
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
	ParentType::CharRemoveEvent(ctx);

	for (auto& child : children)
	{
		if (child.type == LayoutItem::Type::Layout)
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
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	ParentType::CursorMove(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event);

	impl::StackLayout_IterateOverChildren(
		ctx,
		*this,
		widgetRect,
		[&ctx, windowId, visibleRect, event](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::Type::Layout)
				child.layout->CursorMove(
					ctx,
					windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
			else
				child.widget->CursorMove(
					ctx,
					windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
		});
}

void StackLayout::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	ParentType::CursorClick(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		cursorPos,
		event);

	impl::StackLayout_IterateOverChildren(
		ctx,
		*this,
		widgetRect,
		[&ctx, windowId, visibleRect, cursorPos, event](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::Type::Layout)
				child.layout->CursorClick(
					ctx,
					windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					cursorPos,
					event);
			else
				child.widget->CursorClick(
					ctx,
					windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					cursorPos,
					event);
		});
}

void StackLayout::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event)
{
	ParentType::TouchEvent(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event);

	impl::StackLayout_IterateOverChildren(
		ctx,
		*this,
		widgetRect,
		[&ctx, windowId, visibleRect, event](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::Type::Layout)
				child.layout->TouchEvent(
					ctx,
					windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
			else
				child.widget->TouchEvent(
					ctx,
					windowId,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
		});
}
