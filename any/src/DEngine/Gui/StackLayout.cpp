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
		static_assert(
			Std::Trait::isSame<decltype(layout), StackLayout&> || Std::Trait::isSame<decltype(layout), StackLayout const&>,
			"When iterating over a Gui::StackLayout, the must be StackLayout& or StackLayout const&");
		
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
			childSizeHint = child->GetSizeHint(ctx);

			// Add to the sum size.
			bool shouldAdd = layout.direction == StackLayout::Direction::Horizontal && !childSizeHint.expandX;
			shouldAdd |= layout.direction == StackLayout::Direction::Vertical && !childSizeHint.expandY;
			if (shouldAdd)
			{
				u32& directionLength = layout.direction == StackLayout::Direction::Horizontal ?
					sumChildPreferredSize.width :
					sumChildPreferredSize.height;
				u32 const& childDirectionLength = layout.direction == StackLayout::Direction::Horizontal ?
					childSizeHint.preferred.width :
					childSizeHint.preferred.height;
				directionLength += childDirectionLength;

				u32& nonDirectionLength = layout.direction == StackLayout::Direction::Vertical ?
					sumChildPreferredSize.width :
					sumChildPreferredSize.height;
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
			if (layout.direction == StackLayout::Direction::Horizontal)
			{
				if (childSizeHint.expandX)
					childRect.extent.width = remainingDirectionSpace;
				else
					childRect.extent.width = childSizeHint.preferred.width;
			}
			else
			{
				if (childSizeHint.expandY)
					childRect.extent.height = remainingDirectionSpace;
				else
					childRect.extent.height = childSizeHint.preferred.height;
			}
				

			// Translate the i in the SizeHint array back to the Children array's corresponding element.
			// Note! This code is likely bugged in some cases still. Prepare to fix.
			uSize modifiedIndex = i;
			bool childExists = true;
			for (StackLayout::InsertRemoveJob const& job : layout.insertionJobs)
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
				if (child)
					callable(*child, childRect);
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

Widget& StackLayout::At(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto& child = children[index];
	
	DENGINE_IMPL_GUI_ASSERT(child);

	return *child;
}

Widget const& StackLayout::At(uSize index) const
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	auto& child = children[index];

	DENGINE_IMPL_GUI_ASSERT(child);

	return *child;
}

void StackLayout::AddWidget(Std::Box<Widget>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(in);

	children.emplace_back(static_cast<Std::Box<Widget>&&>(in));

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Insert;
		newJob.insert.index = InsertRemoveJob::Insert::pushBackIndex;
		insertionJobs.emplace_back(static_cast<InsertRemoveJob&&>(newJob));
	}
}

Std::Box<Widget> DEngine::Gui::StackLayout::ExtractChild(uSize index)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());

	auto& item = children[index];
	Std::Box<Widget> returnVal = static_cast<Std::Box<Widget>&&>(item);

	children.erase(children.begin() + index);

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Remove;
		newJob.remove.index = index;
		insertionJobs.emplace_back(newJob);
	}

	return returnVal;
}

void StackLayout::InsertWidget(uSize index, Std::Box<Widget>&& in)
{
	DENGINE_IMPL_GUI_ASSERT(index < ChildCount());
	DENGINE_IMPL_GUI_ASSERT(in);

	children.insert(children.begin() + index, static_cast<Std::Box<Widget>&&>(in));

	if (currentlyIterating)
	{
		InsertRemoveJob newJob{};
		newJob.type = InsertRemoveJob::Type::Insert;
		newJob.insert.index = index;
		insertionJobs.emplace_back(newJob);
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
		if (!child)
			continue;

		childCount += 1;

		SizeHint const childSizeHint = child->GetSizeHint(ctx);

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

	if (this->expandNonDirection)
	{
		if (this->direction == Direction::Horizontal)
			returnVal.expandY = true;
		else if (this->direction == Direction::Vertical)
			returnVal.expandX = true;
		else
			DENGINE_IMPL_UNREACHABLE();
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
		drawInfo.PushFilledQuad(widgetRect, color);
	}
	
	impl::StackLayout_IterateOverChildren(
		ctx,
		*this,
		widgetRect,
		[&ctx, framebufferExtent, &drawInfo, visibleRect](
			Widget const& child, 
			Rect childRect)
		{
			Rect childVisibleRect = Rect::Intersection(childRect, visibleRect);
			if (!childVisibleRect.IsNothing())
			{
				drawInfo.PushScissor(childVisibleRect);
				child.Render(
					ctx,
					framebufferExtent,
					childRect,
					childVisibleRect,
					drawInfo);
				drawInfo.PopScissor();
			}
		});
}

void StackLayout::CharEnterEvent(Context& ctx)
{
	ParentType::CharEnterEvent(ctx);

	for (auto& child : children)
	{
		child->CharEnterEvent(ctx);
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
		child->CharEvent(ctx, utfValue);
	}
}

void StackLayout::CharRemoveEvent(Context& ctx)
{
	ParentType::CharRemoveEvent(ctx);

	for (auto& child : children)
	{
		child->CharRemoveEvent(ctx);
	}
}

void StackLayout::InputConnectionLost()
{
	ParentType::InputConnectionLost();

	for (auto& child : children)
	{
		child->InputConnectionLost();
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
		[&ctx, windowId, visibleRect, event](Widget& child, Rect childRect)
		{
			child.CursorMove(
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
		[&ctx, windowId, visibleRect, cursorPos, event](Widget& child, Rect childRect)
		{
			child.CursorClick(
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
		[&ctx, windowId, visibleRect, event](Widget& child, Rect childRect)
		{
			child.TouchEvent(
				ctx,
				windowId,
				childRect,
				Rect::Intersection(childRect, visibleRect),
				event);
		});
}
