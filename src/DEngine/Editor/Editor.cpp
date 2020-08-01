#include "Editor.hpp"
#include "ContextImpl.hpp"

#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/Image.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Text.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Time.hpp>

#include <DEngine/Containers/Box.hpp>
#include <DEngine/Utility.hpp>

#include <DEngine/Math/Constant.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <functional>

namespace DEngine::Editor
{
	class ViewportWidget : public Gui::Image
	{
	public:
		Gfx::ViewportID viewportId = Gfx::ViewportID::Invalid;
		Gfx::Context* gfxCtx = nullptr;
		ContextImpl* implData = nullptr;

		Gui::Extent currentExtent{};

		bool isCurrentlyClicked = false;

		int joystickPixelRadius = 75;
		struct JoyStick
		{
			Std::Opt<u8> touchID{};
			bool isClicked = false;
			Math::Vec2Int startPosition{};
			Math::Vec2Int currentPosition{};
		};
		// 0 is left, 1 is right
		JoyStick joysticks[2]{};

		struct Camera
		{
			Math::Vec3 position{ 0.f, 0.f, 5.f };
			Math::UnitQuat rotation{};
			f32 verticalFov = 60.f;
		};
		Camera cam{};

		ViewportWidget(ContextImpl& implData, Gfx::Context& gfxCtxIn) :
			Gui::Image(),
			gfxCtx(&gfxCtxIn),
			implData(&implData)
		{
			implData.viewportWidget = this;

			auto newViewportRef = gfxCtx->NewViewport();
			viewportId = newViewportRef.ViewportID();
		}

		virtual ~ViewportWidget() override
		{
			gfxCtx->DeleteViewport(viewportId);
			implData->viewportWidget = nullptr;
		}

		virtual void CursorClick(
			Gui::Rect widgetRect,
			Math::Vec2Int cursorPos,
			Gui::CursorClickEvent event) override
		{
			if (event.button == Gui::CursorButton::Right && event.clicked && !isCurrentlyClicked)
			{
				bool isInside = widgetRect.PointIsInside(cursorPos);
				if (isInside)
				{
					isCurrentlyClicked = true;
					App::LockCursor(true);
				}
			}

			if (event.button == Gui::CursorButton::Right && !event.clicked && isCurrentlyClicked)
			{
				isCurrentlyClicked = false;
				App::LockCursor(false);
			}

			UpdateCircleStartPosition(widgetRect);
			if (event.button == Gui::CursorButton::Left && event.clicked)
			{
				for (auto& joystick : joysticks)
				{
					Math::Vec2Int relativeVector = cursorPos - joystick.startPosition;
					if (relativeVector.MagnitudeSqrd() <= Math::Sqrd(joystickPixelRadius))
					{
						// Cursor is inside circle
						joystick.isClicked = true;
						joystick.currentPosition = cursorPos;
					}
				}
			}

			if (event.button == Gui::CursorButton::Left && !event.clicked)
			{
				for (auto& joystick : joysticks)
				{
					joystick.isClicked = false;
					joystick.currentPosition = joystick.startPosition;
				}
			}
		}

		virtual void CursorMove(
			Gui::Rect widgetRect,
			Gui::CursorMoveEvent event) override 
		{
			UpdateCircleStartPosition(widgetRect);

			// Handle regular kb+mouse style camera movement
			if (isCurrentlyClicked)
			{
				f32 sensitivity = 0.25f;
				i32 amountX = event.positionDelta.x;
				// Apply left and right rotation
				cam.rotation = Math::UnitQuat::FromVector(Math::Vec3::Up(), -sensitivity * amountX * Math::degToRad) * cam.rotation;
				i32 amountY = event.positionDelta.y;
				// Limit rotation up and down
				Math::Vec3 forward = Math::LinTran3D::ForwardVector(cam.rotation);
				float dot = Math::Vec3::Dot(forward, Math::Vec3::Up());
				if (dot <= -0.9f)
					amountY = Math::Min(0, amountY);
				else if (dot >= 0.9f)
					amountY = Math::Max(0, amountY);
				// Apply up and down rotation
				Math::Vec3 right = Math::LinTran3D::RightVector(cam.rotation);
				cam.rotation = Math::UnitQuat::FromVector(right, -sensitivity * amountY * Math::degToRad) * cam.rotation;
			}

			for (auto& joystick : joysticks)
			{
				if (joystick.isClicked)
				{
					joystick.currentPosition = event.position;
					Math::Vec2Int relativeVector = joystick.currentPosition - joystick.startPosition;
					Math::Vec2 relativeVectorFloat = { (f32)relativeVector.x, (f32)relativeVector.y };
					if (relativeVectorFloat.MagnitudeSqrd() >= Math::Sqrd(joystickPixelRadius))
					{
						relativeVectorFloat = relativeVectorFloat.Normalized() * joystickPixelRadius;
						joystick.currentPosition.x = joystick.startPosition.x + Math::Round(relativeVectorFloat.x);
						joystick.currentPosition.y = joystick.startPosition.y + Math::Round(relativeVectorFloat.y);
					}
				}
			}

		}

