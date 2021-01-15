#include <DEngine/Gui/CollapsingHeader.hpp>

#include <DEngine/Std/Utility.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

CollapsingHeader::CollapsingHeader(bool collapsed)
{
	childStackLayoutBox = Std::Box{ new StackLayout(StackLayout::Direction::Vertical) };
	childStackLayoutPtr = childStackLayoutBox.Get();

	// Create button
	Button* btn = new Button;
	mainStackLayout.AddWidget(Std::Box<Widget>{ btn });
	btn->type = Button::Type::Toggle;
	btn->text = "Test";
	
	btn->activatePfn = [this](
		Button& btn,
		Context* ctx)
	{
		DENGINE_IMPL_GUI_ASSERT(ctx);
		if (btn.GetToggled())
		{
			// We just toggled the button, this means we need to check to add the child-stacklayout.
			// Assert that the child-stacklayout isn't already inserted.
			// If we only have one child, that means we only have the button inserted, and no child-stacklayout.
			DENGINE_IMPL_GUI_ASSERT(mainStackLayout.ChildCount() == 1);

			mainStackLayout.AddWidget(Std::Box<Widget>{ childStackLayoutBox.Release() });
		}
		else
		{
			// We untoggled the button, we need to extract the child-stacklayout
			// Assert that we have two childs in our main stacklayout. Index #1 is the child-stacklayout.
			DENGINE_IMPL_GUI_ASSERT(mainStackLayout.ChildCount() == 2);

			// We extract the child and store it back in our boxed pointer
			Std::Box<Widget> extractedChild = mainStackLayout.ExtractChild(1);
			childStackLayoutBox = Std::Box{ static_cast<StackLayout*>(extractedChild.Release()) };
			childStackLayoutBox->InputConnectionLost(*ctx);
		}
	};
	

	// If we are not collaped, toggle the button on
	// and add the child stacklayout.
	if (!collapsed)
	{
		btn->SetToggled(true);

		mainStackLayout.AddWidget(Std::Box<Widget>{ childStackLayoutBox.Release() });
	}

	mainStackLayout.padding = 10;

	childStackLayoutPtr->color = { 1.f, 1.f, 1.f, 0.2f };
}