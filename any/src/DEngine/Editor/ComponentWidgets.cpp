#include "ComponentWidgets.hpp"

#include "EditorImpl.hpp"

#include "Editor.hpp"

#include <sstream>

using namespace DEngine;
using namespace DEngine::Editor;
using namespace DEngine::Gui;

MoveWidget::MoveWidget(EditorImpl const& editorImpl)
{
	title = "PlayerMove";
	titleMargin = Settings::defaultTextMargin;

	this->collapseFn = [&editorImpl](CollapsingHeader& widget)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (!widget.collapsed)
		{
			// Confirm we have no component atm.
			DENGINE_DETAIL_ASSERT(!editorImpl.scene->GetComponent<Move>(entity));

			// Add the component
			Move component = {};
			editorImpl.scene->AddComponent(entity, component);
		}
		else
		{
			// Confirm we have transform component atm
			DENGINE_DETAIL_ASSERT(editorImpl.scene->GetComponent<Move>(entity));
			// Remove the component
			editorImpl.scene->DeleteComponent<Move>(entity);
		}
	};

	DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
	auto entity = editorImpl.GetSelectedEntity().Value();
	if (editorImpl.scene->GetComponent<Transform>(entity))
	{
		this->collapsed = false;
	}
	else
	{
		this->collapsed = true;
	}
}

TransformWidget::TransformWidget(EditorImpl const& editorImpl)
{
	title = "Transform";
	titleMargin = Settings::defaultTextMargin;

	auto innerStackLayout = new StackLayout(StackLayout::Dir::Vertical);
	this->child = Std::Box{ innerStackLayout };
	innerStackLayout->spacing = 10;

	// Create the horizontal position stuff layout
	auto positionLayout = new StackLayout(StackLayout::Dir::Horizontal);
	innerStackLayout->AddWidget(Std::Box{ positionLayout });
	positionLayout->spacing = 10;

	// Create the Position:
	auto positionLabel = new Gui::Text;
	positionLabel->margin = Settings::defaultTextMargin;
	positionLayout->AddWidget(Std::Box{ positionLabel });
	positionLabel->String_Set("P:");
	positionLabel->margin = Settings::defaultTextMargin;

	// Create the Position input fields
	for (uSize i = 0; i < 3; i += 1)
	{
		auto& inputField = this->positionInputFields[i];
		inputField = new LineEdit;
		positionLayout->AddWidget(Std::Box{ inputField });
		inputField->margin = Settings::defaultTextMargin;
		inputField->type = LineEdit::Type::Float;
		inputField->textChangedFn = [i, &editorImpl](LineEdit& widget)
		{
			DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
			auto entity = editorImpl.GetSelectedEntity().Value();
			auto componentPtr = editorImpl.scene->GetComponent<Transform>(entity);
			DENGINE_DETAIL_ASSERT(componentPtr);
			auto& component = *componentPtr;

			component.position[i] = std::stof(widget.text.c_str());
		};
	}

	auto rotationLayout = new StackLayout(StackLayout::Dir::Horizontal);
	innerStackLayout->AddWidget(Std::Box{ rotationLayout });
	rotationLayout->spacing = 10;

	auto rotationLabel = new Text;
	rotationLayout->AddWidget(Std::Box{ rotationLabel });
	rotationLabel->String_Set("R:");
	rotationLabel->margin = Settings::defaultTextMargin;

	rotationInput = new LineEdit;
	rotationLayout->AddWidget(Std::Box{ rotationInput });
	rotationInput->margin = Settings::defaultTextMargin;
	rotationInput->textChangedFn = [&editorImpl](LineEdit& widget)
	{
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		auto componentPtr = editorImpl.scene->GetComponent<Transform>(entity);
		DENGINE_DETAIL_ASSERT(componentPtr);
		auto& component = *componentPtr;
		component.rotation = std::stof(widget.text.c_str());
	};

	// Create the horizontal scale stuff layout
	auto* scaleLayout = new StackLayout(StackLayout::Dir::Horizontal);
	innerStackLayout->AddWidget(Std::Box{ scaleLayout });
	scaleLayout->spacing = 10;

	// Create the Scale:
	auto scaleText = new Text;
	scaleLayout->AddWidget(Std::Box{ scaleText });
	scaleText->String_Set("S:");
	scaleText->margin = Settings::defaultTextMargin;

	// Create the scale input fields
	for (uSize i = 0; i < 2; i += 1)
	{
		auto& inputField = this->scaleInputFields[i];
		inputField = new LineEdit;
		scaleLayout->AddWidget(Std::Box{ inputField });
		inputField->margin = Settings::defaultTextMargin;
		inputField->type = LineEdit::Type::Float;
		inputField->textChangedFn = [i, &editorImpl](LineEdit& widget)
		{
			DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
			auto entity = editorImpl.GetSelectedEntity().Value();
			auto componentPtr = editorImpl.scene->GetComponent<Transform>(entity);
			DENGINE_DETAIL_ASSERT(componentPtr);
			auto& component = *componentPtr;

			component.scale[i] = std::stof(widget.text.c_str());
		};
	}

	this->collapseFn = [&editorImpl](CollapsingHeader& widget)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (!widget.collapsed)
		{
			// Confirm we have no component atm.
			DENGINE_DETAIL_ASSERT(!editorImpl.scene->GetComponent<Transform>(entity));
			
			// Add the component
			Transform component{};
			editorImpl.scene->AddComponent(entity, component);

			auto& cast = static_cast<TransformWidget&>(widget);
			cast.Update(*editorImpl.scene->GetComponent<Transform>(entity));
		}
		else
		{
			// Confirm we have transform component atm
			DENGINE_DETAIL_ASSERT(editorImpl.scene->GetComponent<Transform>(entity));
			// Remove the component
			editorImpl.scene->DeleteComponent<Transform>(entity);
		}
	};

	DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
	auto entity = editorImpl.GetSelectedEntity().Value();
	if (auto componentPtr = editorImpl.scene->GetComponent<Transform>(entity))
	{
		this->collapsed = false;
		Update(*componentPtr);
	}
	else
	{
		this->collapsed = true;
	}
}