		virtual void Tick(
			Gui::Context& ctx,
			Gui::Rect widgetRect) override
		{
			currentExtent = widgetRect.extent;
			UpdateCircleStartPosition(widgetRect);
			

			// Handle camera movement
			if (isCurrentlyClicked)
			{
				f32 moveSpeed = 5.f;
				if (App::ButtonValue(App::Button::W))
					cam.position -= moveSpeed * Math::LinTran3D::ForwardVector(cam.rotation) * Time::Delta();
				if (App::ButtonValue(App::Button::S))
					cam.position += moveSpeed * Math::LinTran3D::ForwardVector(cam.rotation) * Time::Delta();
				if (App::ButtonValue(App::Button::D))
					cam.position += moveSpeed * Math::LinTran3D::RightVector(cam.rotation) * Time::Delta();
				if (App::ButtonValue(App::Button::A))
					cam.position -= moveSpeed * Math::LinTran3D::RightVector(cam.rotation) * Time::Delta();
				if (App::ButtonValue(App::Button::Space))
					cam.position.y += moveSpeed * Time::Delta();
				if (App::ButtonValue(App::Button::LeftCtrl))
					cam.position.y -= moveSpeed * Time::Delta();
			}

			for (auto& joystick : joysticks)
			{
				if (joystick.isClicked)
				{
					// Find vector between circle start position, and current position
					// Then apply the logic
					Math::Vec2Int relativeVector = joystick.currentPosition - joystick.startPosition;
					Math::Vec2 relativeVectorFloat = { (f32)relativeVector.x, (f32)relativeVector.y };
					relativeVectorFloat.x /= joystickPixelRadius;
					relativeVectorFloat.y /= joystickPixelRadius;

					f32 moveSpeed = 5.f;
					relativeVectorFloat *= Time::Delta() * moveSpeed;
					cam.position.x += relativeVectorFloat.x;
					cam.position.z += relativeVectorFloat.y;
				}
				else
					joystick.currentPosition = joystick.startPosition;
			}
		}

