#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/impl/Assert.hpp>

#include <DEngine/Math/Common.hpp>

namespace DEngine::Gui
{
	template<bool tick, typename T, typename Callable>
	void IterateOverChildren(
		Context const& ctx,
		T& layout,
		Rect widgetRect,
		Callable const& callable)
	{
		static_assert(Std::Trait::IsSame<decltype(layout), StackLayout&> || Std::Trait::IsSame<decltype(layout), StackLayout const&>);
		
		u32 childCount = layout.children.size();;
		if (childCount == 0)
			return;

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
			auto& childSizeHint = child.lastKnownSizeHint;
			if constexpr (tick)
			{
				if (child.type == StackLayout::LayoutItem::Type::Layout)
					childSizeHint = child.layout->SizeHint_Tick(ctx);
				else
					childSizeHint = child.widget->SizeHint_Tick(ctx);
			}
			else
			{
				if (child.type == StackLayout::LayoutItem::Type::Layout)
					childSizeHint = child.layout->SizeHint(ctx);
				else
					childSizeHint = child.widget->SizeHint(ctx);
			}


			if (!childSizeHint.expand)
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
		{
			remainingDirectionSpace = innerExtent.width - sumChildPreferredSize.width;
		}
		else
		{
			remainingDirectionSpace = innerExtent.height - sumChildPreferredSize.height;
		}

		Rect childRect{};
		childRect.position.x += widgetRect.position.x + layout.padding;
		childRect.position.y += widgetRect.position.y + layout.padding;
		if (layout.direction == StackLayout::Direction::Horizontal)
			childRect.extent.height = innerExtent.height;
		else
			childRect.extent.width = innerExtent.width;
		for (uSize i = 0; i < childCount; i++)
		{
			SizeHint childSizeHint = layout.children[i].lastKnownSizeHint;
			if (childSizeHint.expand)
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

			auto& child = layout.children[i];
			if (child.widget || child.layout)
				callable(child, childRect);

			i32& childPosDirection = layout.direction == StackLayout::Direction::Horizontal ? childRect.position.x : childRect.position.y;
			u32 const& childExtentDirection = layout.direction == StackLayout::Direction::Horizontal ? childRect.extent.width : childRect.extent.height;
			childPosDirection += childExtentDirection + layout.spacing;
		}
	}

}

using namespace DEngine;
using namespace DEngine::Gui;

StackLayout::~StackLayout()
{
}

u32 StackLayout::ChildCount() const
{
	uSize count = children.size();
	for (auto const& child : children)
	{
		if (!child.layout && !child.widget)
			count -= 1;
	}
	return (u32)count;
}

StackLayout::Test StackLayout::At(u32 index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto& child = children[index];
	
	DENGINE_IMPL_GUI_ASSERT(child.layout || child.widget);

	Test returnVal{};
	if (child.type == LayoutItem::Type::Layout)
	{
		returnVal.type = Test::Type::Layout;
		returnVal.layout = child.layout.Get();
	}
	else
	{
		returnVal.type = Test::Type::Widget;
		returnVal.widget = child.widget.Get();
	}

	return returnVal;
}

StackLayout::Test2 StackLayout::At(u32 index) const
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());

	auto& child = children[index];

	DENGINE_IMPL_GUI_ASSERT(child.layout || child.widget);

	Test2 returnVal{};
	if (child.type == LayoutItem::Type::Layout)
	{
		returnVal.type = Test2::Type::Layout;
		returnVal.layout = child.layout.Get();
	}
	else
	{
		returnVal.type = Test2::Type::Widget;
		returnVal.widget = child.widget.Get();
	}
	return returnVal;
}

void StackLayout::AddWidget2(Std::Box<Widget>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(in);

	InsertRemoveJob newJob{};
	newJob.type = InsertRemoveJob::Type::Insert;
	newJob.insert.index = InsertRemoveJob::Insert::pushBackIndex;
	newJob.insert.item.type = LayoutItem::Type::Widget;
	newJob.insert.item.widget = static_cast<Std::Box<Widget>&&>(in);

	addWidgetJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
}

void StackLayout::AddLayout2(Std::Box<Layout>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(in);

	InsertRemoveJob newJob{};
	newJob.type = InsertRemoveJob::Type::Insert;
	newJob.insert.index = InsertRemoveJob::Insert::pushBackIndex;
	newJob.insert.item.type = LayoutItem::Type::Layout;
	newJob.insert.item.layout = static_cast<Std::Box<Layout>&&>(in);
	addWidgetJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
}

void StackLayout::InsertLayout(u32 index, Std::Box<Layout>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(index < children.size());
	DENGINE_IMPL_GUI_ASSERT(in);

	InsertRemoveJob newJob{};
	newJob.type = InsertRemoveJob::Type::Insert;
	newJob.insert.index = index;
	newJob.insert.item.type = LayoutItem::Type::Layout;
	newJob.insert.item.layout = static_cast<Std::Box<Layout>&&>(in);
	addWidgetJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
}

void StackLayout::RemoveItem(u32 index)
{
	DENGINE_IMPL_GUI_ASSERT(index < children.size());

	children[index].layout = {};
	children[index].widget = {};

	InsertRemoveJob newJob{};
	newJob.type = InsertRemoveJob::Type::Remove;
	newJob.remove.index = index;
	addWidgetJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
}

