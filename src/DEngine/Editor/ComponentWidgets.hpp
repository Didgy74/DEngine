#pragma once

#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/CollapsingHeader.hpp>

#include <DEngine/Scene.hpp>

#include <DEngine/Std/Utility.hpp>

namespace DEngine::Editor
{
	class EditorImpl;

	class MoveWidget : public Gui::StackLayout
	{
	public:
		MoveWidget(Scene* scene, Entity id) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			Gui::Button* button = new Gui::Button;
			this->AddWidget(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->text = "Testing - Move";
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context* guiCtx)
			{
				if (btn.GetToggled())
				{
					// Crash if we have an existing rigidbody on this entity
					DENGINE_DETAIL_ASSERT(scene->GetComponent<Move>(id) == nullptr);

					scene->AddComponent(id, Move{});
					Move& component = *scene->GetComponent<Move>(id);

					Expand(*scene, id, component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);

					// Remove the transform
					scene->DeleteComponent<Move>(id);
				}
			};

			Move* moveComponentPtr = scene->GetComponent<Move>(id);
			if (moveComponentPtr != nullptr)
			{
				button->SetToggled(true);

				Move& component = *moveComponentPtr;

				Expand(*scene, id, component);
			}
		}

		void Expand(Scene& scene, Entity id, Move const& component)
		{
		}
	};
	
	class TransformWidget2 : public Gui::CollapsingHeader
	{
	public:
		Gui::LineEdit* positionInputX = nullptr;
		Gui::LineEdit* positionInputY = nullptr;
		Gui::LineEdit* positionInputZ = nullptr;
		Gui::LineEdit* rotationInput = nullptr;

		TransformWidget2(EditorImpl const& editorImpl);

		void Update(Transform const& component);
	};

	class TransformWidget : public Gui::StackLayout
	{
	public:
		Gui::LineEdit* positionInputX = nullptr;
		Gui::LineEdit* positionInputY = nullptr;
		Gui::LineEdit* positionInputZ = nullptr;
		Gui::LineEdit* rotationInput = nullptr;

		TransformWidget(Scene* scene, Entity id) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			Gui::Button* button = new Gui::Button;
			this->AddWidget(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->text = "Transform";
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context* guiCtx)
			{
				if (btn.GetToggled())
				{
					// Crash if we have an existing transform on this entity
					DENGINE_DETAIL_ASSERT(scene->GetComponent<Transform>(id) == nullptr);
					
					scene->AddComponent(id, Transform{});
					Transform& component = *scene->GetComponent<Transform>(id);

					Expand(*scene, id, component);
					Update(component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);
					
					scene->DeleteComponent<Transform>(id);
				}
			};

			// Check if the entity has a rigidbody
			Transform* transformPtr = scene->GetComponent<Transform>(id);
			if (transformPtr != nullptr)
			{
				button->SetToggled(true);

				Transform& component = *transformPtr;

				Expand(*scene, id, component);
				Update(component);
			}
		}

		void Expand(Scene& scene, Entity id, Transform const& component)
		{
			// Create the horizontal position stuff layout
			Gui::StackLayout* positionLayout = new StackLayout(StackLayout::Direction::Horizontal);
			this->AddWidget(Std::Box<Widget>{ positionLayout });
			positionLayout->spacing = 10;

			// Create the Position: text
			Gui::Text* positionText = new Gui::Text;
			positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionText });
			positionText->String_Set("Position: ");

			// Create the Position input field
			positionInputX = new Gui::LineEdit;
			positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionInputX });
			positionInputX->type = Gui::LineEdit::Type::Float;
			positionInputX->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				Transform* transformPtr = scene.GetComponent<Transform>(id);
				DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
				Transform& component = *transformPtr;
				component.position.x = std::stof(widget.text.data());
			};

			// Create the Position input field
			positionInputY = new Gui::LineEdit;
			positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionInputY });
			positionInputY->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				Transform* transformPtr = scene.GetComponent<Transform>(id);
				DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
				Transform& component = *transformPtr;
				component.position.y = std::stof(widget.text.data());
			};

			positionInputZ = new Gui::LineEdit;
			positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionInputZ });
			positionInputZ->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				Transform* transformPtr = scene.GetComponent<Transform>(id);
				DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
				Transform& component = *transformPtr;
				component.position.z = std::stof(widget.text.data());
			};


			Gui::StackLayout* rotationLayout = new StackLayout(StackLayout::Direction::Horizontal);
			this->AddWidget(Std::Box<Widget>{ rotationLayout });
			rotationLayout->spacing = 10;

			Gui::Text* rotationText = new Gui::Text();
			rotationLayout->AddWidget(Std::Box<Gui::Widget>{ rotationText });
			rotationText->String_Set("Rotation: ");

			rotationInput = new Gui::LineEdit;
			rotationLayout->AddWidget(Std::Box<Gui::Widget>{ rotationInput });
			rotationInput->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				Transform* transformPtr = scene.GetComponent<Transform>(id);
				DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
				Transform& component = *transformPtr;
				component.rotation = std::stof(widget.text.data());
			};
		}

		void Update(Transform const& component)
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

		void Tick(Scene& scene, Entity id)
		{
			Transform* transformPtr = scene.GetComponent<Transform>(id);
			if (transformPtr != nullptr)
			{
				Transform& component = *transformPtr;
				Update(component);
			}
		}
	};

	class SpriteRenderer2DWidget : public Gui::StackLayout
	{
	public:
		SpriteRenderer2DWidget(Scene* scene, Entity id) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			Gui::Button* button = new Gui::Button;
			this->AddWidget(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->text = "SpriteRenderer2D";
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context* guiCtx)
			{
				if (btn.GetToggled())
				{
					// Add a rigidbody

					// Crash if we have an existing component on this entity
					DENGINE_DETAIL_ASSERT(scene->GetComponent<Gfx::TextureID>(id) == nullptr);

					scene->AddComponent(id, Gfx::TextureID{});
					Gfx::TextureID& component = *scene->GetComponent<Gfx::TextureID>(id);

					Expand(*scene, id, component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);

					// Remove the component
					scene->DeleteComponent<Gfx::TextureID>(id);
				}
			};

			// Check if the entity has a component
			Gfx::TextureID* componentPtr = scene->GetComponent<Gfx::TextureID>(id);
			if (componentPtr != nullptr)
			{
				button->SetToggled(true);

				Gfx::TextureID& component = *componentPtr;

				Expand(*scene, id, component);
			}
		}

		void Expand(Scene& scene, Entity id, Gfx::TextureID const& component)
		{
			// Create the horizontal position stuff layout
			Gui::StackLayout* positionLayout = new StackLayout(StackLayout::Direction::Horizontal);
			this->AddWidget(Std::Box<Gui::Widget>{ positionLayout });

			// Create the Position: text
			Gui::Text* positionText = new Gui::Text;
			positionLayout->AddWidget(Std::Box<Gui::Widget>{positionText });
			positionText->String_Set("Texture: ");

			// Create the Position input field
			Gui::LineEdit* positionInputX = new Gui::LineEdit;
			positionLayout->AddWidget(Std::Box<Gui::Widget>{ positionInputX });
			positionInputX->type = Gui::LineEdit::Type::UnsignedInteger;
			positionInputX->text = std::to_string((int)component).c_str();
			positionInputX->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				if (!widget.text.empty())
				{
					Gfx::TextureID* componentPtr = scene.GetComponent<Gfx::TextureID>(id);
					DENGINE_DETAIL_ASSERT(componentPtr != nullptr);
					Gfx::TextureID& component = *componentPtr;
					component = (Gfx::TextureID)std::stoi(widget.text.data());
				}
			};
		}
	};
}