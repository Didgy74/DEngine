#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/impl/Assert.hpp>

#include <DEngine/Math/Common.hpp>

#include <DEngine/Std/Trait.hpp>

namespace DEngine::Gui::impl
{
	// Generates the rect for the inner StackLayout Rect with padding taken into account
	[[nodiscard]] static Rect BuildInnerRect(
		Rect const& widgetRect,
		u32 padding) noexcept
	{
		Rect returnVal = widgetRect;
		returnVal.position.x += padding;
		returnVal.position.y += padding;
		returnVal.extent.width -= padding;
		returnVal.extent.height -= padding;
		return returnVal;
	}

	static void BuildChildRects(
		Std::Span<SizeHint const> sizeHints,
		Rect rect,
		StackLayout::Direction dir,
		u32 spacing,
		Std::Span<Rect> outChildRects)
	{
		DENGINE_IMPL_GUI_ASSERT(sizeHints.Size() == outChildRects.Size());

		uSize childCount = sizeHints.Size();

		// Go over each cilds size hint
		// Find the biggest size in non-direction
		// Find the sum of sizes in direction
		Extent sumChildPreferredSize = {};

		for (uSize i = 0; i < childCount; i += 1)
		{
			auto const& childSizeHint = sizeHints[i];

			// Add to the sum size.
			bool shouldAdd = dir == StackLayout::Direction::Horizontal && !childSizeHint.expandX;
			shouldAdd = shouldAdd || (dir == StackLayout::Direction::Vertical && !childSizeHint.expandY);
			if (shouldAdd)
			{
				u32& directionLength = dir == StackLayout::Direction::Horizontal ?
					sumChildPreferredSize.width :
					sumChildPreferredSize.height;
				u32 const& childDirectionLength = dir == StackLayout::Direction::Horizontal ?
					childSizeHint.preferred.width :
					childSizeHint.preferred.height;
				directionLength += childDirectionLength;

				u32& nonDirectionLength = dir == StackLayout::Direction::Vertical ?
					sumChildPreferredSize.width :
					sumChildPreferredSize.height;
				u32 const& childNonDirectionLength = dir == StackLayout::Direction::Vertical ?
					childSizeHint.preferred.width :
					childSizeHint.preferred.height;
				nonDirectionLength = Math::Max(nonDirectionLength, childNonDirectionLength);
			}
		}

		i32 remainingDirSpace = 0;
		if (dir == StackLayout::Direction::Horizontal)
			remainingDirSpace = rect.extent.width - sumChildPreferredSize.width;
		else
			remainingDirSpace = rect.extent.height - sumChildPreferredSize.height;

		Rect childRect = {};
		childRect.position = rect.position;
		if (dir == StackLayout::Direction::Horizontal)
			childRect.extent.height = rect.extent.height;
		else
			childRect.extent.width = rect.extent.width;
		for (uSize i = 0; i < childCount; i++)
		{
			SizeHint const& childSizeHint = sizeHints[i];
			if (dir == StackLayout::Direction::Horizontal)
			{
				if (childSizeHint.expandX)
					childRect.extent.width = remainingDirSpace;
				else
					childRect.extent.width = childSizeHint.preferred.width;
			}
			else
			{
				if (childSizeHint.expandY)
					childRect.extent.height = remainingDirSpace;
				else
					childRect.extent.height = childSizeHint.preferred.height;
			}

			outChildRects[i] = childRect;

			auto& childPosDir = dir == StackLayout::Direction::Horizontal ? childRect.position.x : childRect.position.y;
			auto const& childExtentDir = dir == StackLayout::Direction::Horizontal ? childRect.extent.width : childRect.extent.height;

			childPosDir += childExtentDir + spacing;
		}
	}

	template<class T>
	struct StackLayout_ItPair;
	// DO NOT USE DIRECTLY!
	template<class T>
	struct StackLayout_It
	{
		static constexpr bool isConst = Std::Trait::isConst<T>;

		StackLayout_ItPair<T> const* itPair = nullptr;
		uSize index = 0;

		[[nodiscard]] bool operator!=(StackLayout_It const& rhs) const noexcept
		{
			return index != rhs.index;
		}

		StackLayout_It& operator++() noexcept
		{
			index += 1;
			return *this;
		}

		struct Deref_T
		{
			using Widget_T = Std::Trait::Cond<isConst, Widget const, Widget>;
			Widget_T& widget;
			Rect childRect;
		};
		[[nodiscard]] Deref_T operator*() const noexcept;
	};

	// DO NOT USE DIRECTLY!
	template<class T>
	struct StackLayout_ItPair
	{
		static constexpr bool isConst = Std::Trait::isConst<T>;

		T* layout = nullptr;

		std::vector<Rect> childRects;

		[[nodiscard]] StackLayout_It<T> begin() const noexcept
		{
			StackLayout_It<T> returnVal = {};
			returnVal.itPair = this;
			returnVal.index = 0;
			return returnVal;
		}

		[[nodiscard]] StackLayout_It<T> end() const noexcept
		{
			StackLayout_It<T> returnVal = {};
			returnVal.itPair = this;
			returnVal.index = childRects.size();
			return returnVal;
		}

		[[nodiscard]] decltype(auto) GetWidget(uSize index) const noexcept
		{
			return *layout->children[index];
		}

