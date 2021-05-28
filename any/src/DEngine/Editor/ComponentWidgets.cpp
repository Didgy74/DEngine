#include "ComponentWidgets.hpp"

#include "EditorImpl.hpp"

using namespace DEngine;
using namespace DEngine::Editor;

TransformWidget::TransformWidget(EditorImpl const& editorImpl)
{
	this->SetHeaderText("Transform");

	Gui::StackLayout& mainLayout = this->GetChildStackLayout();
	// Create the horizontal position stuff layout
	Gui::StackLayout* positionLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
	mainLayout.AddWidget(Std::Box<Widget>{ positionLayout });
	positionLayout->spacing = 10;

	// Create the Position:
	Gui::Text* positionText = new Gui::Text;
	positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionText });
	positionText->String_Set("Position:");

	// Create the Position input fields
	for (uSize i = 0; i < 3; i += 1)
	{
		auto& inputField = this->positionInputFields[i];
		inputField = new Gui::LineEdit;
		positionLayout->AddWidget(Std::Box<Gui::Widget>{ inputField });
		inputField->type = Gui::LineEdit::Type::Float;
		inputField->textChangedPfn = [i, &editorImpl](Gui::LineEdit& widget)
		{
			DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
			auto entity = editorImpl.GetSelectedEntity().Value();
			auto componentPtr = editorImpl.scene->GetComponent<Transform>(entity);
			DENGINE_DETAIL_ASSERT(componentPtr);
			auto& component = *componentPtr;
			component.position[i] = std::stof(widget.text.c_str());
		};
	}

	Gui::StackLayout* rotationLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
	mainLayout.AddWidget(Std::Box<Widget>{ rotationLayout });
	rotationLayout->spacing = 10;

	Gui::Text* rotationLabel = new Gui::Text();
	rotationLayout->AddWidget(Std::Box<Gui::Widget>{ rotationLabel });
	rotationLabel->String_Set("Rotation:");

	rotationInput = new Gui::LineEdit;
	rotationLayout->AddWidget(Std::Box<Gui::Widget>{ rotationInput });
	rotationInput->textChangedPfn = [&editorImpl](Gui::LineEdit& widget)
	{
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		auto componentPtr = editorImpl.scene->GetComponent<Transform>(entity);
		DENGINE_DETAIL_ASSERT(componentPtr);
		auto& component = *componentPtr;
		component.rotation = std::stof(widget.text.c_str());
	};

	this->collapseCallback = [this, &editorImpl](bool collapsed)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (collapsed)
		{
			// Confirm we have no component atm.
			DENGINE_DETAIL_ASSERT(!editorImpl.scene->GetComponent<Transform>(entity));
			
			// Add the component
			Transform component{};
			editorImpl.scene->AddComponent(entity, component);

			Update(*editorImpl.scene->GetComponent<Transform>(entity));
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
		this->SetCollapsed(false);
		Update(*componentPtr);
	}
	else
	{
		this->SetCollapsed(true);
	}
}

void TransformWidget::Update(Transform const& component)
{
	for (uSize i = 0; i < 3; i += 1)
	{
		if (!positionInputFields[i]->CurrentlyBeingEdited())
			positionInputFields[i]->text = std::to_string(component.position[i]).c_str();
	}

	if (!rotationInput->CurrentlyBeingEdited())
		rotationInput->text = std::to_string(component.rotation).c_str();
}

