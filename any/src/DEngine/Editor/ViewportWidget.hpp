#pragma once

#include "Editor.hpp"
#include "EditorImpl.hpp"

#include "ViewportGizmo.hpp"

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StdWidgets/AnchorArea.hpp>

#include <DEngine/Editor/Joystick.hpp>

#include <DEngine/Math/Matrix.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>

import DEngine.Std.StackVec;

namespace DEngine::Editor
{
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

		enum class BehaviorState : u8 {
			Normal,
			FreeLooking,
			Gizmo,
		};

		struct HoldingGizmoData {
			u8 pointerId;
			GizmoType gizmoType;
			ViewportGizmo::GizmoPart holdingPart;
			Math::Vec3 normalizedOffsetGizmo;
			// Contains the hit point compared to the position, in world space.
			Math::Vec3 relativeHitPointObject;
			Math::Vec2 initialObjectScale;
			Math::Vec3 initialPos;
			Math::UnitQuat initialRotation;
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

	class InternalViewportWidget : public Gui::Widget {
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

		// Pointer ids (touch fingers, or the cursor) whose primary press currently belongs to this
		// viewport: added when pressed inside, dropped on release. Scene move events are only
		// forwarded for ids in this set, so when several viewports share one scene a pointer captured
		// through one viewport is never also driven by another viewport's ray.
		Std::StackVec<u8, 20> pressedPointerIds;

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

		virtual Gui::Widget::GetSizeHint2_ReturnT GetSizeHint2(
			GetSizeHint2_Params const& params) const override;
		virtual void Render2(
			Render_Params const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter) const override;
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
		virtual Gui::TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
			bool occluded) override;
		virtual Gui::TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<Gui::RectCollection::Iter> const& rectCollIter,
			bool consumed) override;
	};
}