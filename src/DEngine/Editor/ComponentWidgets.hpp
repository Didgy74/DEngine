#pragma once

#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/Scene.hpp>

#include <DEngine/Std/Utility.hpp>

#include <DEngine/Physics.hpp>

namespace DEngine::Editor
{
	class MoveWidget : public Gui::StackLayout
	{
	public:
		MoveWidget(Scene* scene, Entity id) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			Gui::Button* button = new Gui::Button;
			this->AddWidget2(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->textWidget.String_Set("Testing - Move");
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				if (btn.GetToggled())
				{
					// Crash if we have an existing rigidbody on this entity
					DENGINE_DETAIL_ASSERT(Std::FindIf(
						scene->moves.begin(),
						scene->moves.end(),
						[id](decltype(scene->moves[0]) const& val) -> bool { return val.a == id; }) == scene->moves.end());

					scene->moves.push_back(Std::Pair<Entity, Move>{ id, {} });
					Move& component = scene->moves.back().b;

					Expand(*scene, id, component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);

					// Remove the transform
					auto const componentIt = Std::FindIf(
						scene->moves.begin(),
						scene->moves.end(),
						[id](decltype(scene->moves[0]) const& val) -> bool { return val.a == id; });
					DENGINE_DETAIL_ASSERT(componentIt != scene->moves.end());
					scene->moves.erase(componentIt);
				}
			};

			// Check if the entity has a rigidbody
			auto const componentIt = Std::FindIf(
				scene->moves.begin(),
				scene->moves.end(),
				[id](decltype(scene->moves[0]) const& val) -> bool { return val.a == id; });
			if (componentIt != scene->moves.end())
			{
				button->SetToggled(true);

				Move& component = componentIt->b;

				Expand(*scene, id, component);
			}
		}

