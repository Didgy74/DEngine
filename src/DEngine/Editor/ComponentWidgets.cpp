#include "ComponentWidgets.hpp"

#include "EditorImpl.hpp"

using namespace DEngine;
using namespace DEngine::Editor;

TransformWidget2::TransformWidget2(EditorImpl const& editorImpl) : Gui::CollapsingHeader()
{
	Gui::StackLayout& stackLayout = this->GetChildStackLayout();
	// Create the horizontal position stuff layout
	Gui::StackLayout* positionLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
	stackLayout.AddWidget(Std::Box<Widget>{ positionLayout });
	positionLayout->spacing = 10;

	// Create the Position: text
	Gui::Text* positionText = new Gui::Text;
	positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionText });
	positionText->String_Set("Position: ");

	// Create the Position input field
	positionInputX = new Gui::LineEdit;
	positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionInputX });
	positionInputX->type = Gui::LineEdit::Type::Float;
	positionInputX->textChangedPfn = [&editorImpl](Gui::LineEdit& widget)
	{
		if (editorImpl.selectedEntity.HasValue())
		{
			Entity id = editorImpl.selectedEntity.Value();
			Transform* transformPtr = editorImpl.scene->GetComponent<Transform>(id);
			DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
			Transform& component = *transformPtr;
			component.position.x = std::stof(widget.text.c_str());
		}
	};

	// Create the Position input field
	positionInputY = new Gui::LineEdit;
	positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionInputY });
	/*
	positionInputY->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
	{
		Transform* transformPtr = scene.GetComponent<Transform>(id);
		DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
		Transform& component = *transformPtr;
		component.position.y = std::stof(widget.StringView().data());
	};
	*/

	positionInputZ = new Gui::LineEdit;
	positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionInputZ });
	/*
	positionInputZ->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
	{
		Transform* transformPtr = scene.GetComponent<Transform>(id);
		DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
		Transform& component = *transformPtr;
		component.position.z = std::stof(widget.StringView().data());
	};
	*/


	Gui::StackLayout* rotationLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
	stackLayout.AddWidget(Std::Box<Widget>{ rotationLayout });
	rotationLayout->spacing = 10;

	Gui::Text* rotationText = new Gui::Text();
	rotationLayout->AddWidget(Std::Box<Gui::Widget>{ rotationText });
	rotationText->String_Set("Rotation: ");

	rotationInput = new Gui::LineEdit;
	rotationLayout->AddWidget(Std::Box<Gui::Widget>{ rotationInput });
	/*
	rotationInput->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
	{
		Transform* transformPtr = scene.GetComponent<Transform>(id);
		DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
		Transform& component = *transformPtr;
		component.rotation = std::stof(widget.StringView().data());
	};
	*/
}


void TransformWidget2::Update(Transform const& component)
{
	if (!positionInputX->CurrentlyBeingEdited())
		positionInputX->text = std::to_string(component.position.x).c_str();
	if (!positionInputY->CurrentlyBeingEdited())
		positionInputY->text = std::to_string(component.position.y).c_str();
	if (!positionInputZ->CurrentlyBeingEdited())
		positionInputZ->text = std::to_string(component.position.z).c_str();
	if (!rotationInput->CurrentlyBeingEdited())
		rotationInput->text = std::to_string(component.rotation).c_str();
}