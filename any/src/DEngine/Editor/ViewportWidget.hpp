#pragma once

#include "Editor.hpp"
#include "EditorImpl.hpp"

#include <DEngine/Editor/Joystick.hpp>

#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/MenuBar.hpp>

#include <DEngine/Std/Utility.hpp>

#include <DEngine/Math/Matrix.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Math/Vector.hpp>

namespace DEngine::Editor
{
	namespace Gizmo
	{
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
				Arrow arrow = {};
				arrow.capLength = 1 / 3.f;
				arrow.capSize = 0.2f;
				arrow.shaftDiameter = 0.1f;
				arrow.shaftLength = 2 / 3.f;
				return arrow;
			}
		}

		constexpr Arrow defaultArrow = impl::BuildDefaultArrow();

		constexpr f32 defaultGizmoSizeRelative = 0.4f;
		// Size is relative to the gizmo size
		constexpr f32 defaultPlaneScaleRelative = 0.25f;
		// Relative to the gizmo size
		constexpr f32 defaultPlaneOffsetRelative = 0.25f;

		constexpr f32 defaultRotateCircleInnerRadius = 0.1f;
		constexpr f32 defaultRotateCircleOuterRadius = 1.f - defaultRotateCircleInnerRadius;

		// target_size in pixels
		[[nodiscard]] f32 ComputeScale(
			Math::Mat4 const& worldTransform,
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

		Gui::Extent currentExtent = {};
		mutable Gui::Extent newExtent = {};
		mutable bool currentlyResizing = false;
		u32 extentCorrectTickCounter = 0;

		enum class BehaviorState : u8
		{
			Normal,
			FreeLooking,
			Gizmo,
		};
		BehaviorState state = BehaviorState::Normal;
		struct HoldingGizmoData
		{
			u8 pointerId;
			Gizmo::GizmoPart holdingPart;
			Math::Vec3 normalizedOffset;
			Math::Vec3 initialPos;
			// Current rotation offset from the pointer. In radians [-pi, pi]
			f32 rotationOffset;
		};
		Std::Opt<HoldingGizmoData> holdingGizmoData;

		struct Camera
		{
			Math::Vec3 position{ 0.f, 0.f, 1.f };
			Math::UnitQuat rotation = Math::UnitQuat::FromEulerAngles(0, 180.f, 0.f);
			f32 verticalFov = 60.f;
		};
		Camera cam{};

		InternalViewportWidget(EditorImpl& implData, Gfx::Context& gfxCtxIn);

		virtual ~InternalViewportWidget() override;

		[[nodiscard]] Math::Mat4 BuildViewMatrix() const noexcept;
		[[nodiscard]] Math::Mat4 BuildPerspectiveMatrix(f32 aspectRatio) const noexcept;
		[[nodiscard]] Math::Mat4 BuildProjectionMatrix(f32 aspectRatio) const noexcept;
		[[nodiscard]] Math::Vec3 BuildRayDirection(Gui::Rect widgetRect, Math::Vec2 pointerPos) const noexcept;

		void ApplyCameraRotation(Math::Vec2 input) noexcept;

		void ApplyCameraMovement(Math::Vec3 move, f32 speed) noexcept;

		void Tick() noexcept;

		Gfx::ViewportUpdate GetViewportUpdate(
			Context const& editor,
			std::vector<Math::Vec3>& lineVertices,
			std::vector<Gfx::LineDrawCmd>& lineDrawCmds) const noexcept;

		virtual bool CursorPress(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Math::Vec2Int cursorPos,
			Gui::CursorClickEvent event) override;

		virtual bool CursorMove(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::CursorMoveEvent event,
			bool occluded) override;

		virtual bool TouchPressEvent(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::TouchPressEvent event) override;

		virtual bool TouchMoveEvent(
			Gui::Context& ctx,
			Gui::WindowID windowId,
			Gui::Rect widgetRect,
			Gui::Rect visibleRect,
			Gui::TouchMoveEvent event,
			bool occluded) override;

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
		~ViewportWidget();

		EditorImpl* editorImpl = nullptr;
		InternalViewportWidget* viewport = nullptr;
		Joystick* leftJoystick = nullptr;
		Joystick* rightJoystick = nullptr;

		f32 joystickMovementSpeed = 2.5f;

		void Tick(float deltaTime) noexcept;
	};
}