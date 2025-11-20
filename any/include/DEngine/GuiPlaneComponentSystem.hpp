#pragma once

#include <DEngine/GfxGuiDrawEngineImpl.hpp>
#include <DEngine/Scene.hpp>
#include <DEngine/Gfx/Gfx.hpp>

namespace DEngine::GuiPlaneComponentSystem {
	void DispatchNavigationEvent(
		Scene& scene,
		Std::AnyRef appData,
		Gui::TextEngine& textEngine,
		Platform::Context& platformCtx,
		f32 directionalAngle);

	void DispatchActivateEvent(
		Scene& scene,
		Std::AnyRef appData,
		Gui::TextEngine& textEngine,
		Platform::Context& platformCtx);

	void DispatchBackEvent(
		Scene& scene,
		Std::AnyRef appData,
		Gui::TextEngine& textEngine,
		Platform::Context& platformCtx);

	void DispatchTextInputEvent(
		Scene& scene,
		Gui::Widget::WidgetEvent_TextInputParams const& event);
	void DispatchTextSelectionEvent(
		Scene& scene,
		Gui::Widget::WidgetEvent_TextSelectionParams const& event);
	void DispatchEndTextInputSessionEvent(
		Scene& scene,
		Gui::Widget::WidgetEvent_EndTextInputSessionParams const& event);

	struct DispatchCursorPressFromRay_Params {
		Scene& scene;
		Gui::TextEngine& textEngine;
		Platform::Context& platformCtx;

		Gui::AllocRef transientAlloc;

		Math::Vec3 rayOrigin;
		// Must be normalized.
		Math::Vec3 rayDir;

		Gui::CursorButton button;
		bool pressed;
	};
	// Ray-tests every GuiPlane in the scene; if the closest hit is inside the plane's bounds,
	// converts the hit to local widget-pixel coordinates and dispatches CursorPress2 to the
	// backplane widget. Returns true if a plane was hit (regardless of widget consumption).
	bool DispatchCursorPressFromRay(DispatchCursorPressFromRay_Params const& params);

	struct DispatchTouchPressFromRay_Params {
		Scene& scene;
		Gui::TextEngine& textEngine;
		Platform::Context& platformCtx;

		Math::Vec3 rayOrigin;
		Math::Vec3 rayDir;

		u8 fingerId;
		bool pressed;
	};
	// Same as DispatchCursorPressFromRay but for TouchPress2.
	bool DispatchTouchPressFromRay(DispatchTouchPressFromRay_Params const& params);

	struct DispatchCursorMoveFromRay_Params {
		Scene& scene;
		Gui::TextEngine& textEngine;

		Math::Vec3 rayOrigin;
		// Must be normalized.
		Math::Vec3 rayDir;
	};
	// Same as DispatchCursorPressFromRay but for CursorMove. Returns true if a plane was hit
	// (regardless of widget occlusion).
	bool DispatchCursorMoveFromRay(DispatchCursorMoveFromRay_Params const& params);

	struct DispatchTouchMoveFromRay_Params {
		Scene& scene;
		Gui::TextEngine& textEngine;

		Math::Vec3 rayOrigin;
		Math::Vec3 rayDir;

		u8 fingerId;
	};
	// Same as DispatchCursorMoveFromRay but for TouchMove2.
	bool DispatchTouchMoveFromRay(DispatchTouchMoveFromRay_Params const& params);

	[[nodiscard]] bool IsPointerHeld(Scene const& scene, u8 pointerId);

	struct RenderGuiPlaneParams {
		Std::ConstAnyRef appData;
		GfxGuiDrawEngineImpl::Params const& drawEngineParams;
		Component::GuiPlane const& guiPlane;
		std::vector<Gfx::SceneGuiPlane>& guiPlaneDrawCmds;
		Gui::TextEngine& textEngine;
		Transform const& transform;
		Std::FrameAlloc& transientAlloc;
	};
	void RenderGuiPlane(RenderGuiPlaneParams const&);
}
