#include "ComponentWidgets.hpp"

#include "EditorImpl.hpp"

#include "Editor.hpp"

#include <sstream>

#include <DEngine/Gui/Grid.hpp>

using namespace DEngine;
using namespace DEngine::Editor;
using namespace DEngine::Gui;

MoveWidget::MoveWidget(EditorImpl const& editorImpl)
{
	title = "PlayerMove";
	collapsedColor = Settings::GetColor(Settings::Color::Button_Normal);
	expandedColor = Settings::GetColor(Settings::Color::Button_Active);
	titleMargin = Settings::defaultTextMargin;

	this->collapseFn = [&editorImpl](CollapsingHeader& widget)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (!widget.collapsed)
		{
			// Confirm we have no component atm.
			DENGINE_IMPL_ASSERT(!editorImpl.scene->GetComponent<ComponentType>(entity));

			// Add the component
			ComponentType component = {};
			editorImpl.scene->AddComponent(entity, component);
		}
		else
		{
			// Confirm we have transform component atm
			DENGINE_IMPL_ASSERT(editorImpl.scene->GetComponent<ComponentType>(entity));
			// Remove the component
			editorImpl.scene->DeleteComponent<ComponentType>(entity);
		}
	};

	DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
	auto entity = editorImpl.GetSelectedEntity().Value();
	if (editorImpl.scene->GetComponent<ComponentType>(entity))
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
	collapsedColor = Settings::GetColor(Settings::Color::Button_Normal);
	expandedColor = Settings::GetColor(Settings::Color::Button_Active);
	titleMargin = Settings::defaultTextMargin;

	auto* outerGrid = new Grid;
	this->child = Std::Box{ outerGrid };
	outerGrid->SetWidth(4);
	outerGrid->spacing = 10;

	// Create the Position:
	{
		int row = outerGrid->PushBackRow();

		auto positionLabel = new Gui::Text;
		outerGrid->SetChild(0, row, Std::Box{ positionLabel });
		positionLabel->margin = Settings::defaultTextMargin;
		positionLabel->text = "P:";
		positionLabel->expandX = false;

		// Create the Position input fields
		for (int i = 0; i < 3; i += 1)
		{
			auto& inputField = positionInputFields[i];
			inputField = new LineFloatEdit;
			outerGrid->SetChild(i + 1, row, Std::Box{ inputField });

			inputField->backgroundColor = Settings::GetColor(Settings::Color::Button_Normal);
			inputField->margin = Settings::defaultTextMargin;
			inputField->decimalPoints = Settings::inputFieldPrecision;
			inputField->valueChangedFn = [i, &editorImpl](LineFloatEdit& widget, f64 newValue)
			{
				DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
				auto entity = editorImpl.GetSelectedEntity().Value();
				auto componentPtr = editorImpl.scene->GetComponent<ComponentType>(entity);
				DENGINE_IMPL_ASSERT(componentPtr);
				auto& component = *componentPtr;

				component.position[i] = (f32)newValue;
			};
		}
	}



	{
		int row = outerGrid->PushBackRow();

		auto rotationLabel = new Text;
		outerGrid->SetChild(0, row, Std::Box{ rotationLabel });
		rotationLabel->text = "R:";
		rotationLabel->margin = Settings::defaultTextMargin;
		rotationLabel->expandX = false;

		rotationInput = new LineFloatEdit;
		outerGrid->SetChild(1, row, Std::Box{ rotationInput });
		rotationInput->backgroundColor = Settings::GetColor(Settings::Color::Button_Normal);
		rotationInput->margin = Settings::defaultTextMargin;
		rotationInput->decimalPoints = Settings::inputFieldPrecision;
		rotationInput->valueChangedFn = [&editorImpl](LineFloatEdit& widget, f64 newValue)
		{
			DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
			auto entity = editorImpl.GetSelectedEntity().Value();
			auto* componentPtr = editorImpl.scene->GetComponent<ComponentType>(entity);
			DENGINE_IMPL_ASSERT(componentPtr);
			auto& component = *componentPtr;
			component.rotation = (f32)newValue;
		};
	}

	{
		int row = outerGrid->PushBackRow();

		auto scaleText = new Text;
		outerGrid->SetChild(0, row, Std::Box{ scaleText });
		scaleText->text = "S:";
		scaleText->margin = Settings::defaultTextMargin;
		scaleText->expandX = false;

		// Create the scale input fields
		for (int i = 0; i < 2; i += 1)
		{
			auto& inputField = this->scaleInputFields[i];
			inputField = new LineFloatEdit;
			outerGrid->SetChild(i+1, row, Std::Box{ inputField });

			inputField->backgroundColor = Settings::GetColor(Settings::Color::Button_Normal);
			inputField->margin = Settings::defaultTextMargin;
			inputField->decimalPoints = Settings::inputFieldPrecision;
			inputField->valueChangedFn = [i, &editorImpl](LineFloatEdit& widget, f64 newValue)
			{
				DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
				auto entity = editorImpl.GetSelectedEntity().Value();
				auto componentPtr = editorImpl.scene->GetComponent<ComponentType>(entity);
				DENGINE_IMPL_ASSERT(componentPtr);
				auto& component = *componentPtr;
				component.scale[i] = (f32)newValue;
			};
		}
	}


	this->collapseFn = [&editorImpl](CollapsingHeader& widget)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (!widget.collapsed)
		{
			// Confirm we have no component atm.
			DENGINE_IMPL_ASSERT(!editorImpl.scene->GetComponent<ComponentType>(entity));
			
			// Add the component
			ComponentType component{};
			editorImpl.scene->AddComponent(entity, component);

			auto& cast = static_cast<TransformWidget&>(widget);
			cast.Update(*editorImpl.scene->GetComponent<ComponentType>(entity));
		}
		else
		{
			// Confirm we have transform component atm
			DENGINE_IMPL_ASSERT(editorImpl.scene->GetComponent<ComponentType>(entity));
			// Remove the component
			editorImpl.scene->DeleteComponent<ComponentType>(entity);
		}
	};

	DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
	auto entity = editorImpl.GetSelectedEntity().Value();
	if (auto componentPtr = editorImpl.scene->GetComponent<ComponentType>(entity))
	{
		this->collapsed = false;
		Update(*componentPtr);
	}
	else
	{
		this->collapsed = true;
	}
}