		virtual void Render(
			Gui::Context& ctx,
			Gui::Extent framebufferExtent,
			Gui::Rect widgetRect,
			Gui::DrawInfo& drawInfo) const override
		{
			// First draw the viewport.
			Gfx::GuiDrawCmd drawCmd{};
			drawCmd.type = Gfx::GuiDrawCmd::Type::Viewport;
			drawCmd.viewport.id = viewportId;
			drawCmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
			drawCmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
			drawCmd.rectExtent.x = (f32)widgetRect.extent.width / framebufferExtent.width;
			drawCmd.rectExtent.y = (f32)widgetRect.extent.height / framebufferExtent.height;
			drawInfo.drawCmds.push_back(drawCmd);

			// Draw a circle, start from the top, move clockwise
			Gfx::GuiDrawCmd::MeshSpan circleMeshSpan{};
			{
				uSize circleVertexCount = 30;
				circleMeshSpan.vertexOffset = (u32)drawInfo.vertices.size();
				circleMeshSpan.indexOffset = (u32)drawInfo.indices.size();
				circleMeshSpan.indexCount = circleVertexCount * 3;
				// Create the vertices, we insert the middle vertex first.
				drawInfo.vertices.push_back({});
				for (uSize i = 0; i < circleVertexCount; i++)
				{
					Gfx::GuiVertex newVertex{};
					f32 currentRadians = 2 * Math::pi / circleVertexCount * (i-1);
					newVertex.position.x += Math::Sin(currentRadians);
					newVertex.position.y += Math::Cos(currentRadians);
					drawInfo.vertices.push_back(newVertex);
				}
				// Build indices
				for (uSize i = 0; i < circleVertexCount - 1; i++)
				{
					drawInfo.indices.push_back(i + 1);
					drawInfo.indices.push_back(0);
					drawInfo.indices.push_back(i + 2);
				}
				drawInfo.indices.push_back(circleVertexCount);
				drawInfo.indices.push_back(0);
				drawInfo.indices.push_back(1);
			}
			// Draw both joysticks using said circle mesh
			for (auto const& joystick : joysticks)
			{
				Gfx::GuiDrawCmd cmd{};
				cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
				cmd.filledMesh.color = { 1.f, 1.f, 1.f, 1.f };
				cmd.filledMesh.mesh = circleMeshSpan;
				cmd.rectPosition.x = (f32)joystick.currentPosition.x / framebufferExtent.width;
				cmd.rectPosition.y = (f32)joystick.currentPosition.y / framebufferExtent.height;
				cmd.rectExtent.x = (f32)joystickPixelRadius / framebufferExtent.width;
				cmd.rectExtent.y = (f32)joystickPixelRadius / framebufferExtent.height;
				drawInfo.drawCmds.push_back(cmd);
			}
		}

		// Pixel space
	  void UpdateCircleStartPosition(Gui::Rect widgetRect)
		{
			joysticks[0].startPosition.x = widgetRect.position.x + joystickPixelRadius * 2;
			joysticks[0].startPosition.y = widgetRect.position.y + widgetRect.extent.height - joystickPixelRadius * 2;

			joysticks[1].startPosition.x = widgetRect.position.x + widgetRect.extent.width - joystickPixelRadius * 2;
			joysticks[1].startPosition.y = widgetRect.position.y + widgetRect.extent.height - joystickPixelRadius * 2;
		}
	};

	class TransformWidget : public Gui::StackLayout
	{
	public:
		TransformWidget(Entity entityId, Scene* scene)
		{
			direction = Direction::Vertical;

			Std::Opt<uSize> transformIndex = scene->transforms.FindIf(
				[entityId](Std::Pair<Entity, Transform> const& value) -> bool {
					return value.a == entityId; });
			Transform& transform = scene->transforms[transformIndex.Value()].b;

			// Create the horizontal position stuff layout
			Gui::StackLayout* positionLayout = new StackLayout(StackLayout::Direction::Horizontal);
			this->AddLayout2(positionLayout);

			// Create the Position: text
			Gui::Text* positionText = new Gui::Text;
			positionLayout->AddWidget2(positionText);
			positionText->text = "Position: ";

			// Create the Position input field
			Gui::LineEdit* positionInputX = new Gui::LineEdit;
			positionLayout->AddWidget2(positionInputX);
			positionInputX->type = Gui::LineEdit::Type::Float;
			positionInputX->text = std::to_string(transform.position.x);
			positionInputX->textChangedPfn = [entityId, scene](Gui::LineEdit& widget)
			{
				Std::Opt<uSize> transformIndex = scene->transforms.FindIf(
					[entityId](Std::Pair<Entity, Transform> const& value) -> bool {
						return entityId == value.a; });
				Transform& transform = scene->transforms[transformIndex.Value()].b;
				if (!widget.text.empty())
				{
					if (widget.text.size() == 1 && widget.text.front() == '-')
						return;
					transform.position.x = std::stof(widget.text);
				}
				else
					transform.position.x = 0;
			};

			// Create the Position input field
			Gui::LineEdit* positionInputY = new Gui::LineEdit;
			positionLayout->AddWidget2(positionInputY);
			positionInputY->text = std::to_string(transform.position.y);
			positionInputY->textChangedPfn = [entityId, scene](Gui::LineEdit& widget)
			{
				Std::Opt<uSize> transformIndex = scene->transforms.FindIf(
					[entityId](Std::Pair<Entity, Transform> const& value) -> bool {
						return entityId == value.a; });
				Transform& transform = scene->transforms[transformIndex.Value()].b;
				if (!widget.text.empty())
				{
					if (widget.text.size() == 1 && widget.text.front() == '-')
						return;
					transform.position.y = std::stof(widget.text);
				}
				else
					transform.position.y = 0;
			};

			Gui::LineEdit* positionInputZ = new Gui::LineEdit;
			positionLayout->AddWidget2(positionInputZ);
			positionInputZ->text = std::to_string(transform.position.z);
			positionInputZ->textChangedPfn = [entityId, scene](Gui::LineEdit& widget)
			{
				Std::Opt<uSize> transformIndex = scene->transforms.FindIf(
					[entityId](Std::Pair<Entity, Transform> const& value) -> bool {
						return entityId == value.a; });
				Transform& transform = scene->transforms[transformIndex.Value()].b;
				if (!widget.text.empty())
				{
					if (widget.text.size() == 1 && widget.text.front() == '-')
						return;
					transform.position.z = std::stof(widget.text);
				}
				else
					transform.position.z = 0;
			};
		}
	};

