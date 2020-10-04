#pragma once

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/Text.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Containers/Box.hpp>
#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Scene.hpp>

#include <unordered_map>
#include <string_view>

namespace DEngine::Editor
{
	class EntityIdList;
	class ComponentList;
	class ViewportWidget;

	struct ContextImpl : public App::EventInterface, public Gui::WindowHandler
	{
		Std::Box<Gui::Context> guiCtx;

		// Override app-interface methods
		virtual void WindowResize(
			App::WindowID window,
			App::Extent newExtent,
			Math::Vec2Int visiblePos,
			App::Extent visibleExtent) override;
		virtual void WindowMove(
			App::WindowID window,
			Math::Vec2Int position) override;
		virtual void WindowMinimize(
			App::WindowID window,
			bool wasMinimized) override;
		virtual void WindowCursorEnter(
			App::WindowID window,
			bool entered) override;
		virtual void CursorMove(
			Math::Vec2Int position,
			Math::Vec2Int positionDelta) override;
		virtual void ButtonEvent(
			App::Button button,
			bool state) override;
		virtual void CharEvent(u32 value) override;
		virtual void CharEnterEvent() override;
		virtual void CharRemoveEvent() override;
		virtual void TouchEvent(
			u8 id,
			App::TouchEventType type,
			Math::Vec2 position) override;


		// Override window-handler methods
		virtual void SetCursorType(Gui::WindowID, Gui::CursorType) override;
		virtual void OpenSoftInput(Gui::WindowID, std::string_view, Gui::SoftInputFilter) override;

		// App-specific stuff
		Gfx::Context* gfxCtx = nullptr;
		Scene* scene = nullptr;
		Gui::Text* test_fpsText = nullptr;
		Std::Opt<Entity> selectedEntity;
		EntityIdList* entityIdList = nullptr;
		ComponentList* componentList = nullptr;
		ViewportWidget* viewportWidget = nullptr;
		void SelectEntity(Entity id);
		void UnselectEntity();
	};
}