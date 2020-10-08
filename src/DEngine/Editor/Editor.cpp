#include "ViewportWidget.hpp"

#include "Editor.hpp"
#include "ContextImpl.hpp"



#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/DockArea.hpp>
#include <DEngine/Gui/Image.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/ScrollArea.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Text.hpp>
#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Application.hpp>
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
	class TransformWidget : public Gui::StackLayout
	{
	public:
		TransformWidget(Entity entityId, Scene* scene)
		{
			direction = Direction::Vertical;

			auto transformIt = std::find_if(
				scene->transforms.begin(),
				scene->transforms.end(),
				[entityId](decltype(scene->transforms)::value_type const& value) -> bool { return value.a == entityId; });

			Transform& transform = transformIt->b;

			// Create the horizontal position stuff layout
			Gui::StackLayout* positionLayout = new StackLayout(StackLayout::Direction::Horizontal);
			this->AddLayout2(Std::Box<Layout>{ positionLayout });
			positionLayout->spacing = 10;

			// Create the Position: text
			Gui::Text* positionText = new Gui::Text;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionText });
			positionText->String_Set("Position: ");

			// Create the Position input field
			Gui::LineEdit* positionInputX = new Gui::LineEdit;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionInputX });
			positionInputX->type = Gui::LineEdit::Type::Float;
			positionInputX->String_Set(std::to_string(transform.position.x).c_str());
			positionInputX->textChangedPfn = [entityId, scene](Gui::LineEdit& widget)
			{
				auto transformIt = std::find_if(
					scene->transforms.begin(),
					scene->transforms.end(),
					[entityId](decltype(scene->transforms)::value_type const& value) -> bool { return entityId == value.a; });

				Transform& transform = transformIt->b;
				transform.position.x = std::stof(widget.StringView().data());
			};

			// Create the Position input field
			Gui::LineEdit* positionInputY = new Gui::LineEdit;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionInputY });
			positionInputY->String_Set(std::to_string(transform.position.y).c_str());
			positionInputY->textChangedPfn = [entityId, scene](Gui::LineEdit& widget)
			{
				auto transformIt = std::find_if(
					scene->transforms.begin(),
					scene->transforms.end(),
					[entityId](decltype(scene->transforms)::value_type const& value) -> bool { return entityId == value.a; });

				Transform& transform = transformIt->b;
				transform.position.y = std::stof(widget.StringView().data());
			};

			Gui::LineEdit* positionInputZ = new Gui::LineEdit;
			positionLayout->AddWidget2(Std::Box<Gui::Widget>{ positionInputZ });
			positionInputZ->String_Set(std::to_string(transform.position.z).data());
			positionInputZ->textChangedPfn = [entityId, scene](Gui::LineEdit& widget)
			{
				auto transformIt = std::find_if(
					scene->transforms.begin(),
					scene->transforms.end(),
					[entityId](decltype(scene->transforms)::value_type const& value) -> bool { return entityId == value.a; });

				Transform& transform = transformIt->b;
				transform.position.z = std::stof(widget.StringView().data());
			};
		}
	};

	class TextureIdWidget : public Gui::StackLayout
	{
	public:
		TextureIdWidget(Entity entityId, Scene* scene)
		{
			direction = Direction::Vertical;

			auto componentIt = std::find_if(
				scene->textureIDs.begin(),
				scene->textureIDs.end(),
				[entityId](decltype(scene->textureIDs)::value_type const& value) -> bool { return value.a == entityId; });

			Gfx::TextureID& textureId = componentIt->b;

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
			positionInputX->String_Set(std::to_string((int)textureId).c_str());
			positionInputX->textChangedPfn = [entityId, scene](Gui::LineEdit& widget)
			{
				if (!widget.StringView().empty())
				{
					auto transformIt = std::find_if(
						scene->textureIDs.begin(),
						scene->textureIDs.end(),
						[entityId](Std::Pair<Entity, Gfx::TextureID> const& value) -> bool { return entityId == value.a; });
					
					Gfx::TextureID& texId = transformIt->b;
					texId = (Gfx::TextureID)std::stoi(widget.StringView().data());
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
			this->AddLayout2(Std::Box<Gui::Layout>{ topElementLayout });

			Gui::Button* newEntityButton = new Gui::Button;
			topElementLayout->AddWidget2(Std::Box<Gui::Widget>{ newEntityButton });
			newEntityButton->textWidget.String_Set("New");
			newEntityButton->activatePfn = [this](Gui::Button& btn)
			{
				Entity newId = this->ctxImpl->scene->NewEntity();
				AddEntityToList(newId);
			};

			Gui::Button* entityDeleteButton = new Gui::Button;
			topElementLayout->AddWidget2(Std::Box<Gui::Widget>{ entityDeleteButton });
			entityDeleteButton->textWidget.String_Set("Delete");
			entityDeleteButton->activatePfn = [this](Gui::Button& btn)
			{
				if (!this->ctxImpl->selectedEntity.HasValue())
					return;

				Entity selectedEntity = this->ctxImpl->selectedEntity.Value();

				this->ctxImpl->scene->DeleteEntity(selectedEntity);

				this->ctxImpl->UnselectEntity();

				RemoveEntityFromList(selectedEntity);
			};
			
			Gui::ScrollArea* entityListScrollArea = new Gui::ScrollArea();
			this->AddLayout2(Std::Box<Gui::Layout>{ entityListScrollArea });

			entitiesListLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
			entityListScrollArea->childType = Gui::ScrollArea::ChildType::Layout;
			entityListScrollArea->layout = Std::Box<Gui::Layout>{ entitiesListLayout };

			for (uSize i = 0; i < ctxImpl->scene->entities.size(); i++)
			{
				Entity entityId = ctxImpl->scene->entities[i];

				AddEntityToList(entityId);
			}
		}

		Gui::StackLayout* entitiesListLayout = nullptr;
		ContextImpl* ctxImpl = nullptr;
		std::vector<Std::Pair<Entity, Gui::StackLayout*>> entityEntries;

		void AddEntityToList(Entity id)
		{
			Gui::StackLayout* entityListItemLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
			entitiesListLayout->AddLayout2(Std::Box<Gui::Layout>{ entityListItemLayout });
			entityEntries.push_back({ id, entityListItemLayout });

			Gui::Text* textWidget = new Gui::Text;
			entityListItemLayout->AddWidget2(Std::Box<Gui::Widget>{ textWidget });
			textWidget->String_Set(std::to_string((u64)id).c_str());

			Gui::Button* entitySelectButton = new Gui::Button;
			entityListItemLayout->AddWidget2(Std::Box<Gui::Widget>{ entitySelectButton });
			entitySelectButton->textWidget.String_Set("Select");
			entitySelectButton->activatePfn = [this, id](Gui::Button& btn)
			{
				ctxImpl->SelectEntity(id);
			};
		}

		void RemoveEntityFromList(Entity id)
		{
			auto it = Std::FindIf(
				entityEntries.begin(),
				entityEntries.end(),
				[id](decltype(entityEntries)::value_type const& val) -> bool {
					return val.a == id; });
			Gui::StackLayout* targetLayoutPtr = it->b;

			for (u32 i = 0; i < entitiesListLayout->ChildCount(); i++)
			{
				auto item = entitiesListLayout->At(i);
				if (targetLayoutPtr == item.layout)
				{
					entitiesListLayout->RemoveItem(i);
					break;
				}
			}

			entityEntries.erase(it);
		}

		void SelectEntity(Std::Opt<Entity> previousId, Entity newId)
		{
			if (previousId.HasValue())
			{
				UnselectEntity(previousId.Value());
			}

			auto entityEntryIt = Std::FindIf(
				entityEntries.begin(),
				entityEntries.end(),
				[newId](decltype(entityEntries)::value_type const& val) -> bool {
					return val.a == newId; });
			Gui::StackLayout* entityItem = entityEntryIt->b;
			entityItem->color.w = 0.2f;
		}

		void UnselectEntity(Entity id)
		{

			auto entityEntryIt = Std::FindIf(
				entityEntries.begin(),
				entityEntries.end(),
				[id](decltype(entityEntries)::value_type const& val) -> bool {
					return val.a == id; });
			Gui::StackLayout* entityItem = entityEntryIt->b;
			entityItem->color.w = 0.f;
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

			componentWidgetListLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
			this->AddLayout2(Std::Box<Gui::Layout>{ componentWidgetListLayout });
		}

	private:
		void HandleTransformComponent(Entity id)
		{
			// Add a Transform toggle button
			// check if Entity has a Transform component
			Gui::Button* transformButton = new Gui::Button;
			componentWidgetListLayout->AddWidget2(Std::Box<Gui::Widget>{ transformButton });
			transformButton->textWidget.String_Set("Transform");
			transformButton->type = Gui::Button::Type::Toggle;
			transformButton->activatePfn = [id, this](Gui::Button& btn)
			{
				if (btn.GetToggled())
				{
					// Add the transform component widget.
					ctxImpl->scene->transforms.push_back(Std::Pair<Entity, Transform>{ id, Transform() });

					TransformWidget* transformWidget = new TransformWidget(id, ctxImpl->scene);
					componentWidgetListLayout->InsertLayout(1, Std::Box<Gui::Layout>{ transformWidget });
				}
				else
				{
					// Remove the transform component widget.
					// Find the transform button, remove the component after it.
					componentWidgetListLayout->RemoveItem(1);
					auto transformIt = Std::FindIf(
						ctxImpl->scene->transforms.begin(),
						ctxImpl->scene->transforms.end(),
						[id](decltype(ctxImpl->scene->transforms)::value_type const& val) -> bool {
							return val.a == id; });
					ctxImpl->scene->transforms.erase(transformIt);
				}
			};

			// We check if the entity already has a Transform
			// If it does, we set the transform button to toggled state
			// And add the Transform component widget.
			auto transformIt = std::find_if(
				ctxImpl->scene->transforms.begin(),
				ctxImpl->scene->transforms.end(),
				[id](decltype(ctxImpl->scene->transforms)::value_type const& item) -> bool { 
					return item.a == id; });
				
			if (transformIt != ctxImpl->scene->transforms.end())
			{
				Transform& transform = transformIt->b;
				transformButton->SetToggled(true);

				TransformWidget* transformWidget = new TransformWidget(id, ctxImpl->scene);
				componentWidgetListLayout->AddLayout2(Std::Box<Gui::Layout>{ transformWidget });
			}
		}

		void HandleTextureIdComponent(Entity id)
		{
			// Add a Transform toggle button,
			// check if Entity has a Transform component
			Gui::Button* transformButton = new Gui::Button;
			componentWidgetListLayout->AddWidget2(Std::Box<Gui::Widget>{ transformButton });
			transformButton->textWidget.String_Set("SpriteRenderer2D");
			transformButton->type = Gui::Button::Type::Toggle;
			Scene* scene = this->ctxImpl->scene;
			Gui::StackLayout* componentWidgetListLayout = this->componentWidgetListLayout;
			transformButton->activatePfn = [id, scene, componentWidgetListLayout](Gui::Button& btn)
			{
				if (btn.GetToggled())
				{
					// Add the transform component widget.
					scene->textureIDs.push_back(Std::Pair<Entity, Gfx::TextureID>{ id, Gfx::TextureID() });

					TextureIdWidget* transformWidget = new TextureIdWidget(id, scene);
					componentWidgetListLayout->AddLayout2(Std::Box<Gui::Layout>{ transformWidget });
				}
				else
				{
					// Remove the transform component widget.
					// Find the transform button, remove the component after it.
					componentWidgetListLayout->RemoveItem(componentWidgetListLayout->ChildCount() - 1);
					for (uSize i = 0; i < scene->textureIDs.size(); i++)
					{
						if (scene->textureIDs[i].a == id)
						{
							scene->textureIDs.erase(scene->textureIDs.begin() + i);
							break;
						}
					}
					
				}
			};

			// We check if the entity already has a Transform
			// If it does, we set the transform button to toggled state
			// And add the Transform component widget.
			auto componentIt = std::find_if(
				ctxImpl->scene->textureIDs.begin(),
				ctxImpl->scene->textureIDs.end(),
				[id](decltype(ctxImpl->scene->textureIDs)::value_type const& item) -> bool { return item.a == id; });
			
			if (componentIt != ctxImpl->scene->textureIDs.end())
			{
				transformButton->SetToggled(true);

				TextureIdWidget* transformWidget = new TextureIdWidget(id, scene);
				componentWidgetListLayout->AddLayout2(Std::Box<Gui::Layout>{ transformWidget });
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
	implData.guiCtx = Std::Box{ new DEngine::Gui::Context(DEngine::Gui::Context::Create(implData, gfxCtx)) };
	implData.gfxCtx = gfxCtx;
	implData.scene = scene;
	App::InsertEventInterface(implData);

	{
		Gui::StackLayout* outmostLayout = implData.guiCtx->outerLayout;

		// Delta time counter at the top
		Gui::Text* deltaText = new Gui::Text;
		implData.test_fpsText = deltaText;
		deltaText->String_Set("Child text");
		outmostLayout->AddWidget2(Std::Box<Gui::Widget>{ deltaText });

		/*
		Gui::StackLayout* innerHorizLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
		innerHorizLayout->test_expand = true;
		outmostLayout->AddLayout2(innerHorizLayout);

		EntityIdList* entityIdList = new EntityIdList(&implData);
		innerHorizLayout->AddLayout2(entityIdList);

		ViewportWidget* viewport = new ViewportWidget(implData, *implData.gfxCtx);
		innerHorizLayout->AddWidget2(viewport);
		
		ComponentList* componentList = new ComponentList(&implData);
		innerHorizLayout->AddLayout2(componentList);

		Gui::Text* logWidget = new Gui::Text;
		outmostLayout->AddWidget2({ logWidget });
		logWidget->String_Set("Log widget");
		*/

		Gui::DockArea* dockArea = new Gui::DockArea;
		outmostLayout->AddLayout2(Std::Box<Gui::Layout>{ dockArea });

		{
			Gui::DockArea::TopLevelNode newTop{};
			newTop.rect = { { 0, 0 }, { 400, 400 } };
			Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
			newTop.node = Std::Box{ topNode };
			topNode->type = Gui::DockArea::Node::Type::Window;
			topNode->windows.push_back(Gui::DockArea::Window{});
			auto& newWindow = topNode->windows.back();
			newWindow.title = "Entities";
			newWindow.titleBarColor = { 0.5f, 0.5f, 0.f, 1.f };
			EntityIdList* entityIdList = new EntityIdList(&implData);
			newWindow.layout = Std::Box<Gui::Layout>{ entityIdList };

			dockArea->topLevelNodes.emplace_back(Std::Move(newTop));
		}

		{
			Gui::DockArea::TopLevelNode newTop{};
			newTop.rect = { { 150, 150 }, { 400, 400 } };
			Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
			newTop.node = Std::Box{ topNode };
			topNode->type = Gui::DockArea::Node::Type::Window;
			topNode->windows.push_back(Gui::DockArea::Window{});
			auto& newWindow = topNode->windows.back();
			newWindow.title = "Viewport";
			newWindow.titleBarColor = { 0.5f, 0.f, 0.5f, 1.f };
			ViewportWidget* viewport = new ViewportWidget(implData, *implData.gfxCtx);
			newWindow.widget = Std::Box<Gui::Widget>{ viewport };

			dockArea->topLevelNodes.emplace_back(Std::Move(newTop));
		}

		{
			Gui::DockArea::TopLevelNode newTop{};
			newTop.rect = { { 250, 250 }, { 400, 400 } };
			Gui::DockArea::Node* topNode = new Gui::DockArea::Node;
			newTop.node = Std::Box{ topNode };
			topNode->type = Gui::DockArea::Node::Type::Window;
			topNode->windows.push_back(Gui::DockArea::Window{});
			auto& newWindow = topNode->windows.back();
			newWindow.title = "Components";
			newWindow.titleBarColor = { 0.f, 0.5f, 0.5f, 1.f };
			ComponentList* componentList = new ComponentList(&implData);
			newWindow.layout = Std::Box<Gui::Layout>{ componentList };

			dockArea->topLevelNodes.emplace_back(Std::Move(newTop));
		}
	}
	
	return newCtx;
}

void Editor::Context::ProcessEvents()
{
	ContextImpl& implData = *static_cast<ContextImpl*>(this->implData);

	if (App::TickCount() % 60 == 0)
		implData.test_fpsText->String_Set(std::to_string(Time::Delta()).c_str());

	for (auto const& event : implData.queuedGuiEvents)
	{
		switch (event.type)
		{
		case impl::GuiEvent::Type::CharEnterEvent: implData.guiCtx->PushEvent(event.charEnter); break;
		case impl::GuiEvent::Type::CharEvent: implData.guiCtx->PushEvent(event.charEvent); break;
		case impl::GuiEvent::Type::CharRemoveEvent: implData.guiCtx->PushEvent(event.charRemove); break;
		case impl::GuiEvent::Type::CursorClickEvent: implData.guiCtx->PushEvent(event.cursorClick); break;
		case impl::GuiEvent::Type::CursorMoveEvent: implData.guiCtx->PushEvent(event.cursorMove); break;
		case impl::GuiEvent::Type::TouchEvent: implData.guiCtx->PushEvent(event.touch); break;
		case impl::GuiEvent::Type::WindowCloseEvent: implData.guiCtx->PushEvent(event.windowClose); break;
		case impl::GuiEvent::Type::WindowCursorEnterEvent: implData.guiCtx->PushEvent(event.windowCursorEnter); break;
		case impl::GuiEvent::Type::WindowMinimizeEvent: implData.guiCtx->PushEvent(event.windowMinimize); break;
		case impl::GuiEvent::Type::WindowMoveEvent: implData.guiCtx->PushEvent(event.windowMove); break;
		case impl::GuiEvent::Type::WindowResizeEvent: implData.guiCtx->PushEvent(event.windowResize); break;
		}
	}
	implData.queuedGuiEvents.clear();

	if (implData.viewportWidget)
	{
		implData.viewportWidget->isVisible = false;
		// Handle camera movement
		if (implData.viewportWidget->isCurrentlyClicked)
		{
			f32 moveSpeed = 5.f;
			Math::Vec3 moveVector{};
			if (App::ButtonValue(App::Button::W))
				moveVector.z += 1;
			if (App::ButtonValue(App::Button::S))
				moveVector.z -= 1;
			if (App::ButtonValue(App::Button::D))
				moveVector.x += 1;
			if (App::ButtonValue(App::Button::A))
				moveVector.x -= 1;
			if (App::ButtonValue(App::Button::Space))
				moveVector.y += 1;
			if (App::ButtonValue(App::Button::LeftCtrl))
				moveVector.y -= 1;
			implData.viewportWidget->ApplyCameraMovement(moveVector, moveSpeed * Time::Delta());
		}

		implData.viewportWidget->TickTest(Time::Delta());
	}

	implData.guiCtx->Tick();
	implData.guiCtx->Render();
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

	if (implData.viewportWidget && implData.viewportWidget->isVisible)
	{
		DENGINE_DETAIL_ASSERT(implData.viewportWidget->currentExtent.width > 0 && implData.viewportWidget->currentExtent.height > 0);
		DENGINE_DETAIL_ASSERT(implData.viewportWidget->newExtent.width > 0 && implData.viewportWidget->newExtent.height > 0);

		Gfx::ViewportUpdate update{};
		update.id = implData.viewportWidget->viewportId;
		update.width = implData.viewportWidget->currentExtent.width;
		update.height = implData.viewportWidget->currentExtent.height;

		Math::Mat4 test = Math::Mat4::Identity();
		test.At(0, 0) = -1;
		//test.At(1, 1) = -1;
		test.At(2, 2) = -1;
		
		Math::Mat4 camMat = Math::LinTran3D::Rotate_Homo(implData.viewportWidget->cam.rotation) * test;
		Math::LinTran3D::SetTranslation(camMat, implData.viewportWidget->cam.position);

		Math::Mat4 test2 = Math::Mat4::Identity();
		//test2.At(0, 0) = -1;
		//test2.At(1, 1) = -1;
		//test2.At(2, 2) = -1;
		camMat = test2 * camMat;


		camMat = camMat.GetInverse().Value();
		f32 aspectRatio = (f32)implData.viewportWidget->newExtent.width / implData.viewportWidget->newExtent.height;

		camMat = Math::LinTran3D::Perspective_RH_ZO(implData.viewportWidget->cam.verticalFov * Math::degToRad, aspectRatio, 0.1f, 100.f) * camMat;


		//camMat = test * camMat;

		update.transform = camMat;

		returnVal.viewportUpdates.push_back(update);
	}

	return returnVal;
}

void Editor::ContextImpl::SelectEntity(Entity id)
{
	if (selectedEntity.HasValue() && selectedEntity.Value() == id)
		return;

	// Update the entity list
	if (entityIdList)
		entityIdList->SelectEntity(selectedEntity, id);

	// Update the component list
	if (componentList)
		componentList->EntitySelected(id);

	selectedEntity = id;
}

void Editor::ContextImpl::UnselectEntity()
{
	// Update the entity list
	if (selectedEntity.HasValue() && entityIdList)
		entityIdList->UnselectEntity(selectedEntity.Value());

	// Clear the component list
	if (componentList)
		componentList->Clear();

	selectedEntity = Std::nullOpt;
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

void DEngine::Editor::ContextImpl::CharEnterEvent()
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharEnterEvent;
	event.charEnter = {};
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::CharEvent(
	u32 value)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharEvent;
	event.charEvent.utfValue = value;
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::CharRemoveEvent()
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CharRemoveEvent;
	event.charRemove = {};
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::CursorMove(
	Math::Vec2Int position,
	Math::Vec2Int positionDelta)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::CursorMoveEvent;
	event.cursorMove.position = position;
	event.cursorMove.positionDelta = positionDelta;
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::TouchEvent(
	u8 id,
	App::TouchEventType type,
	Math::Vec2 position)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::TouchEvent;
	event.touch.id = id;
	event.touch.position = position;
	if (type == App::TouchEventType::Down)
		event.touch.type = Gui::TouchEventType::Down;
	else if (type == App::TouchEventType::Moved)
		event.touch.type = Gui::TouchEventType::Moved;
	else if (type == App::TouchEventType::Up)
		event.touch.type = Gui::TouchEventType::Up;
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::WindowClose(App::WindowID windowId)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowCloseEvent;
	event.windowClose.windowId = (Gui::WindowID)windowId;
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::WindowCursorEnter(
	App::WindowID window,
	bool entered)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowCursorEnterEvent;
	event.windowCursorEnter.windowId = (Gui::WindowID)window;
	event.windowCursorEnter.entered = entered;
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::WindowMinimize(
	App::WindowID window,
	bool wasMinimized)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowMinimizeEvent;
	event.windowMinimize.windowId = (Gui::WindowID)window;
	event.windowMinimize.wasMinimized = wasMinimized;
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::WindowMove(
	App::WindowID window,
	Math::Vec2Int position)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowMoveEvent;
	event.windowMove.windowId = (Gui::WindowID)window;
	event.windowMove.position = position;
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::WindowResize(
	App::WindowID window,
	App::Extent newExtent,
	Math::Vec2Int visiblePos,
	App::Extent visibleSize)
{
	impl::GuiEvent event{};
	event.type = impl::GuiEvent::Type::WindowResizeEvent;
	event.windowResize.windowId = (Gui::WindowID)window;
	event.windowResize.extent = Gui::Extent{ newExtent.width, newExtent.height };
	event.windowResize.visibleRect = Gui::Rect{ visiblePos, Gui::Extent{ newExtent.width, newExtent.height } };
	queuedGuiEvents.push_back(event);
}

void Editor::ContextImpl::CloseWindow(Gui::WindowID id)
{
	App::DestroyWindow((App::WindowID)id);
}

void Editor::ContextImpl::SetCursorType(Gui::WindowID id, Gui::CursorType cursorType)
{
	App::CursorType appCursorType{};
	switch (cursorType)
	{
	case Gui::CursorType::Arrow:
		appCursorType = App::CursorType::Arrow;
		break;
	case Gui::CursorType::HorizontalResize:
		appCursorType = App::CursorType::HorizontalResize;
		break;
	case Gui::CursorType::VerticalResize:
		appCursorType = App::CursorType::VerticalResize;
		break;
	default:
		throw std::runtime_error("Error. This Gui::CursorType has not been implemented.");
		break;
	}
	App::SetCursor((App::WindowID)id, appCursorType);
}

void Editor::ContextImpl::HideSoftInput()
{
	App::HideSoftInput();
}

void Editor::ContextImpl::OpenSoftInput(
	std::string_view currentText, 
	Gui::SoftInputFilter inputFilter)
{
	App::SoftInputFilter filter{};
	switch (inputFilter)
	{
	case Gui::SoftInputFilter::Float:
		filter = App::SoftInputFilter::Float;
		break;
	case Gui::SoftInputFilter::Integer:
		filter = App::SoftInputFilter::Integer;
		break;
	case Gui::SoftInputFilter::UnsignedInteger:
		filter = App::SoftInputFilter::UnsignedInteger;
		break;
	}
	App::OpenSoftInput(currentText, filter);
}
