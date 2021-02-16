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
			DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
			auto entity = editorImpl.selectedEntity.Value();
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
		DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
		auto entity = editorImpl.selectedEntity.Value();
		auto componentPtr = editorImpl.scene->GetComponent<Transform>(entity);
		DENGINE_DETAIL_ASSERT(componentPtr);
		auto& component = *componentPtr;
		component.rotation = std::stof(widget.text.c_str());
	};

	this->collapseCallback = [this, &editorImpl](bool collapsed)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
		auto entity = editorImpl.selectedEntity.Value();
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

	DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
	auto entity = editorImpl.selectedEntity.Value();
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
		DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
		auto entity = editorImpl.selectedEntity.Value();
		auto componentPtr = editorImpl.scene->GetComponent<Gfx::TextureID>(entity);
		DENGINE_DETAIL_ASSERT(componentPtr);
		auto& component = *componentPtr;
		component = (Gfx::TextureID)std::stoi(widget.text.c_str());
	};

	this->collapseCallback = [this, &editorImpl](bool collapsed)
	{
		// Confirm we have a selected entity, since the widget is alive we must have one.
		DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
		auto entity = editorImpl.selectedEntity.Value();
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

	DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
	auto entity = editorImpl.selectedEntity.Value();
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

Box2DWidget::Box2DWidget(EditorImpl const& editorImpl)
{
	this->SetHeaderText("Box2D");

	Gui::StackLayout& mainLayout = this->GetChildStackLayout();

	{
		Gui::StackLayout* bodyTypeLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
		mainLayout.AddWidget(Std::Box<Widget>{ bodyTypeLayout });
		
		Gui::Text* bodyTypeLabel = new Gui::Text;
		bodyTypeLayout->AddWidget(Std::Box<Widget>{ bodyTypeLabel });
		bodyTypeLabel->String_Set("Type");
		
		Gui::Dropdown* bodyTypeDropdown = new Gui::Dropdown;
		bodyTypeLayout->AddWidget(Std::Box<Widget>{ bodyTypeDropdown });
		bodyTypeDropdown->items.push_back("Static"); // b2BodyType::b2_staticBody = 0
		bodyTypeDropdown->items.push_back("Kinematic"); // b2BodyType::b2_kinematicBody = 1
		bodyTypeDropdown->items.push_back("Dynamic");// b2BodyType::b2_dynamicBody = 1
		bodyTypeDropdown->selectedItem = b2BodyType::b2_dynamicBody;
		bodyTypeDropdown->selectedItemChangedCallback = [&editorImpl](
			Gui::Dropdown& dropdown)
		{
			// Update the box2D body
			DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
			Entity entity = editorImpl.selectedEntity.Value();
			auto componentPtr = editorImpl.scene->GetComponent<Box2D_Component>(entity);
			DENGINE_DETAIL_ASSERT(componentPtr);
			auto& component = *componentPtr;
			component.ptr->SetType(b2BodyType(dropdown.selectedItem));
		};
	}

	{
		Gui::StackLayout* restitutionLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
		mainLayout.AddWidget(Std::Box<Widget>{ restitutionLayout });

		Gui::Text* restitutionLabel = new Gui::Text;
		restitutionLayout->AddWidget(Std::Box<Widget>{ restitutionLabel });
		restitutionLabel->String_Set("Restitution");

		restitutionLineEdit = new Gui::LineFloatEdit;
		restitutionLayout->AddWidget(Std::Box<Widget>{ restitutionLineEdit });
		restitutionLineEdit->min = 0.0;
		restitutionLineEdit->max = 1.0;
		restitutionLineEdit->text = "0.000";
		restitutionLineEdit->valueChangedCallback = [&editorImpl](
			Gui::LineFloatEdit& widget,
			f64 newValue)
		{
			// Update the box2D body
			DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
			Entity entity = editorImpl.selectedEntity.Value();
			auto componentPtr = editorImpl.scene->GetComponent<Box2D_Component>(entity);
			DENGINE_DETAIL_ASSERT(componentPtr);
			auto& component = *componentPtr;
			component.ptr->GetFixtureList()->SetRestitution((f32)newValue);
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
		DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
		auto entity = editorImpl.selectedEntity.Value();
		if (collapsed)
		{
			// Confirm we have no transform component atm.
			DENGINE_DETAIL_ASSERT(!editorImpl.scene->GetComponent<Box2D_Component>(entity));

			// Check if we have a transform component
			auto const* transformPtr = editorImpl.scene->GetComponent<Transform>(entity);
			
			// Add the component
			b2BodyDef bodyDef{};
			if (transformPtr)
			{
				auto& transform = *transformPtr;
				bodyDef.position = { transform.position.x, transform.position.y };
				bodyDef.angle = transform.rotation;
			}
			else
			{
				bodyDef.position = {};
				bodyDef.angle = {};
			}
			bodyDef.allowSleep = false;
			bodyDef.gravityScale = 1.f;
			bodyDef.type = b2BodyType::b2_dynamicBody;
			auto bodyPtr = editorImpl.scene->physicsWorld->CreateBody(&bodyDef);
			DENGINE_DETAIL_ASSERT(bodyPtr);
			
			b2FixtureDef fixtureDef{};
			fixtureDef.density = 1.f;
			fixtureDef.friction = 1.f;
			fixtureDef.restitution = 0.f;
			
			b2PolygonShape shape{};
			shape.SetAsBox(0.5f, 0.5f);
			fixtureDef.shape = &shape;

			bodyPtr->CreateFixture(&fixtureDef);

			Box2D_Component newComponent{};
			newComponent.ptr = bodyPtr;

			editorImpl.scene->AddComponent(entity, newComponent);
			Update(*editorImpl.scene->GetComponent<Box2D_Component>(entity));
		}
		else
		{
			// Confirm we have transform component atm
			auto componentPtr = editorImpl.scene->GetComponent<Box2D_Component>(entity);
			DENGINE_DETAIL_ASSERT(componentPtr);
			auto const component = *componentPtr;
			// Remove the component
			editorImpl.scene->physicsWorld->DestroyBody(component.ptr);
			editorImpl.scene->DeleteComponent<Box2D_Component>(entity);
		}
	};

	DENGINE_DETAIL_ASSERT(editorImpl.selectedEntity.HasValue());
	auto entity = editorImpl.selectedEntity.Value();
	if (auto componentPtr = editorImpl.scene->GetComponent<Box2D_Component>(entity))
	{
		this->SetCollapsed(false);
		Update(*componentPtr);
	}
	else
	{
		this->SetCollapsed(true);
	}
}

void Box2DWidget::Update(Box2D_Component const& component)
{
	DENGINE_DETAIL_ASSERT(component.ptr);

	auto velocity = component.ptr->GetLinearVelocity();
	std::string velocityText = "Velocity: " + std::to_string(velocity.x) + " , " + std::to_string(velocity.y);
	debug_VelocityLabel->String_Set(velocityText.c_str());
}
