#pragma once

#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/MenuBar.hpp>

#include "EditorImpl.hpp"

#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <anton/gizmo/gizmo.hpp>

namespace DEngine::Editor
{
	class InternalViewportWidget : public Gui::Widget
	{
	public:
		Gfx::ViewportID viewportId = Gfx::ViewportID::Invalid;
		Gfx::Context* gfxCtx = nullptr;
		EditorImpl* implData = nullptr;

		mutable bool isVisible = false;

		mutable Gui::Extent currentExtent{};
		mutable Gui::Extent newExtent{};
		mutable bool currentlyResizing = false;
		mutable u32 extentCorrectTickCounter = 0;
		mutable bool extentsAreInitialized = false;

		bool isCurrentlyClicked = false;
		bool isCurrentlyHoldingGizmo = false;
		u8 gizmo_holdingAxis = 0;
		anton::math::Ray gizmo_initialRay{};
		Math::Vec3 gizmo_initialPos{};


		int joystickPixelRadius = 50;
		int joystickPixelDeadZone = 10;
		struct JoyStick
		{
			Std::Opt<u8> touchID{};
			bool isClicked = false;
			mutable Math::Vec2Int originPosition{};
			Math::Vec2Int currentPosition{};
		};
		// 0 is left, 1 is right
		JoyStick joysticks[2]{};

		struct Camera
		{
			Math::Vec3 position{ 0.f, 0.f, 1.f };
			Math::UnitQuat rotation = Math::UnitQuat::FromEulerAngles(0, 180.f, 0.f);
			f32 verticalFov = 60.f;
		};
		Camera cam{};

		InternalViewportWidget(EditorImpl& implData, Gfx::Context& gfxCtxIn) :
			gfxCtx(&gfxCtxIn),
			implData(&implData)
		{
			implData.viewportWidgets.push_back(this);

			auto newViewportRef = gfxCtx->NewViewport();
			viewportId = newViewportRef.ViewportID();
		}

		virtual ~InternalViewportWidget() override
		{
			gfxCtx->DeleteViewport(viewportId);
			auto ptrIt = Std::FindIf(
				implData->viewportWidgets.begin(),
				implData->viewportWidgets.end(),
				[this](decltype(implData->viewportWidgets.front()) const& val) -> bool { return val == this; });
			DENGINE_DETAIL_ASSERT(ptrIt != implData->viewportWidgets.end());
			implData->viewportWidgets.erase(ptrIt);
		}

		[[nodiscard]] static Math::Vec2 GetNormalizedViewportPosition(
			Math::Vec2Int cursorPos,
			Gui::Rect viewportRect) noexcept;

		[[nodiscard]] Math::Mat4 GetViewMatrix() const noexcept;

		[[nodiscard]] Math::Mat4 GetPerspectiveMatrix(f32 aspectRatio) const noexcept;

		Math::Mat4 GetProjectionMatrix(f32 aspectRatio) const noexcept;

		void ApplyCameraRotation(Math::Vec2 input) noexcept;

		void ApplyCameraMovement(Math::Vec3 move, f32 speed) noexcept;

		Math::Vec2 GetLeftJoystickVec() const noexcept;

		Math::Vec2 GetRightJoystickVec() const noexcept;

		void TickTest(f32 deltaTime) noexcept;

		// Pixel space
		void UpdateJoystickOrigin(Gui::Rect widgetRect) const noexcept;

		virtual void CursorClick(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Math::Vec2Int cursorPos,
			Gui::CursorClickEvent event) override;

		virtual void CursorMove(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::CursorMoveEvent event) override;

		virtual void TouchEvent(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::TouchEvent touch) override;

		virtual Gui::SizeHint GetSizeHint(
			Gui::Context const& ctx) const override
		{
			Gui::SizeHint returnVal{};
			returnVal.preferred = { 450, 450 };
			returnVal.expandX = true;
			returnVal.expandY = true;
			return returnVal;
		}

		virtual void Render(
			Gui::Context const& ctx,
			Gui::Extent framebufferExtent,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::DrawInfo& drawInfo) const override;
	};


	class ViewportWidget : public Gui::StackLayout 
	{
	public:
		ViewportWidget(EditorImpl& implData, Gfx::Context& ctx) :
			Gui::StackLayout(Gui::StackLayout::Direction::Vertical)
		{
			// Generate top navigation bar
			Gui::MenuBar* menuBar = new Gui::MenuBar(Gui::MenuBar::Direction::Horizontal);
			AddLayout2(Std::Box<Gui::Layout>{ menuBar });
			
			menuBar->AddSubmenuButton(
				"Camera",
				[](
					Gui::MenuBar& newMenuBar)
				{
					newMenuBar.AddMenuButton(
						"FreeLook",
						[]()
						{

						});
				});

			InternalViewportWidget* viewport = new InternalViewportWidget(implData, ctx);
			AddWidget2(Std::Box<Gui::Widget>{ viewport });
		}
	};
}