void TransformWidget::Update(Transform const& component)
{
	for (uSize i = 0; i < 3; i += 1)
	{
		if (!positionInputFields[i]->CurrentlyBeingEdited())
		{
			auto& widget = *positionInputFields[i];
			std::ostringstream out;
			out.precision(Settings::inputFieldPrecision);
			out << std::fixed << component.position[i];
			widget.text = out.str();
		}
	}

	if (!rotationInput->CurrentlyBeingEdited())
	{
		auto& widget = *rotationInput;
		std::ostringstream out;
		out.precision(Settings::inputFieldPrecision);
		out << std::fixed << component.rotation;
		widget.text = out.str();
	}

	for (uSize i = 0; i < 2; i += 1)
	{
		if (!scaleInputFields[i]->CurrentlyBeingEdited())
		{
			auto& widget = *scaleInputFields[i];
			std::ostringstream out;
			out.precision(Settings::inputFieldPrecision);
			out << std::fixed << component.scale[i];
			widget.text = out.str();
		}
	}
}

SpriteRenderer2DWidget::SpriteRenderer2DWidget(EditorImpl const& editorImpl)
{
	this->title = "SpriteRender2D";
	this->titleMargin = Settings::defaultTextMargin;

	auto innerStackLayout = new StackLayout(StackLayout::Dir::Vertical);
	this->child = Std::Box{ innerStackLayout };

	// Create the horizontal position stuff layout
	auto textureIdLayout = new StackLayout(StackLayout::Dir::Horizontal);
	innerStackLayout->AddWidget(Std::Box{ textureIdLayout });
	textureIdLayout->spacing = 10;

	// Create the Position: text
	auto textureIDLabel = new Text;
	textureIdLayout->AddWidget(Std::Box{ textureIDLabel });
	textureIDLabel->String_Set("Texture ID:");
	textureIDLabel->margin = Editor::Settings::defaultTextMargin;

	// Create the Position input field
	textureIdInput = new LineEdit;
	textureIdLayout->AddWidget(Std::Box{ textureIdInput });
	textureIdInput->margin = Settings::defaultTextMargin;
	textureIdInput->type = LineEdit::Type::Integer;
	textureIdInput->textChangedFn = [&editorImpl](LineEdit& widget)
	{
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		auto componentPtr = editorImpl.scene->GetComponent<Gfx::TextureID>(entity);
		DENGINE_DETAIL_ASSERT(componentPtr);
		auto& component = *componentPtr;
		component = (Gfx::TextureID)std::stoi(widget.text.c_str());
	};

	this->collapseFn = [&editorImpl](CollapsingHeader& widget)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (!widget.collapsed)
		{
			// Confirm we have no component atm.
			DENGINE_DETAIL_ASSERT(!editorImpl.scene->GetComponent<Gfx::TextureID>(entity));

			// Add the component
			Gfx::TextureID component{};
			editorImpl.scene->AddComponent(entity, component);

			auto& cast = static_cast<SpriteRenderer2DWidget&>(widget);
			cast.Update(*editorImpl.scene->GetComponent<Gfx::TextureID>(entity));
		}
		else
		{
			// Confirm we have transform component atm
			DENGINE_DETAIL_ASSERT(editorImpl.scene->GetComponent<Gfx::TextureID>(entity));
			// Remove the component
			editorImpl.scene->DeleteComponent<Gfx::TextureID>(entity);
		}
	};

	DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
	auto entity = editorImpl.GetSelectedEntity().Value();
	if (auto componentPtr = editorImpl.scene->GetComponent<Gfx::TextureID>(entity))
	{
		this->collapsed = false;
		Update(*componentPtr);
	}
	else
	{
		this->collapsed = true;
	}
}

