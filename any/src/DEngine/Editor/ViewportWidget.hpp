#pragma once

#include "Editor.hpp"
#include "EditorImpl.hpp"

#include <DEngine/Editor/Joystick.hpp>

#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/AnchorArea.hpp>

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
			f32 capDiameter = 0;
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
				arrow.capDiameter = 0.2f;
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

	class InternalViewportWidget;

	class ViewportWidget : public Gui::AnchorArea
	{
	public:
		ViewportWidget(EditorImpl& implData);
		~ViewportWidget();

		EditorImpl* editorImpl = nullptr;
		InternalViewportWidget* viewport = nullptr;
		[[nodiscard]] InternalViewportWidget& GetInternalViewport() { return *viewport; }
		[[nodiscard]] InternalViewportWidget const& GetInternalViewport() const { return *viewport; }

		Joystick* leftJoystick = nullptr;
		Joystick* rightJoystick = nullptr;

		f32 joystickMovementSpeed = 2.5f;

		enum class BehaviorState : u8
		{
			Normal,
			FreeLooking,
			Gizmo,
		};

		struct HoldingGizmoData
		{
			u8 pointerId;
			GizmoType gizmoType;
			Gizmo::GizmoPart holdingPart;
			Math::Vec3 normalizedOffsetGizmo;
			// Contains the hit point compared to the position, in world space.
			Math::Vec3 relativeHitPointObject;
			Math::Vec2 initialObjectScale;
			Math::Vec3 initialPos;
			// Current rotation offset from the pointer. In radians [-pi, pi]
			f32 rotationOffset;
		};

		struct Camera
		{
			Math::Vec3 position{ 0.f, 0.f, 1.f };
			Math::UnitQuat rotation = Math::UnitQuat::FromEulerAngles(0, 180.f, 0.f);
			f32 verticalFov = 60.f;
		};

		void Tick(float deltaTime) noexcept;
	};

	class InternalViewportWidget : public Gui::Widget
	{
	public:
		Gfx::ViewportID viewportId = Gfx::ViewportID::Invalid;
		EditorImpl* editorImpl = nullptr;

		bool wasRendered = false;

		Gui::Extent currentExtent = {};
		Gui::Extent newExtent = {};
		bool currentlyResizing = false;
		u32 extentCorrectTickCounter = 0;

		ViewportWidget::BehaviorState state = ViewportWidget::BehaviorState::Normal;

		Std::Opt<ViewportWidget::HoldingGizmoData> holdingGizmoData;

		ViewportWidget::Camera cam = {};

		InternalViewportWidget(EditorImpl& implData);

		virtual ~InternalViewportWidget() override;

		[[nodiscard]] Math::Mat4 BuildViewMatrix() const noexcept;
		[[nodiscard]] Math::Mat4 BuildPerspectiveMatrix(f32 aspectRatio) const noexcept;
		[[nodiscard]] Math::Mat4 BuildProjectionMatrix(f32 aspectRatio) const noexcept;
		[[nodiscard]] Math::Vec3 BuildRayDirection(Gui::Rect widgetRect, Math::Vec2 pointerPos) const noexcept;

		void ApplyCameraRotation(Math::Vec2 input) noexcept;

		void ApplyCameraMovement(Math::Vec3 move, f32 speed) noexcept;

		void Tick() noexcept;

		Gfx::ViewportUpdate BuildViewportUpdate(
			std::vector<Math::Vec3>& lineVertices,
			std::vector<Gfx::LineDrawCmd>& lineDrawCmds) const noexcept;

		virtual Gui::SizeHint GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void Render2(
			Render_Params const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect) const override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect,
			bool occluded) override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect,
			bool consumed) override;
		virtual bool TouchMove2(
			TouchMoveParams const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect,
			bool occluded) override;
		virtual bool TouchPress2(
			TouchPressParams const& params,
			Gui::Rect const& widgetRect,
			Gui::Rect const& visibleRect,
			bool consumed) override;
	};
}