	class TextureIdWidget : public Gui::StackLayout
	{
	public:
		TextureIdWidget(Entity entityId, Scene* scene)
		{
			direction = Direction::Vertical;

			Std::Opt<uSize> componentIndex = scene->textureIDs.FindIf(
				[entityId](Std::Pair<Entity, Gfx::TextureID> const& value) -> bool {
					return value.a == entityId; });
			Gfx::TextureID& textureId = scene->textureIDs[componentIndex.Value()].b;

			// Create the horizontal position stuff layout
			Gui::StackLayout* positionLayout = new StackLayout(StackLayout::Direction::Horizontal);
			this->AddLayout2(positionLayout);

			// Create the Position: text
			Gui::Text* positionText = new Gui::Text;
			positionLayout->AddWidget2(positionText);
			positionText->text = "Texture: ";

			// Create the Position input field
			Gui::LineEdit* positionInputX = new Gui::LineEdit;
			positionLayout->AddWidget2(positionInputX);
			positionInputX->type = Gui::LineEdit::Type::Integer;
			positionInputX->text = std::to_string((int)textureId);
			positionInputX->textChangedPfn = [entityId, scene](Gui::LineEdit& widget)
			{
				if (!widget.text.empty())
				{
					Std::Opt<uSize> transformIndex = scene->textureIDs.FindIf(
						[entityId](Std::Pair<Entity, Gfx::TextureID> const& value) -> bool {
							return entityId == value.a; });
					Gfx::TextureID& texId = scene->textureIDs[transformIndex.Value()].b;
					texId = (Gfx::TextureID)std::stoi(widget.text);
				}
			};
		}
	};

	class EntityIdList : public Gui::StackLayout
	{
	public:
		EntityIdList(ContextImpl* ctxImpl) :
			ctxImpl(ctxImpl)
		{
			ctxImpl->entityIdList = this;

			this->direction = Direction::Vertical;

			Gui::StackLayout* topElementLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			this->AddLayout2({ topElementLayout });
			this->SetChildRelativeSize(0, 0.1f, true);

			Gui::Text* entityIdText = new Gui::Text;
			topElementLayout->AddWidget2({ entityIdText });
			entityIdText->text = "Entities";

			Gui::Button* newEntityButton = new Gui::Button;
			topElementLayout->AddWidget2({ newEntityButton });
			newEntityButton->textWidget.text = "New";
			newEntityButton->pUserData = this;
			newEntityButton->activatePfn = [](Gui::Button& btn)
			{
				EntityIdList* entityList = reinterpret_cast<EntityIdList*>(btn.pUserData);
				entityList->AddEntityToList(entityList->ctxImpl->scene->NewEntity());
			};
			
			entitiesListLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
			this->AddLayout2({ entitiesListLayout });

			for (uSize i = 0; i < ctxImpl->scene->entities.Size(); i++)
			{
				Entity entityId = ctxImpl->scene->entities[i];

				AddEntityToList(entityId);
			}
		}

		DEngine::Gui::StackLayout* entitiesListLayout = nullptr;
		ContextImpl* ctxImpl = nullptr;