void SpriteRenderer2DWidget::Update(Gfx::TextureID const& component)
{
	if (!textureIdInput->CurrentlyBeingEdited())
		textureIdInput->text = std::to_string((unsigned int)component);
}

RigidbodyWidget::RigidbodyWidget(EditorImpl const& editorImpl)
{
	this->title = "Rigidbody2D";
	this->titleMargin = Settings::defaultTextMargin;

	auto innerStackLayout = new StackLayout(StackLayout::Dir::Vertical);
	this->child = Std::Box{ innerStackLayout };

	{
		auto bodyTypeLayout = new StackLayout(StackLayout::Dir::Horizontal);
		innerStackLayout->AddWidget(Std::Box{ bodyTypeLayout });
		
		auto bodyTypeLabel = new Text;
		bodyTypeLayout->AddWidget(Std::Box{ bodyTypeLabel });
		bodyTypeLabel->String_Set("Type:");
		bodyTypeLabel->margin = Settings::defaultTextMargin;
		
		bodyTypeDropdown = new Dropdown;
		bodyTypeLayout->AddWidget(Std::Box{ bodyTypeDropdown });
		bodyTypeDropdown->textMargin = Settings::defaultTextMargin;
		bodyTypeDropdown->items.push_back("Dynamic"); // Rigidbody2D::Type::Dynamic = 0
		bodyTypeDropdown->items.push_back("Static"); // Rigidbody2D::Type::Static = 0
		bodyTypeDropdown->selectedItem = (u32)Physics::Rigidbody2D::Type::Dynamic;
		bodyTypeDropdown->selectionChangedCallback = [&editorImpl](Dropdown& dropdown)
		{
			// Update the box2D body
			DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
			Entity entity = editorImpl.GetSelectedEntity().Value();
			auto componentPtr = editorImpl.scene->GetComponent<Physics::Rigidbody2D>(entity);
			DENGINE_DETAIL_ASSERT(componentPtr);
			auto& component = *componentPtr;
			component.type = (Physics::Rigidbody2D::Type)dropdown.selectedItem;
		};
	}

	this->collapseFn = [&editorImpl](CollapsingHeader& widget)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (!widget.collapsed)
		{
			// Confirm we have no rigidbody component atm.
			DENGINE_DETAIL_ASSERT(!editorImpl.scene->GetComponent<Physics::Rigidbody2D>(entity));

			Physics::Rigidbody2D newComponent = {};

			editorImpl.scene->AddComponent(entity, newComponent);
			auto& cast = static_cast<RigidbodyWidget&>(widget);
			cast.Update(*editorImpl.scene->GetComponent<Physics::Rigidbody2D>(entity));
		}
		else
		{
			editorImpl.scene->DeleteComponent<Physics::Rigidbody2D>(entity);
		}
	};

	DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
	auto entity = editorImpl.GetSelectedEntity().Value();
	if (auto componentPtr = editorImpl.scene->GetComponent<Physics::Rigidbody2D>(entity))
	{
		this->collapsed = false;
		Update(*componentPtr);
	}
	else
	{
		this->collapsed = true;
	}
}

void RigidbodyWidget::Update(Physics::Rigidbody2D const& component)
{
	if (!component.b2BodyPtr)
		return;
	b2Body* physBody = (b2Body*)component.b2BodyPtr;
	auto velocity = physBody->GetLinearVelocity();
	std::string velocityText = "Velocity: " + std::to_string(velocity.x) + " , " + std::to_string(velocity.y);
	debug_VelocityLabel->String_Set(velocityText.c_str());
}