		void Expand(Scene& scene, Entity id, Move const& component)
		{
		}
	};
	
	class TransformWidget : public Gui::StackLayout
	{
	public:
		Gui::LineEdit* positionInputX = nullptr;
		Gui::LineEdit* positionInputY = nullptr;
		Gui::LineEdit* positionInputZ = nullptr;

		TransformWidget(Scene* scene, Entity id) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			Gui::Button* button = new Gui::Button;
			this->AddWidget2(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->textWidget.String_Set("Transform");
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				if (btn.GetToggled())
				{
					// Add a rigidbody

					// Crash if we have an existing rigidbody on this entity
					DENGINE_DETAIL_ASSERT(Std::FindIf(
						scene->transforms.begin(),
						scene->transforms.end(),
						[id](decltype(scene->transforms[0]) const& val) -> bool { return val.a == id; }) == scene->transforms.end());

					scene->transforms.push_back(Std::Pair<Entity, Transform>{ id, {} });
					Transform& component = scene->transforms.back().b;

					Expand(*scene, id, component);
					Update(component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);
					
					// Remove the transform
					auto const componentIt = Std::FindIf(
						scene->transforms.begin(),
						scene->transforms.end(),
						[id](decltype(scene->transforms[0]) const& val) -> bool { return val.a == id; });
					DENGINE_DETAIL_ASSERT(componentIt != scene->transforms.end());
					scene->transforms.erase(componentIt);
				}
			};

			// Check if the entity has a rigidbody
			auto const componentIt = Std::FindIf(
				scene->transforms.begin(),
				scene->transforms.end(),
				[id](decltype(scene->transforms[0]) const& val) -> bool { return val.a == id; });
			if (componentIt != scene->transforms.end())
			{
				button->SetToggled(true);

				Transform& component = componentIt->b;

				Expand(*scene, id, component);
				Update(component);
			}
		}

		void Expand(Scene& scene, Entity id, Transform const& component)
		{
			// Create the horizontal position stuff layout
			Gui::StackLayout* positionLayout = new StackLayout(StackLayout::Direction::Horizontal);
			this->AddLayout2(Std::Box<Layout>{ positionLayout });
			positionLayout->spacing = 10;

			// Create the Position: text
			Gui::Text* positionText = new Gui::Text;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionText });
			positionText->String_Set("Position: ");

			// Create the Position input field
			positionInputX = new Gui::LineEdit;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionInputX });
			positionInputX->type = Gui::LineEdit::Type::Float;
			positionInputX->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				auto const componentIt = Std::FindIf(
					scene.transforms.begin(),
					scene.transforms.end(),
					[id](decltype(scene.transforms[0]) const& value) -> bool { return id == value.a; });
				DENGINE_DETAIL_ASSERT(componentIt != scene.transforms.end());
				Transform& component = componentIt->b;
				component.position.x = std::stof(widget.StringView().data());
			};

			// Create the Position input field
			positionInputY = new Gui::LineEdit;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionInputY });
			positionInputY->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				auto const componentIt = Std::FindIf(
					scene.transforms.begin(),
					scene.transforms.end(),
					[id](decltype(scene.transforms[0]) const& value) -> bool { return id == value.a; });
				DENGINE_DETAIL_ASSERT(componentIt != scene.transforms.end());
				Transform& component = componentIt->b;
				component.position.y = std::stof(widget.StringView().data());
			};

			positionInputZ = new Gui::LineEdit;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionInputZ });
			positionInputZ->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				auto const componentIt = Std::FindIf(
					scene.transforms.begin(),
					scene.transforms.end(),
					[id](decltype(scene.transforms[0]) const& value) -> bool { return id == value.a; });
				DENGINE_DETAIL_ASSERT(componentIt != scene.transforms.end());
				Transform& component = componentIt->b;
				component.position.z = std::stof(widget.StringView().data());
			};
		}

		void Update(Transform const& component)
		{
			if (!positionInputX->CurrentlyBeingEdited())
				positionInputX->String_Set(std::to_string(component.position.x).c_str());
			if (!positionInputY->CurrentlyBeingEdited())
				positionInputY->String_Set(std::to_string(component.position.y).c_str());
			if (!positionInputZ->CurrentlyBeingEdited())
				positionInputZ->String_Set(std::to_string(component.position.z).data());
		}

		void Tick(Scene& scene, Entity id)
		{
			auto const componentIt = Std::FindIf(
				scene.transforms.begin(),
				scene.transforms.end(),
				[id](decltype(scene.transforms[0]) const& value) -> bool { return id == value.a; });
			if (componentIt != scene.transforms.end())
			{
				Transform& component = componentIt->b;
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
			this->AddWidget2(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->textWidget.String_Set("SpriteRenderer2D");
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				if (btn.GetToggled())
				{
					// Add a rigidbody

					// Crash if we have an existing component on this entity
					DENGINE_DETAIL_ASSERT(Std::FindIf(
						scene->textureIDs.begin(),
						scene->textureIDs.end(),
						[id](decltype(scene->textureIDs[0]) const& val) -> bool { return val.a == id; }) == scene->textureIDs.end());

					scene->textureIDs.push_back(Std::Pair<Entity, Gfx::TextureID>{ id, {} });
					Gfx::TextureID& component = scene->textureIDs.back().b;

					Expand(*scene, id, component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);

					// Remove the transform
					auto const componentIt = Std::FindIf(
						scene->textureIDs.begin(),
						scene->textureIDs.end(),
						[id](decltype(scene->textureIDs[0]) const& val) -> bool { return val.a == id; });
					DENGINE_DETAIL_ASSERT(componentIt != scene->textureIDs.end());
					scene->textureIDs.erase(componentIt);
				}
			};

			// Check if the entity has a rigidbody
			auto const componentIt = Std::FindIf(
				scene->textureIDs.begin(),
				scene->textureIDs.end(),
				[id](decltype(scene->textureIDs[0]) const& val) -> bool { return val.a == id; });
			if (componentIt != scene->textureIDs.end())
			{
				button->SetToggled(true);

				Gfx::TextureID& component = componentIt->b;

				Expand(*scene, id, component);
			}
		}

		void Expand(Scene& scene, Entity id, Gfx::TextureID const& component)
		{
			// Create the horizontal position stuff layout
			Gui::StackLayout* positionLayout = new StackLayout(StackLayout::Direction::Horizontal);
			this->AddLayout2(Std::Box<Gui::Layout>{ positionLayout });

			// Create the Position: text
			Gui::Text* positionText = new Gui::Text;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{positionText });
			positionText->String_Set("Texture: ");

			// Create the Position input field
			Gui::LineEdit* positionInputX = new Gui::LineEdit;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionInputX });
			positionInputX->type = Gui::LineEdit::Type::UnsignedInteger;
			positionInputX->String_Set(std::to_string((int)component).c_str());
			positionInputX->textChangedPfn = [id, &scene](Gui::LineEdit& widget)
			{
				if (!widget.StringView().empty())
				{
					auto componentIt = Std::FindIf(
						scene.textureIDs.begin(),
						scene.textureIDs.end(),
						[id](Std::Pair<Entity, Gfx::TextureID> const& value) -> bool { return id == value.a; });

					Gfx::TextureID& component = componentIt->b;
					component = (Gfx::TextureID)std::stoi(widget.StringView().data());
				}
			};
		}
	};

	class Rigidbody2DWidget : public Gui::StackLayout
	{
	public:
		Gui::Text* velocityText = nullptr;

		Rigidbody2DWidget(Scene* scene, Entity id) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			Gui::Button* button = new Gui::Button;
			this->AddWidget2(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->textWidget.String_Set("Rigidbody2D");
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				if (btn.GetToggled())
				{
					// Add a rigidbody

					// Crash if we have an existing rigidbody on this entity
					DENGINE_DETAIL_ASSERT(Std::FindIf(
						scene->rigidbodies.begin(),
						scene->rigidbodies.end(),
						[id](decltype(scene->rigidbodies[0]) const& val) -> bool { return val.a == id; }) == scene->rigidbodies.end());

					scene->rigidbodies.push_back(Std::Pair<Entity, Physics::Rigidbody2D>{ id, {} });
					Physics::Rigidbody2D& component = scene->rigidbodies.back().b;

					Expand(component);
					Update(component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);

					// Remove the rigidbody
					auto const componentIt = Std::FindIf(
						scene->rigidbodies.begin(),
						scene->rigidbodies.end(),
						[id](decltype(scene->rigidbodies[0]) const& val) -> bool { return val.a == id; });
					DENGINE_DETAIL_ASSERT(componentIt != scene->rigidbodies.end());
					scene->rigidbodies.erase(componentIt);
				}
			};

			// Check if the entity has a rigidbody
			auto const componentIt = Std::FindIf(
				scene->rigidbodies.begin(),
				scene->rigidbodies.end(),
				[id](decltype(scene->rigidbodies[0]) const& val) -> bool { return val.a == id; });
			if (componentIt != scene->rigidbodies.end())
			{
				button->SetToggled(true);

				Physics::Rigidbody2D& component = componentIt->b;

				Expand(component);
				Update(component);
			}
		}

		void Expand(Physics::Rigidbody2D const& component)
		{
			velocityText = new Gui::Text;
			this->AddWidget2(Std::Box<Gui::Widget>{ velocityText });
			Update(component);
		}

		void Update(Physics::Rigidbody2D const& component)
		{
			std::string velocityString = "Velocity: ";
			velocityString += std::to_string(component.velocity.x) + " ";
			velocityString += std::to_string(component.velocity.y);
			velocityText->String_Set(velocityString.c_str());
		}

		void Tick(Scene& scene, Entity id)
		{
			auto const componentIt = Std::FindIf(
				scene.rigidbodies.begin(),
				scene.rigidbodies.end(),
				[id](decltype(scene.rigidbodies[0]) const& value) -> bool { return id == value.a; });
			if (componentIt != scene.rigidbodies.end())
			{
				Physics::Rigidbody2D& component = componentIt->b;
				Update(component);
			}
		}
	};

	class CircleCollider2DWidget : public Gui::StackLayout
	{
	public:
		CircleCollider2DWidget(Scene* scene, Entity id) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			Gui::Button* button = new Gui::Button;
			this->AddWidget2(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->textWidget.String_Set("CircleCollider2D");
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				if (btn.GetToggled())
				{
					// Add a rigidbody

					// Crash if we have an existing boxcollider on this entity
					DENGINE_DETAIL_ASSERT(Std::FindIf(
						scene->boxColliders.begin(),
						scene->boxColliders.end(),
						[id](decltype(scene->boxColliders[0]) const& val) -> bool { return val.a == id; }) == scene->boxColliders.end());
					
					// Crash if we have an existing circlecollider on this entity
					DENGINE_DETAIL_ASSERT(Std::FindIf(
						scene->circleColliders.begin(),
						scene->circleColliders.end(),
						[id](decltype(scene->circleColliders[0]) const& val) -> bool { return val.a == id; }) == scene->circleColliders.end());

					scene->circleColliders.push_back(Std::Pair<Entity, Physics::CircleCollider2D>{ id, {} });
					Physics::CircleCollider2D& component = scene->circleColliders.back().b;

					Expand(component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);

					// Remove the rigidbody
					auto const componentIt = Std::FindIf(
						scene->circleColliders.begin(),
						scene->circleColliders.end(),
						[id](decltype(scene->circleColliders[0]) const& val) -> bool { return val.a == id; });
					DENGINE_DETAIL_ASSERT(componentIt != scene->circleColliders.end());
					scene->circleColliders.erase(componentIt);
				}
			};

			// Check if the entity has a rigidbody
			auto const componentIt = Std::FindIf(
				scene->circleColliders.begin(),
				scene->circleColliders.end(),
				[id](decltype(scene->circleColliders[0]) const& val) -> bool { return val.a == id; });
			if (componentIt != scene->circleColliders.end())
			{
				button->SetToggled(true);

				Physics::CircleCollider2D& component = componentIt->b;

				Expand(component);
			}
		}

		void Expand(Physics::CircleCollider2D const& component)
		{
			Gui::Text* radiusText = new Gui::Text;
			this->AddWidget2(Std::Box<Gui::Widget>{ radiusText });
			std::string radiusString = "Radius: " + std::to_string(component.radius);
			radiusText->String_Set(radiusString.c_str());
		}
	};

	class BoxCollider2DWidget : public Gui::StackLayout
	{
	public:
		BoxCollider2DWidget(Scene* scene, Entity id) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			Gui::Button* button = new Gui::Button;
			this->AddWidget2(Std::Box<Gui::Widget>{ button });
			button->type = Gui::Button::Type::Toggle;
			button->textWidget.String_Set("BoxCollider2D");
			button->activatePfn = [this, id, scene](
				Gui::Button& btn,
				Gui::Context& ctx,
				Gui::WindowID windowId,
				Gui::Rect widgetRect,
				Gui::Rect visibleRect)
			{
				if (btn.GetToggled())
				{
					// Crash if we have an existing circlecollider on this entity
					DENGINE_DETAIL_ASSERT(Std::FindIf(
						scene->circleColliders.begin(),
						scene->circleColliders.end(),
						[id](decltype(scene->circleColliders[0]) const& val) -> bool { return val.a == id; }) == scene->circleColliders.end());

					// Crash if we have an existing boxcollider on this entity
					DENGINE_DETAIL_ASSERT(Std::FindIf(
						scene->boxColliders.begin(),
						scene->boxColliders.end(),
						[id](decltype(scene->boxColliders[0]) const& val) -> bool { return val.a == id; }) == scene->boxColliders.end());

					scene->boxColliders.push_back(Std::Pair<Entity, Physics::BoxCollider2D>{ id, {} });
					Physics::BoxCollider2D& component = scene->boxColliders.back().b;

					Expand(component);
				}
				else
				{
					// Remove all children except for index 0, it's the button to add/remove.
					uSize childCount = this->ChildCount();
					for (uSize i = childCount - 1; i != 0; i -= 1)
						this->RemoveItem(i);

					// Remove the rigidbody
					auto const componentIt = Std::FindIf(
						scene->boxColliders.begin(),
						scene->boxColliders.end(),
						[id](decltype(scene->boxColliders[0]) const& val) -> bool { return val.a == id; });
					DENGINE_DETAIL_ASSERT(componentIt != scene->boxColliders.end());
					scene->boxColliders.erase(componentIt);
				}
			};

			// Check if the entity has a rigidbody
			auto const componentIt = Std::FindIf(
				scene->boxColliders.begin(),
				scene->boxColliders.end(),
				[id](decltype(scene->boxColliders[0]) const& val) -> bool { return val.a == id; });
			if (componentIt != scene->boxColliders.end())
			{
				button->SetToggled(true);

				Physics::BoxCollider2D& component = componentIt->b;

				Expand(component);
			}
		}

		void Expand(Physics::BoxCollider2D const& component)
		{
			Gui::Text* sizeText = new Gui::Text;
			this->AddWidget2(Std::Box<Gui::Widget>{ sizeText });
			std::string sizeString = "Size: " + std::to_string(component.size.x) + " " + std::to_string(component.size.y);
			sizeText->String_Set(sizeString.c_str());
		}
	};
}