void StackLayout::ClearChildren()
{
	children.clear();
}

SizeHint StackLayout::SizeHint(
	Context const& ctx) const
{
	Gui::SizeHint returnVal{};
	u32 childCount = 0;
	for (auto const& child : children)
	{
		if (!child.widget && !child.layout)
			continue;

		childCount += 1;

		Gui::SizeHint childSizeHint{};
		if (child.type == LayoutItem::Type::Layout)
			childSizeHint = child.layout->SizeHint(ctx);
		else
			childSizeHint = child.widget->SizeHint(ctx);

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
		returnVal.expand = true;

	return returnVal;
}

SizeHint StackLayout::SizeHint_Tick(Context const& ctx)
{
	ResolveWidgetAddRemoves();
	Gui::SizeHint returnVal{};
	u32 childCount = 0;
	for (auto& child : children)
	{
		if (!child.widget && !child.layout)
			continue;

		childCount += 1;

		Gui::SizeHint childSizeHint{};
		if (child.type == LayoutItem::Type::Layout)
			childSizeHint = child.layout->SizeHint_Tick(ctx);
		else
			childSizeHint = child.widget->SizeHint_Tick(ctx);

		u32& directionLength = direction == Direction::Horizontal ? returnVal.preferred.width : returnVal.preferred.height;
		u32 const& childDirectionLength = direction == Direction::Horizontal ? childSizeHint.preferred.width : childSizeHint.preferred.height;
		directionLength += childDirectionLength;

		u32& nonDirectionLength = direction == Direction::Horizontal ? returnVal.preferred.height : returnVal.preferred.width;
		u32 const& childNonDirectionLength = direction == Direction::Horizontal ? childSizeHint.preferred.height : childSizeHint.preferred.width;
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
		returnVal.expand = true;

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

	IterateOverChildren<false>(
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

void StackLayout::CharEvent(Context& ctx, u32 utfValue)
{
	ParentType::CharEvent(
		ctx,
		utfValue);

	ResolveWidgetAddRemoves();

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

	ResolveWidgetAddRemoves();

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
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	ParentType::CursorMove(
		ctx,
		widgetRect,
		visibleRect,
		event);

	ResolveWidgetAddRemoves();

	IterateOverChildren<false>(
		ctx,
		*this,
		widgetRect,
		[&ctx, visibleRect, event](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::Type::Layout)
				child.layout->CursorMove(
					ctx,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
			else
				child.widget->CursorMove(
					ctx,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
		});
}

void StackLayout::CursorClick(
	Context& ctx,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	ParentType::CursorClick(
		ctx,
		widgetRect,
		visibleRect,
		cursorPos,
		event);

	ResolveWidgetAddRemoves();

	IterateOverChildren<false>(
		ctx,
		*this,
		widgetRect,
		[&ctx, visibleRect, cursorPos, event](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::Type::Layout)
				child.layout->CursorClick(
					ctx,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					cursorPos,
					event);
			else
				child.widget->CursorClick(
					ctx,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					cursorPos,
					event);
		});
}

void StackLayout::TouchEvent(
	Context& ctx,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event)
{
	ParentType::TouchEvent(
		ctx,
		widgetRect,
		visibleRect,
		event);

	ResolveWidgetAddRemoves();

	IterateOverChildren<false>(
		ctx,
		*this,
		widgetRect,
		[&ctx, visibleRect, event](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::Type::Layout)
				child.layout->TouchEvent(
					ctx,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
			else
				child.widget->TouchEvent(
					ctx,
					childRect,
					Rect::Intersection(childRect, visibleRect),
					event);
		});
}

void StackLayout::Tick(
	Context& ctx,
	Rect widgetRect,
	Rect visibleRect)
{
	ParentType::Tick(
		ctx,
		widgetRect,
		visibleRect);

	ResolveWidgetAddRemoves();

	IterateOverChildren<true>(
		ctx,
		*this,
		widgetRect,
		[&ctx, visibleRect](LayoutItem& child, Rect childRect)
		{
			if (child.type == LayoutItem::Type::Layout)
				child.layout->Tick(
					ctx,
					childRect,
					Rect::Intersection(visibleRect, childRect));
			else
				child.widget->Tick(
					ctx,
					childRect,
					Rect::Intersection(visibleRect, childRect));
		});
}

void StackLayout::ResolveWidgetAddRemoves()
{
	for (auto& job : addWidgetJobs)
	{
		DENGINE_IMPL_GUI_ASSERT(
			job.type == InsertRemoveJob::Type::Insert ||
			job.type == InsertRemoveJob::Type::Remove);

		if (job.type == InsertRemoveJob::Type::Insert)
		{
			auto& insert = job.insert;
			if (insert.index == InsertRemoveJob::Insert::pushBackIndex)
				children.emplace_back(static_cast<LayoutItem&&>(insert.item));
			else
				children.emplace(children.begin() + insert.index, static_cast<LayoutItem&&>(insert.item));
		}
		else if (job.type == InsertRemoveJob::Type::Remove)
		{
			auto& remove = job.remove;

			children.erase(children.begin() + remove.index);
		}
	}
	addWidgetJobs.clear();
}