		struct EntityDeleteButtonData
		{
			EntityIdList* idList;
			Entity entityId;

			EntityDeleteButtonData(EntityIdList* idList, Entity id) :
				idList(idList),
				entityId(id) {}
		};
		struct EntitySelectButtonData
		{
			EntityIdList* idList;
			Entity entityId;

			EntitySelectButtonData(EntityIdList* idList, Entity id) :
				idList(idList),
				entityId(id) {}
		};

		void AddEntityToList(Entity id)
		{
			Gui::StackLayout* entityListItemLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			entitiesListLayout->AddLayout2({ entityListItemLayout });

			Gui::Text* textWidget = new Gui::Text;
			entityListItemLayout->AddWidget2({ textWidget });
			textWidget->text = std::to_string((u64)id);

			Gui::Button* entitySelectButton = new Gui::Button;
			entityListItemLayout->AddWidget2({ entitySelectButton });
			entitySelectButton->textWidget.text = "Select";
			EntitySelectButtonData* selectButtonData = new EntitySelectButtonData(this, id);
			entitySelectButton->pUserData = selectButtonData;
			entitySelectButton->activatePfn = [](Gui::Button& btn)
			{
				EntitySelectButtonData buttonData = *reinterpret_cast<EntitySelectButtonData*>(btn.pUserData);
				buttonData.idList->ctxImpl->SelectEntity(buttonData.entityId);
			};

			Gui::Button* entityDeleteButton = new Gui::Button;
			entityListItemLayout->AddWidget2({ entityDeleteButton });
			entityDeleteButton->textWidget.text = "Delete";
			EntityDeleteButtonData* deleteButtonData = new EntityDeleteButtonData(this, id);
			entityDeleteButton->pUserData = deleteButtonData;
			entityDeleteButton->activatePfn = [](Gui::Button& btn)
			{
				EntityDeleteButtonData buttonData = *reinterpret_cast<EntityDeleteButtonData*>(btn.pUserData);
				buttonData.idList->RemoveEntityFromList(buttonData.entityId);
				ContextImpl& ctxImpl = *buttonData.idList->ctxImpl;
				ctxImpl.scene->DeleteEntity(buttonData.entityId);
				if (ctxImpl.selectedEntity.HasValue() && ctxImpl.selectedEntity.Value() == buttonData.entityId)
					buttonData.idList->ctxImpl->UnselectEntity();
			};
		}

		void RemoveEntityFromList(Entity id)
		{
			for (uSize i = 0; i < entitiesListLayout->children.size(); i++)
			{
				Gui::StackLayout& itemLayout = *static_cast<Gui::StackLayout*>(entitiesListLayout->children[i].layout.Get());
				Gui::Button& btn = *static_cast<Gui::Button*>(itemLayout.children.back().widget.Get());
				EntityDeleteButtonData buttonData = *reinterpret_cast<EntityDeleteButtonData*>(btn.pUserData);
				if (id == buttonData.entityId)
				{
					entitiesListLayout->RemoveItem((u32)i);
					ctxImpl->scene->DeleteEntity(id);
					break;
				}
			}
		}

		void SelectEntity(Std::Opt<Entity> previousId, Entity newId)
		{
			if (previousId.HasValue())
			{
				UnselectEntity(previousId.Value());
			}

			for (auto& item : entitiesListLayout->children)
			{
				Gui::StackLayout& itemLayout = *static_cast<Gui::StackLayout*>(item.layout.Get());
				Gui::Button& btn = *static_cast<Gui::Button*>(itemLayout.children.back().widget.Get());
				EntityDeleteButtonData buttonData = *reinterpret_cast<EntityDeleteButtonData*>(btn.pUserData);
				if (buttonData.entityId == newId)
				{
					itemLayout.color.w = 0.2f;
					break;
				}
			}
		}

		void UnselectEntity(Entity id)
		{
			for (auto& item : entitiesListLayout->children)
			{
				Gui::StackLayout& itemLayout = *static_cast<Gui::StackLayout*>(item.layout.Get());
				Gui::Button& btn = *static_cast<Gui::Button*>(itemLayout.children.back().widget.Get());
				EntityDeleteButtonData buttonData = *reinterpret_cast<EntityDeleteButtonData*>(btn.pUserData);
				if (buttonData.entityId == id)
				{
					itemLayout.color.w = 0.f;
					break;
				}
			}
		}
	};