void TransformWidget::Update(ComponentType const& component)
{
	for (uSize i = 0; i < 3; i += 1)
	{
		if (!positionInputFields[i]->HasInputSession())
		{
			auto& widget = *positionInputFields[i];
			widget.SetValue(component.position[i]);
		}
	}

	if (!rotationInput->HasInputSession())
	{
		auto& widget = *rotationInput;
		widget.SetValue(component.rotation);
	}

	for (uSize i = 0; i < 2; i += 1)
	{
		if (!scaleInputFields[i]->HasInputSession())
		{
			auto& widget = *scaleInputFields[i];
			widget.SetValue(component.scale[i]);
		}
	}
}

SpriteRenderer2DWidget::SpriteRenderer2DWidget(EditorImpl const& editorImpl)
{
	title = "SpriteRender2D";
	collapsedColor = Settings::GetColor(Settings::Color::Button_Normal);
	expandedColor = Settings::GetColor(Settings::Color::Button_Active);
	titleMargin = Settings::defaultTextMargin;

	auto innerStackLayout = new StackLayout(StackLayout::Dir::Vertical);
	child = Std::Box{ innerStackLayout };

	// Create the horizontal position stuff layout
	auto textureIdLayout = new StackLayout(StackLayout::Dir::Horizontal);
	innerStackLayout->AddWidget(Std::Box{ textureIdLayout });
	textureIdLayout->spacing = 10;

	// Create the Position: text
	auto textureIDLabel = new Text;
	textureIdLayout->AddWidget(Std::Box{ textureIDLabel });
	textureIDLabel->text = "Texture ID:";
	textureIDLabel->margin = Editor::Settings::defaultTextMargin;
	textureIDLabel->expandX = false;

	// Create the integer field
	textureIdInput = new LineIntEdit;
	textureIdLayout->AddWidget(Std::Box{ textureIdInput });
	textureIdInput->backgroundColor = Settings::GetColor(Settings::Color::Button_Normal);
	textureIdInput->margin = Settings::defaultTextMargin;
	textureIdInput->min = 0;
	textureIdInput->valueChangedFn = [&editorImpl](LineIntEdit& widget, i64 newValue)
	{
		DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		auto* componentPtr = editorImpl.scene->GetComponent<ComponentType>(entity);
		DENGINE_IMPL_ASSERT(componentPtr);
		auto& component = *componentPtr;
		component = (Gfx::TextureID)newValue;
	};

	this->collapseFn = [&editorImpl](CollapsingHeader& widget)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (!widget.collapsed)
		{
			// Confirm we have no component atm.
			DENGINE_IMPL_ASSERT(!editorImpl.scene->GetComponent<ComponentType>(entity));

			// Add the component
			Gfx::TextureID component{};
			editorImpl.scene->AddComponent(entity, component);

			auto& cast = static_cast<SpriteRenderer2DWidget&>(widget);
			cast.Update(*editorImpl.scene->GetComponent<ComponentType>(entity));
		}
		else
		{
			// Confirm we have transform component atm
			DENGINE_IMPL_ASSERT(editorImpl.scene->GetComponent<ComponentType>(entity));
			// Remove the component
			editorImpl.scene->DeleteComponent<ComponentType>(entity);
		}
	};

	DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
	auto entity = editorImpl.GetSelectedEntity().Value();
	if (auto componentPtr = editorImpl.scene->GetComponent<ComponentType>(entity))
	{
		this->collapsed = false;
		Update(*componentPtr);
	}
	else
	{
		this->collapsed = true;
	}
}

