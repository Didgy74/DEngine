#pragma once

#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/MenuBar.hpp>

#include "EditorImpl.hpp"

#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <iostream>

namespace DEngine::Editor
{
	namespace Gizmo
	{
		struct Test_Ray
		{
			Math::Vec3 origin{};
			Math::Vec3 direction{};
		};

		enum class GizmoPart
		{
			ArrowX,
			ArrowY,
			PlaneXY
		};

		struct Arrow
		{
			f32 capSize = 0;
			f32 capLength = 0;
			f32 shaftLength = 0;
			f32 shaftDiameter = 0;
		};

		namespace impl
		{
			[[nodiscard]] constexpr Arrow BuildDefaultArrow() noexcept
			{
				Arrow arrow{};
				arrow.capLength = 1 / 3.f;
				arrow.capSize = 0.2f;
				arrow.shaftDiameter = 0.1f;
				arrow.shaftLength = 2 / 3.f;
				return arrow;
			}
		}

		constexpr Arrow defaultArrow = impl::BuildDefaultArrow();

		constexpr f32 defaultGizmoSizeRelative = 0.33f;
		// Size is relative to the gizmo size
		constexpr f32 defaultPlaneScaleRelative = 0.25f;
		// Relative to the gizmo size
		constexpr f32 defaultPlaneOffsetRelative = 0.25f;

		// target_size in pixels
		[[nodiscard]] f32 ComputeScale(
			Math::Mat4 const& worldTransform,
			u32 targetSizePx,
			Math::Mat4 const& projection,
			Gui::Extent viewportSize) noexcept;
	}

	class InternalViewportWidget : public Gui::Widget
	{
	public:
		Gfx::ViewportID viewportId = Gfx::ViewportID::Invalid;
		Gfx::Context* gfxCtx = nullptr;
		EditorImpl* editorImpl = nullptr;

		mutable bool isVisible = false;

		mutable Gui::Extent currentExtent{};
		mutable Gui::Extent newExtent{};
		mutable bool currentlyResizing = false;
		mutable u32 extentCorrectTickCounter = 0;
		mutable bool extentsAreInitialized = false;

		bool isCurrentlyClicked = false;
		bool isCurrentlyHoldingGizmo = false;
		Std::Opt<u8> gizmo_touchID{};
		Gizmo::GizmoPart gizmo_holdingPart{};
		Gizmo::Test_Ray gizmo_initialRay{};
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

		InternalViewportWidget(EditorImpl& implData, Gfx::Context& gfxCtxIn);

		virtual ~InternalViewportWidget() override;

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
			Gui::Context const& ctx) const override;

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
		ViewportWidget(EditorImpl& implData, Gfx::Context& ctx);
	};
}