#include <DEngine/Gui/CollapsingHeader.hpp>

#include <DEngine/Std/Utility.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

CollapsingHeader::CollapsingHeader(bool collapsed)
{
	childStackLayoutBox = Std::Box{ new StackLayout(StackLayout::Direction::Vertical) };
	childStackLayoutPtr = childStackLayoutBox.Get();
	childStackLayoutPtr->color = fieldBackgroundColor;
	childStackLayoutPtr->padding = 5;
	childStackLayoutPtr->spacing = 5;

	// Create button
	Button* btn = new Button;
	mainStackLayout.AddWidget(Std::Box<Widget>{ btn });
	btn->type = Button::Type::Toggle;
	btn->text = "Test";
	
	btn->activatePfn = [this](
		Button& btn)
	{
		if (btn.GetToggled())
		{
			// We just toggled the button, this means we need to check to add the child-stacklayout.
			// Assert that the child-stacklayout isn't already inserted.
			// If we only have one child, that means we only have the button inserted, and no child-stacklayout.
			DENGINE_IMPL_GUI_ASSERT(mainStackLayout.ChildCount() == 1);

			mainStackLayout.AddWidget(Std::Box<Widget>{ childStackLayoutBox.Release() });

			if (collapseCallback)
				collapseCallback(true);
		}
		else
		{
			// We untoggled the button, we need to extract the child-stacklayout
			// Assert that we have two childs in our main stacklayout. Index #1 is the child-stacklayout.
			DENGINE_IMPL_GUI_ASSERT(mainStackLayout.ChildCount() == 2);

			// We extract the child and store it back in our boxed pointer
			mainStackLayout.At(1).InputConnectionLost();
			Std::Box<Widget> extractedChild = mainStackLayout.ExtractChild(1);
			childStackLayoutBox = Std::Box{ static_cast<StackLayout*>(extractedChild.Release()) };

			if (collapseCallback)
				collapseCallback(false);
		}
	};
	
	// If we are not collaped, toggle the button on
	// and add the child stacklayout.
	if (!collapsed)
	{
		btn->SetToggled(true);

		mainStackLayout.AddWidget(Std::Box<Widget>{ childStackLayoutBox.Release() });
	}
}

bool CollapsingHeader::IsCollapsed() const noexcept
{
	// If there is more than our button attached, our header is not collapsed.
	return mainStackLayout.ChildCount() <= 1;
}

void CollapsingHeader::SetCollapsed(bool collapsed)
{
	if (collapsed == IsCollapsed())
		return;

	Button& btn = *static_cast<Button*>(&mainStackLayout.At(0));

	if (collapsed)
	{
		btn.SetToggled(false);
		// We untoggled the button, we need to extract the child-stacklayout
		// Assert that we have two childs in our main stacklayout. Index #1 is the child-stacklayout.
		DENGINE_IMPL_GUI_ASSERT(mainStackLayout.ChildCount() == 2);

		// We extract the child and store it back in our boxed pointer
		Std::Box<Widget> extractedChild = mainStackLayout.ExtractChild(1);
		childStackLayoutBox = Std::Box{ static_cast<StackLayout*>(extractedChild.Release()) };
		childStackLayoutBox->InputConnectionLost();
	}
	else
	{
		btn.SetToggled(true);

		DENGINE_IMPL_GUI_ASSERT(mainStackLayout.ChildCount() == 1);

		mainStackLayout.AddWidget(Std::Box<Widget>{ childStackLayoutBox.Release() });
	}
}

void CollapsingHeader::SetHeaderText(std::string_view text)
{
	Button& btn = *static_cast<Button*>(&mainStackLayout.At(0));

	btn.text = text;
}

SizeHint CollapsingHeader::GetSizeHint(
	Context const& ctx) const
{
	return mainStackLayout.GetSizeHint(ctx);
}

void CollapsingHeader::Render(
	Context const& ctx, 
	Extent framebufferExtent, 
	Rect widgetRect, 
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	mainStackLayout.Render(
		ctx,
		framebufferExtent,
		widgetRect,
		visibleRect,
		drawInfo);
}

void CollapsingHeader::CharEnterEvent(
	Context& ctx)
{
	return mainStackLayout.CharEnterEvent(ctx);
}

void CollapsingHeader::CharEvent(
	Context& ctx,
	u32 utfValue)
{
	return mainStackLayout.CharEvent(ctx, utfValue);
}

void CollapsingHeader::CharRemoveEvent(
	Context& ctx)
{
	return mainStackLayout.CharRemoveEvent(ctx);
}

void CollapsingHeader::InputConnectionLost()
{
	return mainStackLayout.InputConnectionLost();
}

void CollapsingHeader::CursorMove(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	CursorMoveEvent event)
{
	return mainStackLayout.CursorMove(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event);
}

void CollapsingHeader::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	return mainStackLayout.CursorClick(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		cursorPos,
		event);
}

void CollapsingHeader::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event)
{
	return mainStackLayout.TouchEvent(
		ctx,
		windowId,
		widgetRect,
		visibleRect,
		event);
}