	class ComponentList : public Gui::StackLayout
	{
	public:
		ContextImpl* ctxImpl = nullptr;
		Gui::StackLayout* componentWidgetListLayout = nullptr;

		ComponentList(ContextImpl* ctxImpl) :
			ctxImpl(ctxImpl)
		{
			ctxImpl->componentList = this;

			direction = Direction::Vertical;

			Gui::Text* componentsText = new Gui::Text;
			this->AddWidget2({ componentsText });
			this->SetChildRelativeSize(0, 0.1f, true);
			componentsText->text = "Components";

			componentWidgetListLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
			this->AddLayout2({ componentWidgetListLayout });
		}



	private:
		void HandleTransformComponent(Entity id)
		{
			// Add a Transform toggle button,
// check if Entity has a Transform component
			Gui::Button* transformButton = new Gui::Button;
			componentWidgetListLayout->AddWidget2({ transformButton });
			componentWidgetListLayout->SetChildRelativeSize((u32)componentWidgetListLayout->children.size() - 1, 0.1f, true);
			transformButton->textWidget.text = "Transform";
			transformButton->type = Gui::Button::Type::Toggle;
			Scene* scene = this->ctxImpl->scene;
			transformButton->activatePfn = [id, scene, this](Gui::Button& btn)
			{
				if (btn.GetToggled())
				{
					// Add the transform component widget.
					scene->transforms.PushBack(Std::Pair<Entity, Transform>{ id, Transform() });

					TransformWidget* transformWidget = new TransformWidget(id, scene);
					componentWidgetListLayout->InsertLayout(1, transformWidget);
				}
				else
				{
					// Remove the transform component widget.
					// Find the transform button, remove the component after it.
					componentWidgetListLayout->RemoveItem(1);
					for (uSize i = 0; i < scene->transforms.Size(); i++)
					{
						if (scene->transforms[i].a == id)
						{
							scene->transforms.Erase(i);
							break;
						}
					}
				}
			};

			// We check if the entity already has a Transform
			// If it does, we set the transform button to toggled state
			// And add the Transform component widget.
			Std::Opt<uSize> transformIndex = ctxImpl->scene->transforms.FindIf(
				[id](Std::Pair<Entity, Transform> const& item) -> bool {
					return item.a == id; });
			if (transformIndex.HasValue())
			{
				Transform& transform = ctxImpl->scene->transforms[transformIndex.Value()].b;
				transformButton->SetToggled(true);

				TransformWidget* transformWidget = new TransformWidget(id, ctxImpl->scene);
				componentWidgetListLayout->AddLayout2(transformWidget);
			}
		}

		void HandleTextureIdComponent(Entity id)
		{
			// Add a Transform toggle button,
			// check if Entity has a Transform component
			Gui::Button* transformButton = new Gui::Button;
			componentWidgetListLayout->AddWidget2({ transformButton });
			componentWidgetListLayout->SetChildRelativeSize((u32)componentWidgetListLayout->children.size() - 1, 0.1f, true);
			transformButton->textWidget.text = "SpriteRenderer2D";
			transformButton->type = Gui::Button::Type::Toggle;
			Scene* scene = this->ctxImpl->scene;
			Gui::StackLayout* componentWidgetListLayout = this->componentWidgetListLayout;
			transformButton->activatePfn = [id, scene, componentWidgetListLayout](Gui::Button& btn)
			{
				if (btn.GetToggled())
				{
					// Add the transform component widget.
					scene->textureIDs.PushBack(Std::Pair<Entity, Gfx::TextureID>{ id, Gfx::TextureID() });

					TextureIdWidget* transformWidget = new TextureIdWidget(id, scene);
					componentWidgetListLayout->AddLayout2(transformWidget);
				}
				else
				{
					
					// Remove the transform component widget.
					// Find the transform button, remove the component after it.
					componentWidgetListLayout->RemoveItem((u32)componentWidgetListLayout->children.size() - 1);
					for (uSize i = 0; i < scene->textureIDs.Size(); i++)
					{
						if (scene->textureIDs[i].a == id)
						{
							scene->textureIDs.Erase(i);
							break;
						}
					}
					
				}
			};

			// We check if the entity already has a Transform
			// If it does, we set the transform button to toggled state
			// And add the Transform component widget.
			Std::Opt<uSize> componentIndex = ctxImpl->scene->textureIDs.FindIf(
				[id](Std::Pair<Entity, Gfx::TextureID> const& item) -> bool {
					return item.a == id; });
			if (componentIndex.HasValue())
			{
				transformButton->SetToggled(true);

				TextureIdWidget* transformWidget = new TextureIdWidget(id, scene);
				componentWidgetListLayout->AddLayout2(transformWidget);
			}
		}
	public:
		void EntitySelected(Entity id)
		{
			Clear();

			HandleTransformComponent(id);

			HandleTextureIdComponent(id);
		}