SpriteRenderer2DWidget::SpriteRenderer2DWidget(EditorImpl const& editorImpl)
{
	this->SetHeaderText("SpriteRender2D");

	Gui::StackLayout& mainLayout = this->GetChildStackLayout();

	// Create the horizontal position stuff layout
	Gui::StackLayout* textureIdLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
	mainLayout.AddWidget(Std::Box<Widget>{ textureIdLayout });
	textureIdLayout->spacing = 10;

	// Create the Position: text
	Gui::Text* textureIDLabel = new Gui::Text;
	textureIdLayout->AddWidget(Std::Box<Gui::Widget>{ textureIDLabel });
	textureIDLabel->String_Set("Texture ID:");

	// Create the Position input field
	textureIdInput = new Gui::LineEdit;
	textureIdLayout->AddWidget(Std::Box<Gui::Widget>{ textureIdInput });
	textureIdInput->type = Gui::LineEdit::Type::Integer;
	textureIdInput->textChangedPfn = [&editorImpl](Gui::LineEdit& widget)
	{
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		auto componentPtr = editorImpl.scene->GetComponent<Gfx::TextureID>(entity);
		DENGINE_DETAIL_ASSERT(componentPtr);
		auto& component = *componentPtr;
		component = (Gfx::TextureID)std::stoi(widget.text.c_str());
	};

	this->collapseCallback = [&editorImpl](bool collapsed)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (collapsed)
		{
			// Confirm we have no component atm.
			DENGINE_DETAIL_ASSERT(!editorImpl.scene->GetComponent<Gfx::TextureID>(entity));

			// Add the component
			Gfx::TextureID component{};
			editorImpl.scene->AddComponent(entity, component);
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
		this->SetCollapsed(false);
		Update(*componentPtr);
	}
	else
	{
		this->SetCollapsed(true);
	}
}

void SpriteRenderer2DWidget::Update(Gfx::TextureID const& component)
{
	if (!textureIdInput->CurrentlyBeingEdited())
		textureIdInput->text = std::to_string((unsigned int)component).c_str();
}

RigidbodyWidget::RigidbodyWidget(EditorImpl const& editorImpl)
{
	this->SetHeaderText("Rigidbody2D");

	Gui::StackLayout& mainLayout = this->GetChildStackLayout();

	{
		Gui::StackLayout* bodyTypeLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
		mainLayout.AddWidget(Std::Box<Widget>{ bodyTypeLayout });
		
		Gui::Text* bodyTypeLabel = new Gui::Text;
		bodyTypeLayout->AddWidget(Std::Box<Widget>{ bodyTypeLabel });
		bodyTypeLabel->String_Set("Type");
		
		Gui::Dropdown* bodyTypeDropdown = new Gui::Dropdown;
		bodyTypeLayout->AddWidget(Std::Box<Widget>{ bodyTypeDropdown });
		bodyTypeDropdown->items.push_back("Dynamic"); // Rigidbody2D::Type::Dynamic = 0
		bodyTypeDropdown->items.push_back("Static"); // Rigidbody2D::Type::Static = 0
		bodyTypeDropdown->selectedItem = (u32)Physics::Rigidbody2D::Type::Dynamic;
		bodyTypeDropdown->selectedItemChangedCallback = [&editorImpl](
			Gui::Dropdown& dropdown)
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

	{
		Gui::Text* debugLabel = new Gui::Text;
		mainLayout.AddWidget(Std::Box<Gui::Widget>{ debugLabel });
		debugLabel->String_Set("Debug:");
	}

	{
		debug_VelocityLabel = new Gui::Text;
		mainLayout.AddWidget(Std::Box<Gui::Widget>{ debug_VelocityLabel });
	}

	this->collapseCallback = [this, &editorImpl](bool collapsed)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.GetSelectedEntity().HasValue());
		auto entity = editorImpl.GetSelectedEntity().Value();
		if (collapsed)
		{
			// Confirm we have no rigidbody component atm.
			DENGINE_DETAIL_ASSERT(!editorImpl.scene->GetComponent<Physics::Rigidbody2D>(entity));

			Physics::Rigidbody2D newComponent = {};

			editorImpl.scene->AddComponent(entity, newComponent);
			Update(*editorImpl.scene->GetComponent<Physics::Rigidbody2D>(entity));
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
		this->SetCollapsed(false);
		Update(*componentPtr);
	}
	else
	{
		this->SetCollapsed(true);
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