void SpriteRenderer2DWidget::Update(ComponentType const& component)
{
	if (!textureIdInput->HasInputSession())
	{
		textureIdInput->SetValue((unsigned int)component);
	}
}

RigidbodyWidget::RigidbodyWidget(EditorImpl const& editorImpl)
{
	title = "Rigidbody2D";
	collapsedColor = Settings::GetColor(Settings::Color::Button_Normal);
	expandedColor = Settings::GetColor(Settings::Color::Button_Active);
	titleMargin = Settings::defaultTextMargin;

	auto innerStackLayout = new StackLayout(StackLayout::Dir::Vertical);
	child = Std::Box{ innerStackLayout };

	{
		auto bodyTypeLayout = new StackLayout(StackLayout::Dir::Horizontal);
		innerStackLayout->AddWidget(Std::Box{ bodyTypeLayout });
		
		auto bodyTypeLabel = new Text;
		bodyTypeLayout->AddWidget(Std::Box{ bodyTypeLabel });
		bodyTypeLabel->text = "Type:";
		bodyTypeLabel->margin = Settings::defaultTextMargin;
		bodyTypeLabel->expandX = false;
		
		bodyTypeDropdown = new Dropdown;
		bodyTypeLayout->AddWidget(Std::Box{ bodyTypeDropdown });
		bodyTypeDropdown->textMargin = Settings::defaultTextMargin;
		bodyTypeDropdown->items.push_back("Dynamic"); // Rigidbody2D::Type::Dynamic = 0
		bodyTypeDropdown->items.push_back("Static"); // Rigidbody2D::Type::Static = 0
		bodyTypeDropdown->selectedItem = (u32)Physics::Rigidbody2D::Type::Dynamic;
		bodyTypeDropdown->selectionChangedCallback = [&editorImpl](Dropdown& dropdown)
		{
			// Update the box2D body
			DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
			Entity entity = editorImpl.GetSelectedEntity().Value();
			auto componentPtr = editorImpl.scene->GetComponent<ComponentType>(entity);
			DENGINE_IMPL_ASSERT(componentPtr);
			auto& component = *componentPtr;
			component.type = (ComponentType::Type)dropdown.selectedItem;
		};
	}

	this->collapseFn = [&editorImpl](CollapsingHeader& widget)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (!widget.collapsed)
		{
			// Confirm we have no rigidbody component atm.
			DENGINE_IMPL_ASSERT(!editorImpl.scene->GetComponent<ComponentType>(entity));

			ComponentType newComponent = {};

			editorImpl.scene->AddComponent(entity, newComponent);
			auto& cast = static_cast<RigidbodyWidget&>(widget);
			cast.Update(*editorImpl.scene->GetComponent<ComponentType>(entity));
		}
		else
		{
			editorImpl.scene->DeleteComponent<ComponentType>(entity);
		}
	};

	DENGINE_IMPL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
	auto entity = editorImpl.GetSelectedEntity().Value();
	if (auto componentPtr = editorImpl.scene->GetComponent<ComponentType>(entity))
	{
		this->collapsed = false;
		Update(*componentPtr);
	}
	else
	{
		this->collapsed = true;
	}
}

void RigidbodyWidget::Update(ComponentType const& component)
{
	if (!component.b2BodyPtr)
		return;
	auto physBody = (b2Body*)component.b2BodyPtr;
	auto velocity = physBody->GetLinearVelocity();
	std::string velocityText = "Velocity: " + std::to_string(velocity.x) + " , " + std::to_string(velocity.y);
	debug_VelocityLabel->text = velocityText;
}