		void Clear()
		{
			componentWidgetListLayout->ClearChildren();
		}
	};
}

namespace DEngine::Editor::detail
{
	struct Gfx_App_Connection : public Gfx::WsiInterface
	{
		App::WindowID appWindowID{};

		// Return type is VkResult
		//
		// Argument #1: VkInstance - The Vulkan instance handle
		// Argument #2: VkAllocationCallbacks const* - Allocation callbacks for surface creation.
		// Argument #3: VkSurfaceKHR* - The output surface handle
		virtual i32 CreateVkSurface(uSize vkInstance, void const* allocCallbacks, u64& outSurface) override
		{
			auto resultOpt = App::CreateVkSurface(appWindowID, vkInstance, nullptr);
			if (resultOpt.HasValue())
			{
				outSurface = resultOpt.Value();
				return 0; // 0 is VK_RESULT_SUCCESS
			}
			else
				return -1;
		}
	};
}

using namespace DEngine;

Editor::Context Editor::Context::Create(
	App::WindowID mainWindow,
	Scene* scene,
	Gfx::Context* gfxCtx)
{
	Context newCtx;

	newCtx.implData = new ContextImpl;
	ContextImpl& implData = *static_cast<ContextImpl*>(newCtx.implData);
	implData.guiCtx = new DEngine::Gui::Context(DEngine::Gui::Context::Create(gfxCtx));
	implData.gfxCtx = gfxCtx;
	implData.scene = scene;
	App::InsertEventInterface(implData);

	{
		Gui::StackLayout* outmostLayout = implData.guiCtx->outerLayout;
		// Delta time counter at the top
		Gui::Text* deltaText = new Gui::Text;
		deltaText->text = "Test text";
		outmostLayout->AddWidget2(deltaText);
		outmostLayout->SetChildRelativeSize(0, 0.05f, true);

		Gui::StackLayout* innerHorizLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
		outmostLayout->AddLayout2(innerHorizLayout);

		EntityIdList* entityIdList = new EntityIdList(&implData);
		innerHorizLayout->AddLayout2(entityIdList);

		ViewportWidget* viewport = new ViewportWidget(implData, *implData.gfxCtx);
		innerHorizLayout->AddWidget2(viewport);
		innerHorizLayout->SetChildRelativeSize(1, 0.5f, true);
		
		ComponentList* componentList = new ComponentList(&implData);
		innerHorizLayout->AddLayout2(componentList);

		Gui::Text* logWidget = new Gui::Text;
		outmostLayout->AddWidget2({ logWidget });
		logWidget->text = "Log widget goes here";
		outmostLayout->SetChildRelativeSize(2, 0.2f, true);
	}
	

	return newCtx;
}

void Editor::Context::ProcessEvents()
{
	ContextImpl& implData = *static_cast<ContextImpl*>(this->implData);

	implData.guiCtx->ProcessEvents();
}

Editor::Context::Context(Context&& other) noexcept :
	implData{ other.implData }
{
	other.implData = nullptr;
}

Editor::Context::~Context()
{
	if (this->implData)
	{
		ContextImpl& implData = *static_cast<ContextImpl*>(this->implData);
		App::RemoveEventInterface(implData);
		delete &implData;
	}
}