		// DO NOT USE DIRECTLY!
		[[nodiscard]] static StackLayout_ItPair Create(
			T& layout,
			Context const& ctx,
			Rect rect)
		{
			// Set the StackLayout's internal state to currently iterating
			if constexpr (!isConst)
				layout.currentlyIterating = true;

			StackLayout_ItPair returnVal;
			returnVal.layout = &layout;

			uSize const childCount = (uSize)layout.children.size();

			std::vector<SizeHint> childSizeHints;
			childSizeHints.resize(childCount);
			for (uSize i = 0; i < childCount; i += 1)
				childSizeHints[i] = layout.children[i]->GetSizeHint(ctx);

			returnVal.childRects.resize(childCount);
			BuildChildRects(
				{ childSizeHints.data(), childSizeHints.size() },
				rect,
				layout.direction,
				layout.spacing,
				{ returnVal.childRects.data(), returnVal.childRects.size() });

			return returnVal;
		}

		~StackLayout_ItPair() noexcept
		{
			if constexpr (!isConst)
			{
				// Reset the "changed indices stuff"
				layout->currentlyIterating = false;
				layout->insertionJobs.clear();
			}
		}
	};

	template<class T>
	typename StackLayout_It<T>::Deref_T StackLayout_It<T>::operator*() const noexcept
	{
		return Deref_T{
			itPair->GetWidget(index),
			itPair->childRects[index] };
	}

	[[nodiscard]] static StackLayout_ItPair<StackLayout> BuildItPair(
		StackLayout& layout,
		Context const& ctx,
		Rect widgetRect) noexcept
	{
		return StackLayout_ItPair<StackLayout>::Create(
			layout,
			ctx,
			widgetRect);
	}

	[[nodiscard]] static StackLayout_ItPair<StackLayout const> BuildItPair(
		StackLayout const& layout,
		Context const& ctx,
		Rect widgetRect) noexcept
	{
		return StackLayout_ItPair<StackLayout const>::Create(
			layout,
			ctx,
			widgetRect);
	}

	[[nodiscard]] static bool PointerMove(
		StackLayout& layout,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		Math::Vec2 pointerPos,
		bool pointerOccluded,
		CursorMoveEvent const& event)
	{
		auto innerRect = impl::BuildInnerRect(widgetRect, layout.padding);

		auto itPair = impl::BuildItPair(layout, ctx, innerRect);
		for (auto const& itItem : itPair)
		{
			itItem.widget.CursorMove(
				ctx,
				windowId,
				itItem.childRect,
				visibleRect,
				event,
				pointerOccluded);
		}

		bool returnVal = false;

		// If we're inside the inner rect, we want to occlude the pointer.
		bool pointerInsideWidget = innerRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		if (pointerInsideWidget)
			returnVal = true;

		return returnVal;
	}

	enum class PointerType : u8 { Primary, Secondary };
	[[nodiscard]] static PointerType ToPointerType(Gui::CursorButton in) noexcept
	{
		switch (in)
		{
			case Gui::CursorButton::Primary: return PointerType::Primary;
			case Gui::CursorButton::Secondary: return PointerType::Secondary;
			default: break;
		}
		DENGINE_IMPL_UNREACHABLE();
		return {};
	}
	[[nodiscard]] static bool PointerPress(
		StackLayout& layout,
		Context& ctx,
		WindowID windowId,
		Rect const& widgetRect,
		Rect const& visibleRect,
		Math::Vec2 pointerPos,
		bool pointerPressed,
		CursorClickEvent const& event)
	{
		auto innerRect = impl::BuildInnerRect(widgetRect, layout.padding);
		
		bool pointerInWidget = innerRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
		if (pointerPressed && !pointerInWidget)
			return false;

		auto itPair = impl::BuildItPair(layout, ctx, innerRect);
		for (auto const& itItem : itPair)
		{
			bool pointerInChild = itItem.childRect.PointIsInside(pointerPos) && visibleRect.PointIsInside(pointerPos);
			if (pointerPressed && !pointerInChild)
				continue;
			
			bool eventConsumed = itItem.widget.CursorPress(
				ctx,
				windowId,
				itItem.childRect,
				visibleRect,
				{ (i32)pointerPos.x, (i32)pointerPos.y },
				event);
			if (pointerPressed && eventConsumed)
				return true;
		}

		// We know we're inside the widget, so we consume the event.
		return true;
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

	if (Rect::Intersection(widgetRect, visibleRect).IsNothing())
		return;

	// Only draw the quad of the layout if the alpha of the color is high enough to be visible.
	if (color.w > 0.01f)
	{
		drawInfo.PushFilledQuad(widgetRect, color);
	}

	auto itPair = impl::BuildItPair(*this, ctx, widgetRect);
	for (auto const& item : itPair)
	{
		Rect childVisibleRect = Rect::Intersection(item.childRect, visibleRect);
		if (childVisibleRect.IsNothing())
			continue;
		item.widget.Render(
			ctx,
			framebufferExtent,
			item.childRect,
			childVisibleRect,
			drawInfo);
	}
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

bool StackLayout::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event,
	bool cursorOccluded)
{
	return impl::PointerMove(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		{ (f32)event.position.x, (f32)event.position.y },
		cursorOccluded,
		event);
}

bool StackLayout::CursorPress(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	return impl::PointerPress(
		*this,
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		{ (f32)cursorPos.x, (f32)cursorPos.y },
		event.clicked,
		event);
}

void StackLayout::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event,
	bool cursorOccluded)
{
	/*
	ParentType::TouchEvent(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event,
		cursorOccluded);

	impl::StackLayout_IterateOverChildren(
		ctx,
		*this,
		widgetRect,
		[&ctx, windowId, visibleRect, event, cursorOccluded](Widget& child, Rect childRect)
		{
			child.TouchEvent(
				ctx,
				windowId,
				childRect,
				Rect::Intersection(childRect, visibleRect),
				event,
				cursorOccluded);
		});
		*/
}
