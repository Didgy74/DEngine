#pragma once

#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/CollapsingHeader.hpp>
#include <DEngine/Gui/Dropdown.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/LineFloatEdit.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Text.hpp>

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
				Gui::Button& btn)
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
	
	class TransformWidget : public Gui::CollapsingHeader
	{
	public:
		Gui::LineEdit* positionInputFields[3] = {};
		Gui::LineEdit* rotationInput = nullptr;

		TransformWidget(EditorImpl const& editorImpl);
		void Update(Transform const& component);
	};

	class SpriteRenderer2DWidget : public Gui::CollapsingHeader
	{
	public:
		Gui::LineEdit* textureIdInput = nullptr;
		
		SpriteRenderer2DWidget(EditorImpl const& editorImpl);
		void Update(Gfx::TextureID const& component);
	};

	class Box2DWidget : public Gui::CollapsingHeader
	{
		Gui::Dropdown* bodyTypeDropdown = nullptr;
		Gui::LineFloatEdit* restitutionLineEdit = nullptr;

		Gui::Text* debug_VelocityLabel = nullptr;

	public:
		Box2DWidget(EditorImpl const& editorImpl);
		void Update(Box2D_Component const& component);
	};
}