Editor::DrawInfo Editor::Context::GetDrawInfo() const
{
	ContextImpl& implData = *static_cast<ContextImpl*>(this->implData);

	DrawInfo returnVal;
	
	returnVal.vertices = implData.guiCtx->vertices;
	returnVal.indices = implData.guiCtx->indices;
	returnVal.drawCmds = implData.guiCtx->drawCmds;

	returnVal.windowUpdates = implData.guiCtx->windowUpdates;

	if (implData.viewportWidget)
	{
		Gfx::ViewportUpdate update{};
		update.id = implData.viewportWidget->viewportId;
		update.width = implData.viewportWidget->currentExtent.width;
		update.height = implData.viewportWidget->currentExtent.height;

		Math::Mat4 camMat = Math::LinTran3D::Rotate_Homo(implData.viewportWidget->cam.rotation);
		Math::LinTran3D::SetTranslation(camMat, implData.viewportWidget->cam.position);
		camMat = camMat.GetInverse().Value();
		f32 aspectRatio = (f32)update.width / update.height;
		camMat = Math::LinTran3D::Perspective_RH_ZO(implData.viewportWidget->cam.verticalFov * Math::degToRad, aspectRatio, 0.1f, 100.f) * camMat;
		update.transform = camMat;

		returnVal.viewportUpdates.push_back(update);
	}

	return returnVal;
}

void Editor::ContextImpl::SelectEntity(Entity id)
{
	// Update the entity list
	entityIdList->SelectEntity(selectedEntity, id);

	// Update the component list
	componentList->EntitySelected(id);

	selectedEntity = id;
}

void Editor::ContextImpl::UnselectEntity()
{
	// Update the entity list
	if (selectedEntity.HasValue())
		entityIdList->UnselectEntity(selectedEntity.Value());

	// Clear the component list
	componentList->Clear();

	selectedEntity = Std::nullOpt;
}

void Editor::ContextImpl::WindowResize(
	App::WindowID window,
	App::Extent newExtent)
{
	Gui::WindowResizeEvent event{};
	event.windowId = (Gui::WindowID)window;
	event.extent = { newExtent.width, newExtent.height };
	guiCtx->PushEvent(event);
}

void Editor::ContextImpl::WindowMove(
	App::WindowID window,
	Math::Vec2Int position)
{
	Gui::WindowMoveEvent event{};
	event.windowId = (Gui::WindowID)window;
	event.position = position;
	guiCtx->PushEvent(event);
}

void Editor::ContextImpl::WindowCursorEnter(
	App::WindowID window,
	bool entered)
{
	Gui::WindowCursorEnterEvent event{};
	event.windowId = (Gui::WindowID)window;
	event.entered = entered;
	guiCtx->PushEvent(event);
}

void Editor::ContextImpl::CursorMove(
	Math::Vec2Int position,
	Math::Vec2Int positionDelta)
{
	Gui::CursorMoveEvent event{};
	event.position = position;
	event.positionDelta = positionDelta;
	guiCtx->PushEvent(event);
}

void Editor::ContextImpl::ButtonEvent(
	App::Button button,
	bool state)
{
	if (button == App::Button::LeftMouse || button == App::Button::RightMouse)
	{
		Gui::CursorClickEvent event{};
		if (button == App::Button::LeftMouse)
			event.button = Gui::CursorButton::Left;
		else if (button == App::Button::RightMouse)
			event.button = Gui::CursorButton::Right;
		event.clicked = state;
		guiCtx->PushEvent(event);
	}
}

void Editor::ContextImpl::CharEvent(
	u32 value)
{
	Gui::CharEvent event{};
	event.utfValue = value;
	guiCtx->PushEvent(event);
}

void Editor::ContextImpl::CharRemoveEvent()
{
	Gui::CharRemoveEvent event{};
	guiCtx->PushEvent(event);
}

void Editor::ContextImpl::TouchEvent(
	u8 id,
	App::TouchEventType type,
	Math::Vec2 position)
{
	Gui::TouchEvent event{};
	event.id = id;
	event.position = position;
	if (type == App::TouchEventType::Down)
		event.type = Gui::TouchEventType::Down;
	else if (type == App::TouchEventType::Moved)
		event.type = Gui::TouchEventType::Moved;
	else if (type == App::TouchEventType::Up)
		event.type = Gui::TouchEventType::Up;
	guiCtx->PushEvent(event);
}