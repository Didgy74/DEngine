#include <DEngine/GuiPlaneComponentSystem.hpp>

#include <DEngine/PlatformGuiGlue.hpp>

#include <DEngine/Math/LinearTransform3D.hpp>

#include <cmath>

import DEngine.Math.Common;
import DEngine.Std.FnRef;

using namespace DEngine;

namespace DEngine::GuiPlaneComponentSystem {
	// Very WIP! Just need a bigass visibleRect for some top-level nodes
	[[nodiscard]] constexpr Gui::Rect TopLevelLocalVisibleRect() {
		return { { -100000, -100000 }, { 200000, 200000 } };
	}

	struct CalcWindowInfo_ReturnT {
		u32 windowWidth = 0;
		u32 windowHeight = 0;
		Gui::EventWindowInfo windowInfo;
	};
	[[nodiscard]] CalcWindowInfo_ReturnT CalcWindowInfo(Transform const& transform) {
		u32 windowHeight = Scene::guiPlaneVerticalResolution;
		u32 windowWidth = (u32)Std::Round(transform.scale.x / transform.scale.y * (f32)windowHeight);
		Gui::EventWindowInfo windowInfo = {};
		windowInfo.dpi = Scene::guiPlaneDpi;
		windowInfo.contentScale = Scene::guiPlaneContentScale;
		windowInfo.fontScale = Scene::guiPlaneTextScale;
		return {
			.windowWidth = windowWidth,
			.windowHeight = windowHeight,
			.windowInfo = windowInfo, };
	}
}

[[nodiscard]] static f32 RotateDirectionalAngle(f32 directionalAngle, f32 rotationRadians) {
	// Convert radians to normalized rotation (0..1)
	float rotationNormalized = rotationRadians / (2*Math::pi);

	// Apply rotation
	float result = directionalAngle - rotationNormalized;

	// Wrap into [0,1)
	result = std::fmod(result, 1.0f);
	if (result < 0.0f)
		result += 1.0f;

	return result;
}

// TODO: This likely needs to be done as a task that happens after event dispatching?
bool Component::GuiPlane::GuiWindowHandler::SetFrontmostLayer(
	Std::Box<Gui::Widget>&& incomingWidget,
	Math::Vec2Int relativePosition,
	Gui::Extent extent,
	Std::Opt<Gui::Widget::Widget_FocusWidgetResult> const& focusItemOpt)
{
	DENGINE_IMPL_ASSERT(!extent.IsNothing());

	auto& guiPlane = *this->m_guiPlane;

	auto newFrontmostLayer = FrontmostLayer {
		.positionOffset = relativePosition,
		.extent = extent,
		.widget = Std::Move(incomingWidget),
	};
	if (focusItemOpt.Has()) {
		auto const& focusItem = focusItemOpt.Get();
		newFrontmostLayer.focusWidget = GuiPlane::FocusWidget {
			.widget = focusItem.widget,
			.secondaryIndex = focusItem.secondaryIndex };
	}

	guiPlane.frontmostLayer = Std::Move(newFrontmostLayer);
	return true;
}

void Component::GuiPlane::GuiWindowHandler::QueueRemoveCurrentLayer() {
	DENGINE_IMPL_ASSERT(m_removeCurrentLayer != nullptr);
	DENGINE_IMPL_ASSERT(*m_removeCurrentLayer == false);
	DENGINE_IMPL_ASSERT(this->m_guiPlane && this->m_guiPlane->frontmostLayer.Has());

	*m_removeCurrentLayer = true;
}

Std::Box<Gui::Widget::Widget_TextInputSession> Component::GuiPlane::GuiWindowHandler::TakeTextInputConnection(
	Gui::Widget& widget,
	Gui::TextInputType textInputFilter,
	Std::Span<char const> currentText,
	u64 selStart,
	u64 selCount)
{
	DENGINE_IMPL_ASSERT(this->m_scene != nullptr);
	DENGINE_IMPL_ASSERT(this->m_platformCtx != nullptr);

	// TODO: If there is an existing Widget with input focus, we must notify it that it
	// no longer has input focus.
	DENGINE_IMPL_ASSERT(this->m_scene->m_guiPlaneState.activeTextInputSession == nullptr);

	// TODO: We need to correctly handle WindowId here.
	this->m_platformCtx->StartTextInputSession(
		Platform::WindowID(0),
		PlatformGuiGlue::ToPlatformSoftInputFilter(textInputFilter),
		currentText,
		selStart,
		selCount);

	auto out = Std::BoxAdopt(new GuiTextInputSession);
	out->m_platformCtx = this->m_platformCtx;
	out->m_scene = this->m_scene;

	this->m_scene->m_guiPlaneState.activeTextInputSession = out.Get();

	return out;
}

Component::GuiPlane::GuiTextInputSession::~GuiTextInputSession() {
	DENGINE_IMPL_ASSERT(this->m_platformCtx != nullptr);
	DENGINE_IMPL_ASSERT(this->m_scene != nullptr);

	// TODO: This probably should have an identifier for each session
	// so we can avoid stopping the wrong session
	this->m_platformCtx->StopTextInputSession();

	// We need to clear whatever Widget reference we have stored in the Scene
	DENGINE_IMPL_ASSERT(this->m_scene->m_guiPlaneState.activeTextInputSession == this);
	this->m_scene->m_guiPlaneState.activeTextInputSession = nullptr;
}

namespace DEngine::GuiPlaneComponentSystem {
	struct PrepareRectCollection_Params {
		SceneGuiConstAppData const& constAppData;
		Gui::EventWindowInfo const& eventWindowInfo;
		Component::GuiPlane::FrontmostLayer const* frontmostLayer = nullptr;
		Gui::TextEngine& textEngine;
		Std::FrameAlloc& transientAlloc;
		Gui::Widget const& widget;
		u32 windowWidth = 0;
		u32 windowHeight = 0;
	};
	static void PrepareRectCollection(
		PrepareRectCollection_Params const& params,
		bool includeRendering,
		Gui::RectCollection& outRectCollection)
	{
		auto const& appData = params.constAppData;
		auto const& eventWindowInfo = params.eventWindowInfo;
		auto const& frontmostLayer = params.frontmostLayer;
		auto& textEngine = params.textEngine;
		auto& transientAlloc = params.transientAlloc;
		auto const& widget = params.widget;
		auto const& windowWidth = params.windowWidth;
		auto const& windowHeight = params.windowHeight;

		DENGINE_IMPL_ASSERT(transientAlloc.IsEmpty());

		outRectCollection.Prepare(includeRendering);
		{
			auto sizeHintPusher = Gui::RectCollection::SizeHintPusher{ outRectCollection };

			Gui::Widget::WidgetEvent_Measure_Params sizeHintParams = {
				.window = eventWindowInfo,
				.textEngine = textEngine,
				.appData = Std::ConstAnyRef{ appData },
				.transientAlloc = transientAlloc,
				.pusher = sizeHintPusher, };
			[[maybe_unused]] auto unused = widget.WidgetEvent_Measure(sizeHintParams);

			if (frontmostLayer != nullptr) {
				[[maybe_unused]] auto unused2 =
					frontmostLayer->widget->WidgetEvent_Measure(sizeHintParams);
			}

			transientAlloc.Reset();
		}

		{
			auto rectPusher = Gui::RectCollection::RectPusher{ outRectCollection };
			auto childEntry = rectPusher.GetEntry(widget);
			rectPusher.SetLocalRectPair(childEntry, {
				.widgetRect = { .position = {}, .extent = { .width = windowWidth, .height = windowHeight }},
				.visibleRect = { .position = {}, .extent = { .width = windowWidth, .height = windowHeight }} });
			Gui::Widget::WidgetEvent_Layout_Params eventParams = {
				.window = eventWindowInfo,
				.textEngine = textEngine,
				.appData = Std::ConstAnyRef{ appData },
				.transientAlloc = transientAlloc,
				.pusher = rectPusher, };
			widget.WidgetEvent_Layout(
				eventParams,
				{ .position = {}, .extent ={ .width = windowWidth, .height = windowHeight } },
				{ .position = {}, .extent ={ .width = windowWidth, .height = windowHeight } },
				Std::nullOpt);
			if (frontmostLayer != nullptr) {
				DENGINE_IMPL_ASSERT(frontmostLayer->widget.Has());
				// The layer's size is fixed at SetFrontmostLayer time by the caller —
				// we don't query GetSizeHint for it. A layer can conceptually be backed
				// by a child OS window, which makes resizing after the fact impractical
				// across platforms; treating the extent as authoritative keeps that
				// constraint visible here.
				auto const& frontmostLayerWidget = *frontmostLayer->widget;
				auto layerEntry = rectPusher.GetEntry(frontmostLayerWidget);
				Gui::Rect const layerRect = {
					.position = frontmostLayer->positionOffset,
					.extent = frontmostLayer->extent };
				rectPusher.SetLocalRectPair(layerEntry, {
					.widgetRect = layerRect,
					.visibleRect = TopLevelLocalVisibleRect() });
				frontmostLayerWidget.WidgetEvent_Layout(
					eventParams,
					layerRect,
					TopLevelLocalVisibleRect(),
					Std::nullOpt);
			}

			transientAlloc.Reset();
		}

		outRectCollection.SetTreeOffsetAndWindowExtent(
			{},
			{ .width = windowWidth, .height = windowHeight } );
	}

	[[nodiscard]] static Std::Opt<Gui::Widget::CurrentFocusWidgetData> CalcCurrentFocusWidgetData(
		Component::GuiPlane const& guiPlane,
		Gui::RectCollection const& rectCollection)
	{
		Std::Opt<Gui::Widget::CurrentFocusWidgetData> result;
		if (guiPlane.focusWidget.Has()) {
			auto const& focusWidget = guiPlane.focusWidget.Get();
			auto entryOpt = rectCollection.GetEntry(*focusWidget.widget);
			DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
			result = Gui::Widget::CurrentFocusWidgetData {
				.widget = focusWidget.widget,
				.iter = entryOpt.Get(),
				.secondaryIndex = focusWidget.secondaryIndex };
		}
		return result;
	}

	struct GuiPlaneEventContext {
		SceneGuiMutAppData mutAppData;
		Component::GuiPlane& guiPlane;
		Platform::Context& platformCtx;
		Gui::RectCollection& rectCollection;
		Transform const& transform;
		Std::FrameAlloc& transientAlloc;
		u32 windowHeight;
		Gui::EventWindowInfo const& windowInfo;
		u32 windowWidth;
	};

	static void DispatchToGuiPlanes(
		Scene& scene,
		SceneGuiConstAppData const& constAppData,
		SceneGuiMutAppData& mutAppData,
		Gui::TextEngine& textEngine,
		Platform::Context& platformCtx,
		Std::FnRef<bool(GuiPlaneEventContext const&)> callback)
	{
		constexpr auto frameAllocDefaultSize = 1024 * 1024;
		auto transientAlloc = Std::FrameAlloc::PreAllocate(frameAllocDefaultSize).Get();
		auto rectCollection = Gui::RectCollection{};

		for (auto& [entity, guiPlane] : scene.GetAllComponents<Component::GuiPlane>()) {
			auto const* transformPtr = scene.GetComponent<Transform>(entity);
			if (transformPtr == nullptr) {
				continue;
			}

			auto const& transform = *transformPtr;

			auto [windowWidth, windowHeight, windowInfo] = CalcWindowInfo(transform);

			PrepareRectCollection_Params prepareParams {
				.constAppData = constAppData,
				.eventWindowInfo = windowInfo,
				.frontmostLayer = guiPlane.frontmostLayer.ToPtr(),
				.textEngine = textEngine,
				.transientAlloc = transientAlloc,
				.widget = *guiPlane.widget,
				.windowWidth = windowWidth,
				.windowHeight = windowHeight };
			PrepareRectCollection(prepareParams, true, rectCollection);

			GuiPlaneEventContext ctx = {
				.mutAppData = mutAppData,
				.guiPlane = guiPlane,
				.platformCtx = platformCtx,
				.rectCollection = rectCollection,
				.transform = transform,
				.transientAlloc = transientAlloc,
				.windowHeight = windowHeight,
				.windowInfo = windowInfo,
				.windowWidth = windowWidth, };

			if (callback(ctx)) {
				break;
			}
		}
	}

	// Runs the immutable focus-navigation pass for one plane in a single direction, updating
	// the plane's stored focus if a widget took it. Shared by the discrete navigation dispatch
	// and the axis-input dispatch's on-Passed fallback. worldDirectionalAngle uses the app
	// convention (0 = up, 0.25 = left, 0.5 = down, 0.75 = right) and is rotated into the plane's
	// local frame here.
	static void NavigatePlane(
		GuiPlaneEventContext const& ctx,
		f32 worldDirectionalAngle)
	{
		auto& guiPlane = ctx.guiPlane;
		auto& rectCollection = ctx.rectCollection;

		auto localDirectionalAngle = RotateDirectionalAngle(
			worldDirectionalAngle,
			Math::degToRad * ctx.transform.rotation.ToEulerAngles().z);

		if (guiPlane.frontmostLayer.Has()) {
			auto& frontmostLayer = guiPlane.frontmostLayer.Get();

			Std::Opt<Gui::Widget::CurrentFocusWidgetData> currentFocusOpt;
			if (frontmostLayer.focusWidget.Has()) {
				auto const& focusWidget = frontmostLayer.focusWidget.Get();
				auto entryOpt = rectCollection.GetEntry(*focusWidget.widget);
				DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
				currentFocusOpt = Gui::Widget::CurrentFocusWidgetData {
					.widget = focusWidget.widget,
					.iter = entryOpt.Get(),
					.secondaryIndex = focusWidget.secondaryIndex };
			}

			Gui::Widget::Widget_NavigationParams navParams = {
				.window = ctx.windowInfo,
				.transientAlloc = ctx.transientAlloc,
				.rectCollection = ctx.rectCollection,
				.directional = true,
				.angle = localDirectionalAngle,
				.forward = false,
				.currentFocusOpt = currentFocusOpt };
			Gui::Widget::Widget_NavigationState navState = {};
			frontmostLayer.widget->WidgetEvent_Navigation(
				navParams,
				Std::nullOpt,
				navState);
			if (navState.IsConsumed()) {
				frontmostLayer.focusWidget = Component::GuiPlane::FocusWidget {
					.widget = navState.consumedWidget,
					.secondaryIndex = navState.consumedSecondaryIndex };
			}
			return;
		}

		auto currentFocusWidgetData = CalcCurrentFocusWidgetData(
			guiPlane,
			ctx.rectCollection);

		Gui::Widget::Widget_NavigationParams navParams = {
			.window = ctx.windowInfo,
			.transientAlloc = ctx.transientAlloc,
			.rectCollection = ctx.rectCollection,
			.directional = true,
			.angle = localDirectionalAngle,
			.forward = false,
			.currentFocusOpt = currentFocusWidgetData };
		Gui::Widget::Widget_NavigationState navState = {};
		guiPlane.widget->WidgetEvent_Navigation(
			navParams,
			Std::nullOpt,
			navState);
		if (navState.IsConsumed()) {
			guiPlane.focusWidget = Component::GuiPlane::FocusWidget {
				.widget = navState.consumedWidget,
				.secondaryIndex = navState.consumedSecondaryIndex };
		}
	}

	// Maps the axis-input source's discrete steps (Gui convention: +X right, +Y down) to a single
	// directional-navigation angle. Only called when there is a nonzero step; vertical wins over
	// horizontal when both stepped in the same frame.
	[[nodiscard]] static f32 StepsToDirectionalAngle(Math::Vec2Int steps) {
		if (steps.y < 0) return 0.f;    // up
		if (steps.y > 0) return 0.5f;   // down
		if (steps.x < 0) return 0.25f;  // left
		return 0.75f;                   // right
	}
} // namespace GuiPlaneComponentSystem

void GuiPlaneComponentSystem::DispatchNavigationEvent(
	Scene& scene,
	Gui::TextEngine& textEngine,
	Platform::Context& platformCtx,
	f32 directionalAngle)
{
	SceneGuiConstAppData constAppData = {
		.scene = scene, };
	SceneGuiMutAppData mutAppData = {
		.scene = scene, };

	DispatchToGuiPlanes(
		scene,
		constAppData,
		mutAppData,
		textEngine,
		platformCtx,
		[&](GuiPlaneEventContext const& ctx) {
			NavigatePlane(ctx, directionalAngle);
			return true;
		});
}

void GuiPlaneComponentSystem::DispatchActivateEvent(
	Scene& scene,
	Gui::TextEngine& textEngine,
	Platform::Context& platformCtx)
{
	SceneGuiConstAppData constAppData = {
		.scene = scene, };
	SceneGuiMutAppData mutAppData = {
		.scene = scene, };

	DispatchToGuiPlanes(
		scene,
		constAppData,
		mutAppData,
		textEngine,
		platformCtx,
		[&](GuiPlaneEventContext const& ctx) {
			auto& guiPlane = ctx.guiPlane;

			bool hasFrontmostLayer = guiPlane.frontmostLayer.Has();

			bool removeFrontmostLayer = false;

			// First check frontmost layer if it exists
			if (hasFrontmostLayer) {
				auto& frontmostLayer = guiPlane.frontmostLayer.Get();
				if (frontmostLayer.focusWidget.Has()) {
					auto& focusWidget = frontmostLayer.focusWidget.Get();
					DENGINE_IMPL_ASSERT(focusWidget.widget != nullptr);

					Component::GuiPlane::GuiWindowHandler windowHandler;
					windowHandler.m_guiPlane = &ctx.guiPlane;
					windowHandler.m_removeCurrentLayer = &removeFrontmostLayer;
					windowHandler.m_scene = &scene;

					Std::Opt<uSize> secondaryIndex;
					if (focusWidget.secondaryIndex.Has()) {
						secondaryIndex = focusWidget.secondaryIndex.Get();
					}

					Gui::Widget::WidgetEvent_ActivateParams params = {
						.customData = Std::AnyRef{ mutAppData },
						.rectCollection = ctx.rectCollection,
						.transientAlloc = ctx.transientAlloc,
						.windowHandler = windowHandler,
						.focusSecondaryIndex = secondaryIndex };
					[[maybe_unused]] auto result = focusWidget.widget->WidgetEvent_Activate(params, Std::nullOpt);
				}
			}

			if (removeFrontmostLayer) {
				guiPlane.frontmostLayer = Std::nullOpt;
			}

			if (!hasFrontmostLayer) {
				if (!ctx.guiPlane.focusWidget.Has()) {
					return false;
				}

				auto& focusWidget = ctx.guiPlane.focusWidget.Get();

				Component::GuiPlane::GuiWindowHandler windowHandler = {};
				windowHandler.m_guiPlane = &ctx.guiPlane;
				windowHandler.m_scene = &scene;
				windowHandler.m_platformCtx = &ctx.platformCtx;
				windowHandler.m_removeCurrentLayer = &removeFrontmostLayer;

				Std::Opt<uSize> secondaryIndex;
				if (focusWidget.secondaryIndex.Has()) {
					secondaryIndex = focusWidget.secondaryIndex.Get();
				}

				Gui::Widget::WidgetEvent_ActivateParams params = {
					.customData = Std::AnyRef{ mutAppData },
					.rectCollection = ctx.rectCollection,
					.transientAlloc = ctx.transientAlloc,
					.windowHandler = windowHandler,
					.focusSecondaryIndex = secondaryIndex };

				[[maybe_unused]] auto result = focusWidget.widget->WidgetEvent_Activate(params, Std::nullOpt);
			}

			return true;
		});
}

void GuiPlaneComponentSystem::DispatchBackEvent(
	Scene& scene,
	Gui::TextEngine& textEngine,
	Platform::Context& platformCtx)
{
	SceneGuiConstAppData constAppData = {
		.scene = scene, };
	SceneGuiMutAppData mutAppData = {
		.scene = scene, };

	DispatchToGuiPlanes(
		scene,
		constAppData,
		mutAppData,
		textEngine,
		platformCtx,
		[&](GuiPlaneEventContext const& ctx) {
			auto& guiPlane = ctx.guiPlane;

			bool removeFrontmostLayer = false;

			// Back is a semantic cancel request, not a focus-navigation event.
			// The frontmost layer receives first refusal because popups and menus
			// usually need to close without activating the focused item.
			if (guiPlane.frontmostLayer.Has()) {
				auto& frontmostLayer = guiPlane.frontmostLayer.Get();

				Std::Opt<Gui::Widget::CurrentFocusWidgetData> currentFocusOpt;
				if (frontmostLayer.focusWidget.Has()) {
					auto const& focusWidget = frontmostLayer.focusWidget.Get();
					auto entryOpt = ctx.rectCollection.GetEntry(*focusWidget.widget);
					DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
					currentFocusOpt = Gui::Widget::CurrentFocusWidgetData {
						.widget = focusWidget.widget,
						.iter = entryOpt.Get(),
						.secondaryIndex = focusWidget.secondaryIndex };
				}

				Component::GuiPlane::GuiWindowHandler windowHandler;
				windowHandler.m_guiPlane = &ctx.guiPlane;
				windowHandler.m_removeCurrentLayer = &removeFrontmostLayer;

				Gui::Widget::WidgetEvent_BackParams params = {
					.customData = Std::AnyRef{ mutAppData },
					.rectCollection = ctx.rectCollection,
					.transientAlloc = ctx.transientAlloc,
					.windowHandler = windowHandler,
					.focusedWidget = currentFocusOpt };
				[[maybe_unused]] bool temp = frontmostLayer.widget->WidgetEvent_Back(
					params,
					Std::nullOpt);

				if (removeFrontmostLayer) {
					guiPlane.frontmostLayer = Std::nullOpt;
				}

				// Stop at a frontmost layer even if it did not handle Back. A layer
				// in front of this plane is still the top UI surface for this event.
				return true;
			}

			auto currentFocusOpt = CalcCurrentFocusWidgetData(
				guiPlane,
				ctx.rectCollection);
			if (!currentFocusOpt.Has()) {
				return false;
			}

			Component::GuiPlane::GuiWindowHandler windowHandler;
			windowHandler.m_guiPlane = &ctx.guiPlane;

			Gui::Widget::WidgetEvent_BackParams params = {
				.customData = Std::AnyRef{ mutAppData },
				.rectCollection = ctx.rectCollection,
				.transientAlloc = ctx.transientAlloc,
				.windowHandler = windowHandler,
				.focusedWidget = currentFocusOpt, };
			auto temp = currentFocusOpt.Get().widget->WidgetEvent_Back(
				params,
				Std::nullOpt);
			return temp;
		});
}

namespace DEngine::GuiPlaneComponentSystem {
	// Shared traversal for the non-spatial text events. Unlike pointer events, these don't ray-test
	// or need a RectCollection — the active text-editing Widget mutates its own state in response.
	// We broadcast into every plane (and any frontmost layer); only the Widget holding the active
	// session reacts. dispatchFn does the concrete Widget call: void(Gui::Widget&, Gui::AllocRef const&)
	template<typename DispatchFnT>
	static void DispatchTextEventCommon(Scene& scene, DispatchFnT const& dispatchFn) {
		// Exactly one Widget across the scene can own a text-input session at a time. If none does,
		// there is nothing that could consume this event, so we skip the whole traversal.
		if (scene.m_guiPlaneState.activeTextInputSession == nullptr) {
			return;
		}

		// Text handling is lightweight and most Widgets ignore the allocator entirely, so a small
		// bump block keeps the per-event cost down. It grows on demand if some Widget needs more.
		constexpr auto frameAllocDefaultSize = 2048;
		auto transientAlloc = Std::FrameAlloc::PreAllocate(frameAllocDefaultSize).Get();
		auto allocRef = Gui::AllocRef{ transientAlloc };

		for (auto& [entity, guiPlane] : scene.GetAllComponents<Component::GuiPlane>()) {
			dispatchFn(*guiPlane.widget, allocRef);
			if (guiPlane.frontmostLayer.Has()) {
				dispatchFn(*guiPlane.frontmostLayer.Get().widget, allocRef);
			}
			transientAlloc.Reset();
		}
	}
} // namespace GuiPlaneComponentSystem

void GuiPlaneComponentSystem::DispatchTextInputEvent(
	Scene& scene,
	Gui::Widget::WidgetEvent_TextInputParams const& event)
{
	DispatchTextEventCommon(
		scene,
		[&](Gui::Widget& widget, Gui::AllocRef const& allocRef) {
			widget.WidgetEvent_TextInput(allocRef, event);
		});
}

void GuiPlaneComponentSystem::DispatchTextSelectionEvent(
	Scene& scene,
	Gui::Widget::WidgetEvent_TextSelectionParams const& event)
{
	DispatchTextEventCommon(
		scene,
		[&](Gui::Widget& widget, Gui::AllocRef const& allocRef) {
			widget.WidgetEvent_TextSelection(allocRef, event);
		});
}

void GuiPlaneComponentSystem::DispatchEndTextInputSessionEvent(
	Scene& scene,
	Gui::Widget::WidgetEvent_EndTextInputSessionParams const& event)
{
	DispatchTextEventCommon(
		scene,
		[&](Gui::Widget& widget, Gui::AllocRef const& allocRef) {
			widget.WidgetEvent_EndTextInputSession(allocRef, event);
		});
}

namespace DEngine::GuiPlaneComponentSystem {
	// Special pointer id standing in for the mouse cursor. Touch fingers carry their small,
	// non-negative platform ids, leaving the top of the u8 range free for the cursor. This matches
	// the cursorPointerId convention used throughout the StdWidgets.
	static constexpr u8 cursorPointerId = (u8)-1;

	struct PlaneRayHit {
		f32 distance = 0;
		// Hit point in the plane's local frame; the unit quad spans [-0.5, 0.5] in x and y.
		Math::Vec2 hitLocal;
		// In widget-pixel coordinates (top-left origin, y-down).
		Math::Vec2 hitPixel;
		bool withinBounds;
	};

	[[nodiscard]] static Std::Opt<PlaneRayHit> RayIntersectsUnboundedGuiPlane(
		Transform const& transform,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir)
	{
		auto const worldMat =
			Math::LinAlg3D::Translate(transform.position)
			* Math::LinAlg3D::Scale_Homo(transform.scale.AsVec3(1.f))
			* Math::LinAlg3D::Rotate_Homo(transform.rotation);

		auto const worldMatInvOpt = worldMat.GetInverse();
		if (!worldMatInvOpt.HasValue()) {
			return Std::nullOpt;
		}
		auto const& worldMatInv = worldMatInvOpt.Value();

		// Bring the ray into the plane's local frame. The plane is the local z = 0 quad
		// spanning [-0.5, 0.5] in x and y.
		auto const rayOriginLocal4 = worldMatInv * rayOrigin.AsVec4(1.f);
		auto const rayDirLocal4 = worldMatInv * rayDir.AsVec4(0.f);
		Math::Vec3 const rayOriginLocal = { rayOriginLocal4.x, rayOriginLocal4.y, rayOriginLocal4.z };
		Math::Vec3 const rayDirLocal = { rayDirLocal4.x, rayDirLocal4.y, rayDirLocal4.z };

		if (Math::Abs(rayDirLocal.z) < 0.0000001f) {
			return Std::nullOpt;
		}

		f32 const t = -rayOriginLocal.z / rayDirLocal.z;
		if (t < 0.f) {
			return Std::nullOpt;
		}

		Math::Vec3 const hitLocal = rayOriginLocal + rayDirLocal * t;

		auto [windowWidth, windowHeight, _] = CalcWindowInfo(transform);

		// GUI pixel y is top-left; plane local +y is up.
		f32 const hitPixelX = (hitLocal.x + 0.5f) * (f32)windowWidth;
		f32 const hitPixelY = (0.5f - hitLocal.y) * (f32)windowHeight;

		PlaneRayHit result = {};
		result.distance = t;
		result.hitLocal = { hitLocal.x, hitLocal.y };
		result.hitPixel = { hitPixelX, hitPixelY };
		result.withinBounds = hitLocal.x > -0.5f && hitLocal.x < 0.5f && hitLocal.y > -0.5f && hitLocal.y < 0.5f;
		return result;
	}

	// True if a widget-pixel point lies within a frontmost layer's rect. The layer rect can extend
	// beyond the base unit quad, so it is tested in pixel coords against its own
	// [positionOffset, positionOffset + extent) bounds.
	[[nodiscard]] static bool FrontmostLayerContainsPixel(
		Component::GuiPlane::FrontmostLayer const& layer,
		Math::Vec2Int pixel)
	{
		i32 const minX = layer.positionOffset.x;
		i32 const minY = layer.positionOffset.y;
		i32 const maxX = minX + (i32)layer.extent.width;
		i32 const maxY = minY + (i32)layer.extent.height;
		return
			pixel.x >= minX && pixel.x < maxX &&
			pixel.y >= minY && pixel.y < maxY;
	}

	// Finds the GuiPlane whose 3D quad is hit first by the ray, bounds-checked against the base
	// quad and any frontmost layer. Returns nullOpt if no plane is hit within bounds. t is the
	// world-space distance along the ray (the outer T*S*R preserves the ray parameter), so it is
	// directly comparable across planes.
	struct RayHitClosestGuiPlane_Result {
		Entity entity;
		PlaneRayHit hit;
	};
	[[nodiscard]] static Std::Opt<RayHitClosestGuiPlane_Result> RayHitClosestGuiPlane(
		Scene const& scene,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir)
	{
		Std::Opt<RayHitClosestGuiPlane_Result> best;

		for (auto const& [entity, guiPlane] : scene.GetAllComponents<Component::GuiPlane>()) {
			auto const* transformPtr = scene.GetComponent<Transform>(entity);
			if (transformPtr == nullptr) {
				continue;
			}

			auto const hitOpt = RayIntersectsUnboundedGuiPlane(*transformPtr, rayOrigin, rayDir);
			if (!hitOpt.HasValue()) {
				continue;
			}
			auto const& hit = hitOpt.Value();

			bool const withinBounds =
				hit.hitLocal.x >= -0.5f && hit.hitLocal.x <= 0.5f &&
				hit.hitLocal.y >= -0.5f && hit.hitLocal.y <= 0.5f;

			bool hitFrontmostLayer = false;
			if (guiPlane.frontmostLayer.Has()) {
				hitFrontmostLayer = FrontmostLayerContainsPixel(
					guiPlane.frontmostLayer.Get(),
					hit.hitPixel.Cast<i32>());
			}

			if (!withinBounds && !hitFrontmostLayer) {
				continue;
			}

			if (best.HasValue() && hit.distance >= best.Value().hit.distance) {
				continue;
			}

			RayHitClosestGuiPlane_Result out = {
				.entity = entity,
				.hit = hit,
			};
			best = out;
		}

		return best;
	}

	// Returns the entity of the GuiPlane currently holding (capturing) pointerId, if any. A real
	// pointer — a touch finger or the cursor — can be held by at most one plane at a time.
	[[nodiscard]] static Std::Opt<Entity> FindPlaneHoldingPointer(Scene const& scene, u8 pointerId) {
		for (auto const& [entity, guiPlane] : scene.GetAllComponents<Component::GuiPlane>()) {
			for (auto const& held : guiPlane.heldPointerIds) {
				if (held.id == pointerId) {
					return entity;
				}
			}
		}
		return Std::nullOpt;
	}

	// Public form of the above for hosts that embed scene GuiPlanes (see header).
	bool IsPointerHeld(Scene const& scene, u8 pointerId) {
		return FindPlaneHoldingPointer(scene, pointerId).HasValue();
	}

	// Records pointerId as captured by this plane so its later move/release events keep arriving
	// even once the ray leaves the plane bounds. Updates the stored position if already held.
	static void CaptureHeldPointer(Component::GuiPlane& guiPlane, u8 pointerId, Math::Vec2 pixelPos) {
		for (auto& held : guiPlane.heldPointerIds) {
			if (held.id == pointerId) {
				held.position = pixelPos;
				return;
			}
		}
		guiPlane.heldPointerIds.PushBack({ .id = pointerId, .position = pixelPos });
	}

	// Refreshes the stored position of an already-held pointer. No-op if the plane is not holding it.
	static void UpdateHeldPointerPosition(Component::GuiPlane& guiPlane, u8 pointerId, Math::Vec2 pixelPos) {
		for (auto& held : guiPlane.heldPointerIds) {
			if (held.id == pointerId) {
				held.position = pixelPos;
				return;
			}
		}
	}

	// Drops pointerId from whichever plane is holding it. Returns true if it was held somewhere.
	// Order within a plane's list is irrelevant, so the swap-erase keeps this O(held count).
	static bool ReleaseHeldPointer(Scene& scene, u8 pointerId) {
		for (auto& [entity, guiPlane] : scene.GetAllComponents<Component::GuiPlane>()) {
			auto& held = guiPlane.heldPointerIds;
			for (uSize i = 0; i < held.Size(); i += 1) {
				if (held[i].id == pointerId) {
					held.EraseUnsorted(i);
					return true;
				}
			}
		}
		return false;
	}

	struct ResolvedPlaneTarget {
		Entity entity;
		Math::Vec2 hitPixel;
	};
	[[nodiscard]] static Std::Opt<ResolvedPlaneTarget> ResolvePlaneTarget(
		Scene const& scene,
		u8 pointerId,
		Math::Vec3 rayOrigin,
		Math::Vec3 rayDir)
	{
		auto const heldEntityOpt = FindPlaneHoldingPointer(scene, pointerId);
		if (heldEntityOpt.HasValue()) {
			auto const entity = heldEntityOpt.Value();
			auto const* transformPtr = scene.GetComponent<Transform>(entity);
			DENGINE_IMPL_ASSERT(transformPtr != nullptr);
			auto const hitOpt = RayIntersectsUnboundedGuiPlane(*transformPtr, rayOrigin, rayDir);
			if (!hitOpt.HasValue()) {
				return Std::nullOpt;
			}
			return ResolvedPlaneTarget{ entity, hitOpt.Value().hitPixel };
		}

		auto const hitOpt = RayHitClosestGuiPlane(scene, rayOrigin, rayDir);
		if (!hitOpt.HasValue()) {
			return Std::nullOpt;
		}
		return ResolvedPlaneTarget{ hitOpt.Value().entity, hitOpt.Value().hit.hitPixel };
	}

	// Backend for Gui::Widget::WidgetEvent_DeferredJobQueue. During a press dispatch, widgets may
	// register movable callables that must run only once the dispatch has fully unwound — this lets
	// a widget mutate the UI tree (e.g. select an entity, open/close a layer) without invalidating
	// the traversal that is currently walking it. Both the job list and the callables themselves are
	// stored in a Gui::AllocRef-wrapped FrameAlloc owned by the caller; nothing here outlives the
	// single dispatch. Flush() invokes every job, then destroys and frees them.
	struct DeferredJobQueueBackend final : public Gui::Widget::WidgetEvent_DeferredJobQueueBackend {
		struct Job {
			void* ptr = nullptr;
			int allocSize = 0;
			InvokeFnT invokeFn = nullptr;
			DestroyFnT destroyFn = nullptr;
		};

		Gui::AllocRef alloc;
		Std::Vec<Job, Gui::AllocRef> jobs;

		explicit DeferredJobQueueBackend(Gui::AllocRef in) : alloc{ in }, jobs{ in } {}

		void PushPostEventJob(
			int size,
			int alignment,
			InvokeFnT invokeFn,
			void* callablePtr,
			MoveFnT moveFn,
			DestroyFnT destroyFn) override
		{
			Job job = {};
			job.ptr = alloc.Alloc((uSize)size, (uSize)alignment);
			job.allocSize = size;
			job.invokeFn = invokeFn;
			job.destroyFn = destroyFn;
			// Move-construct the callable into our storage; the caller still owns and destroys the source.
			moveFn(job.ptr, callablePtr);
			jobs.PushBack(job);
		}

		// Invokes every registered job exactly once, then destroys and frees them. Call once, after
		// the triggering event dispatch has returned.
		void Flush(Std::AnyRef customData) {
			for (auto const& job : jobs.ToSpan()) {
				job.invokeFn(job.ptr, customData);
			}
			// Free in reverse allocation order so the bump allocator can roll its offset back.
			for (uSize i = jobs.Size(); i != 0; i -= 1) {
				auto const& job = jobs[i - 1];
				job.destroyFn(job.ptr);
				alloc.Free(job.ptr, (uSize)job.allocSize);
			}
			jobs.Clear();
		}
	};

	struct DispatchPressShared {
		Scene& scene;
		Gui::TextEngine& textEngine;
		Platform::Context& platformCtx;
		Math::Vec3 rayOrigin;
		Math::Vec3 rayDir;
	};
	struct DispatchPressFromRayCommon_DispatchParamsT {
		Component::GuiPlane& guiPlane;
		Math::Vec2 hitPixel;
		Gui::Widget::WidgetEvent_DeferredJobQueue& jobQueue;
		Gui::RectCollection& rectCollection;
		Std::FrameAlloc& transientAlloc;
		Component::GuiPlane::GuiWindowHandler& windowHandler;
		Gui::EventWindowInfo const& windowInfo;
	};
	// Common path for press and release. A press-down hit-tests within the plane bounds and captures
	// the pointer onto the hit plane; a release follows that capture (bounds ignored) and ends it.
	// Prepares the RectCollection, then hands off to the per-event-type dispatcher. Returns true if a
	// plane handled the event.
	[[nodiscard]] static bool DispatchPressFromRayCommon(
		DispatchPressShared const& shared,
		u8 pointerId,
		bool pressed,
		Std::FnRef<void(DispatchPressFromRayCommon_DispatchParamsT const&)> const& dispatchFn)
	{
		auto& scene = shared.scene;

		// A press-down begins a fresh interaction: it hit-tests within bounds and ignores any
		// existing capture (a real pointer cannot press again without first releasing, so a leftover
		// capture would be stale). A release instead follows the capture — it routes to the holding
		// plane with bounds ignored, so a drag that wandered off the plane still gets its release.
		Entity targetEntity = {};
		Math::Vec2 hitPixel = {};
		if (pressed) {
			auto const hitOpt = RayHitClosestGuiPlane(scene, shared.rayOrigin, shared.rayDir);

			// A press-down auto-closes every open frontmost layer the press did not land inside,
			// including the case where the ray misses every plane. The one layer the press lands
			// in (if any) is kept so it can be dispatched into below. The dismissing press is passed
			// through: it still routes to whatever it hit, it just closes the other layers first.
			Std::Opt<Entity> keepLayerEntity;
			if (hitOpt.HasValue()) {
				auto const hitEntity = hitOpt.Value().entity;
				auto const* hitPlane = scene.GetComponent<Component::GuiPlane>(hitEntity);
				DENGINE_IMPL_ASSERT(hitPlane != nullptr);
				if (hitPlane->frontmostLayer.Has() &&
					FrontmostLayerContainsPixel(hitPlane->frontmostLayer.Get(), hitOpt.Value().hit.hitPixel.Cast<i32>()))
				{
					keepLayerEntity = hitEntity;
				}
			}
			for (auto& [entity, guiPlane] : scene.GetAllComponents<Component::GuiPlane>()) {
				if (!guiPlane.frontmostLayer.Has()) {
					continue;
				}

				if (keepLayerEntity.HasValue() && keepLayerEntity.Value() == entity) {
					continue;
				}

				guiPlane.frontmostLayer = Std::nullOpt;
			}

			if (!hitOpt.HasValue()) {
				// Missed every plane; the layers we just closed were the only thing to act on.
				return false;
			}
			targetEntity = hitOpt.Value().entity;
			hitPixel = hitOpt.Value().hit.hitPixel;
		} else {
			auto const targetOpt = ResolvePlaneTarget(scene, pointerId, shared.rayOrigin, shared.rayDir);
			if (!targetOpt.HasValue()) {
				// The held plane is no longer reachable by the ray (e.g. it spun edge-on). Still drop
				// any capture so it cannot get stuck, and report it consumed if it actually was held.
				return ReleaseHeldPointer(scene, pointerId);
			}
			targetEntity = targetOpt.Value().entity;
			hitPixel = targetOpt.Value().hitPixel;
		}

		auto* guiPlanePtr = scene.GetComponent<Component::GuiPlane>(targetEntity);
		DENGINE_IMPL_ASSERT(guiPlanePtr != nullptr);
		auto& guiPlane = *guiPlanePtr;
		auto const* transformPtr = scene.GetComponent<Transform>(targetEntity);
		DENGINE_IMPL_ASSERT(transformPtr != nullptr);
		auto const& transform = *transformPtr;

		// TODO: This should reuse the transient alloc from the outside, but we should have a checkpoint system
		// for the frame allocator.
		constexpr auto frameAllocDefaultSize = 1024 * 1024;
		auto transientAlloc = Std::FrameAlloc::PreAllocate(frameAllocDefaultSize).Get();
		auto rectCollection = Gui::RectCollection{};
		auto [windowWidth, windowHeight, windowInfo] = CalcWindowInfo(transform);

		SceneGuiConstAppData constAppData = {
			.scene = scene, };
		SceneGuiMutAppData mutAppData = {
			.scene = scene, };
		PrepareRectCollection_Params prepareParams {
			.constAppData = constAppData,
			.eventWindowInfo = windowInfo,
			.frontmostLayer = guiPlane.frontmostLayer.ToPtr(),
			.textEngine = shared.textEngine,
			.transientAlloc = transientAlloc,
			.widget = *guiPlane.widget,
			.windowWidth = windowWidth,
			.windowHeight = windowHeight };
		PrepareRectCollection(prepareParams, false, rectCollection);

		bool removeFrontmostLayer = false;
		Component::GuiPlane::GuiWindowHandler windowHandler = {};
		windowHandler.m_guiPlane = &guiPlane;
		windowHandler.m_removeCurrentLayer = &removeFrontmostLayer;
		windowHandler.m_scene = &scene;
		windowHandler.m_platformCtx = &shared.platformCtx;

		// Deferred jobs registered during dispatch run after it returns. They get their own
		// FrameAlloc so the per-event transientAlloc can be reset freely during the traversal.
		auto deferredJobAlloc = Std::FrameAlloc::PreAllocate(1024).Get();
		auto jobQueueBackend = DeferredJobQueueBackend{ Gui::AllocRef{ deferredJobAlloc } };
		auto jobQueue = Gui::Widget::WidgetEvent_DeferredJobQueue{ jobQueueBackend };

		DispatchPressFromRayCommon_DispatchParamsT params = {
			.guiPlane = guiPlane,
			.hitPixel = hitPixel,
			.jobQueue = jobQueue,
			.rectCollection = rectCollection,
			.transientAlloc = transientAlloc,
			.windowHandler = windowHandler,
			.windowInfo = windowInfo, };
		dispatchFn(params);

		// The dispatch has fully unwound — now it is safe to invoke any jobs that wanted to mutate
		// the UI tree, and to action a layer-removal requested through the window handler.
		jobQueueBackend.Flush(Std::AnyRef{ scene });

		if (removeFrontmostLayer) {
			guiPlane.frontmostLayer = Std::nullOpt;
		}

		// Capture bookkeeping: a press-down captures this pointer onto the plane so its later move
		// and release events keep flowing here even off-bounds; a release ends the capture. Clearing
		// any prior capture before re-capturing keeps the "held by at most one plane" invariant intact
		// even if an earlier release was missed (e.g. it landed off the host viewport).
		if (pressed) {
			ReleaseHeldPointer(scene, pointerId);
			CaptureHeldPointer(guiPlane, pointerId, hitPixel);
		} else {
			ReleaseHeldPointer(scene, pointerId);
		}

		return true;
	}

	struct DispatchMoveShared {
		Scene& scene;
		Gui::TextEngine& textEngine;
		Math::Vec3 rayOrigin;
		Math::Vec3 rayDir;
	};
	struct DispatchMoveFromRayCommon_DispatchParamsT {
		Component::GuiPlane& guiPlane;
		Math::Vec2 hitPixel;
		Gui::RectCollection const& rectCollection;
		Std::FrameAlloc& transientAlloc;
		Gui::EventWindowInfo const& windowInfo;
	};
	// Move counterpart to DispatchPressFromRayCommon: resolve the target plane (a captured plane
	// ignores its bounds; otherwise the closest plane hit within bounds), prepare the RectCollection,
	// then hand off to the per-event-type dispatcher. Move events carry no windowHandler (a layer
	// can't be opened/closed by a hover), so the path is simpler than the press one. Returns true if
	// a plane was hit.
	static bool DispatchMoveFromRayCommon(
		DispatchMoveShared const& shared,
		u8 pointerId,
		Std::FnRef<void(DispatchMoveFromRayCommon_DispatchParamsT)> const& dispatchFn)
	{
		auto& scene = shared.scene;

		auto const targetOpt = ResolvePlaneTarget(scene, pointerId, shared.rayOrigin, shared.rayDir);
		if (!targetOpt.Has()) {
			return false;
		}
		auto const& target = targetOpt.Value();

		auto* guiPlanePtr = scene.GetComponent<Component::GuiPlane>(target.entity);
		DENGINE_IMPL_ASSERT(guiPlanePtr != nullptr);
		auto& guiPlane = *guiPlanePtr;
		auto const* transformPtr = scene.GetComponent<Transform>(target.entity);
		DENGINE_IMPL_ASSERT(transformPtr != nullptr);
		auto const& transform = *transformPtr;

		constexpr auto frameAllocDefaultSize = 1024 * 1024;
		auto transientAlloc = Std::FrameAlloc::PreAllocate(frameAllocDefaultSize).Get();
		auto rectCollection = Gui::RectCollection{};
		auto [windowWidth, windowHeight, windowInfo] = CalcWindowInfo(transform);

		SceneGuiConstAppData constAppData = {
			.scene = scene, };
		PrepareRectCollection_Params prepareParams = {
			.constAppData = constAppData,
			.eventWindowInfo = windowInfo,
			.frontmostLayer = guiPlane.frontmostLayer.ToPtr(),
			.textEngine = shared.textEngine,
			.transientAlloc = transientAlloc,
			.widget = *guiPlane.widget,
			.windowWidth = windowWidth,
			.windowHeight = windowHeight };
		PrepareRectCollection(prepareParams, false, rectCollection);

		// If this plane is capturing the pointer, keep its stored position current; no-op otherwise.
		UpdateHeldPointerPosition(guiPlane, pointerId, target.hitPixel);

		DispatchMoveFromRayCommon_DispatchParamsT params = {
			.guiPlane = guiPlane,
			.hitPixel = target.hitPixel,
			.rectCollection = rectCollection,
			.transientAlloc = transientAlloc,
			.windowInfo = windowInfo, };
		dispatchFn(params);

		return true;
	}
} // namespace GuiPlaneComponentSystem

void GuiPlaneComponentSystem::DispatchAxisInputEvent(
	Scene& scene,
	Gui::TextEngine& textEngine,
	Platform::Context& platformCtx,
	Gui::Widget::AxisInputSource const& axisSource,
	Gui::AxisId axisId,
	f64 deltaTime)
{
	SceneGuiConstAppData constAppData = {
		.scene = scene, };
	SceneGuiMutAppData mutAppData = {
		.scene = scene, };

	DispatchToGuiPlanes(
		scene,
		constAppData,
		mutAppData,
		textEngine,
		platformCtx,
		[&](GuiPlaneEventContext const& ctx) {
			auto& guiPlane = ctx.guiPlane;

			auto deferredJobAlloc = Std::FrameAlloc::PreAllocate(1024).Get();
			auto jobQueueBackend = DeferredJobQueueBackend{ Gui::AllocRef{ deferredJobAlloc } };
			auto jobQueue = Gui::Widget::WidgetEvent_DeferredJobQueue{ jobQueueBackend };

			bool removeFrontmostLayer = false;
			Component::GuiPlane::GuiWindowHandler windowHandler = {};
			windowHandler.m_guiPlane = &ctx.guiPlane;
			windowHandler.m_scene = &scene;
			windowHandler.m_platformCtx = &ctx.platformCtx;
			windowHandler.m_removeCurrentLayer = &removeFrontmostLayer;

			// The frontmost layer occludes the base plane, matching the other dispatches.
			Gui::Widget* targetWidget = nullptr;
			Std::Opt<Gui::Widget::CurrentFocusWidgetData> focusOpt;
			if (guiPlane.frontmostLayer.Has()) {
				auto& frontmostLayer = guiPlane.frontmostLayer.Get();
				targetWidget = frontmostLayer.widget.Get();
				if (frontmostLayer.focusWidget.Has()) {
					auto const& fw = frontmostLayer.focusWidget.Get();
					auto entryOpt = ctx.rectCollection.GetEntry(*fw.widget);
					DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
					focusOpt = Gui::Widget::CurrentFocusWidgetData {
						.widget = fw.widget,
						.iter = entryOpt.Get(),
						.secondaryIndex = fw.secondaryIndex };
				}
			} else {
				targetWidget = guiPlane.widget.Get();
				focusOpt = CalcCurrentFocusWidgetData(guiPlane, ctx.rectCollection);
			}

			Gui::Widget::WidgetEvent_AxisInputParams params = {
				.axisSource = axisSource,
				.axisId = axisId,
				.customData = Std::AnyRef{ mutAppData },
				.deltaTime = deltaTime,
				.focusedWidget = focusOpt,
				.jobQueue = jobQueue,
				.rectCollection = ctx.rectCollection,
				.textEngine = textEngine,
				.transientAlloc = ctx.transientAlloc,
				.window = ctx.windowInfo,
				.windowHandler = windowHandler, };

			Gui::Widget::WidgetEvent_AxisInputEventState eventState = {};
			targetWidget->WidgetEvent_AxisInput(params, Std::nullOpt, eventState);

			jobQueueBackend.Flush(Std::AnyRef{ scene });

			if (removeFrontmostLayer) {
				guiPlane.frontmostLayer = Std::nullOpt;
			}

			// Only the frontmost plane receives the control, matching DispatchNavigationEvent.
			return true;
		});
}

bool GuiPlaneComponentSystem::DispatchCursorPressFromRay(
	DispatchCursorPressFromRay_Params const& params)
{
	DispatchPressShared shared = {
		.scene = params.scene,
		.textEngine = params.textEngine,
		.platformCtx = params.platformCtx,
		.rayOrigin = params.rayOrigin,
		.rayDir = params.rayDir };

	return DispatchPressFromRayCommon(
		shared,
		cursorPointerId,
		params.pressed,
		[&](DispatchPressFromRayCommon_DispatchParamsT const& paramsInner)
	{
		Gui::Widget::CursorPressParams cursorPressParams = {
			.cursorButton = params.button,
			.cursorPos = paramsInner.hitPixel.Cast<i32>(),
			.cursorPressed = params.pressed,
			.customData = Std::AnyRef{ params.scene },
			.jobQueue = paramsInner.jobQueue,
			.rectCollection = paramsInner.rectCollection,
			.textEngine = params.textEngine,
			.transientAlloc = paramsInner.transientAlloc,
			.window = paramsInner.windowInfo,
			.windowHandler = paramsInner.windowHandler, };

		// Outside presses auto-close their layer before dispatch (see DispatchPressFromRayCommon),
		// so a layer is present here only when the press landed inside it. Dispatch into it; it
		// occludes the base plane underneath.
		bool consumed = false;
		if (paramsInner.guiPlane.frontmostLayer.Has()) {
			consumed = paramsInner.guiPlane.frontmostLayer.Get().widget->CursorPress2(
				cursorPressParams,
				Std::nullOpt,
				false);
		}

		[[maybe_unused]] bool baseConsumed = paramsInner.guiPlane.widget->CursorPress2(
			cursorPressParams,
			Std::nullOpt,
			consumed);
	});
}

bool GuiPlaneComponentSystem::DispatchTouchPressFromRay(
	DispatchTouchPressFromRay_Params const& params)
{
	DispatchPressShared shared = {
		.scene = params.scene,
		.textEngine = params.textEngine,
		.platformCtx = params.platformCtx,
		.rayOrigin = params.rayOrigin,
		.rayDir = params.rayDir };

	return DispatchPressFromRayCommon(
		shared,
		params.fingerId,
		params.pressed,
		[&](DispatchPressFromRayCommon_DispatchParamsT const& paramsInner)
	{
		Gui::Widget::WidgetEvent_TouchPressEventData event = {
			.id = params.fingerId,
			.position = { (f32)paramsInner.hitPixel.x, (f32)paramsInner.hitPixel.y },
			.pressed = params.pressed };
		Gui::Widget::WidgetEvent_TouchPressParams touchPressParams = {
			.customData = Std::AnyRef{ params.scene },
			.event = event,
			.jobQueue = paramsInner.jobQueue,
			.rectCollection = paramsInner.rectCollection,
			.textEngine = params.textEngine,
			.transientAlloc = paramsInner.transientAlloc,
			.window = paramsInner.windowInfo,
			.windowHandler = paramsInner.windowHandler, };

		// Outside presses auto-close their layer before dispatch (see DispatchPressFromRayCommon),
		// so a layer is present here only when the press landed inside it. Dispatch into it; it
		// occludes the base plane underneath.
		bool consumed = false;
		if (paramsInner.guiPlane.frontmostLayer.Has()) {
			auto layerResult = paramsInner.guiPlane.frontmostLayer.Get().widget->WidgetEvent_TouchPress(
				touchPressParams,
				Std::nullOpt,
				false);
			consumed = layerResult.consumed;
		}

		[[maybe_unused]] auto result = paramsInner.guiPlane.widget->WidgetEvent_TouchPress(
			touchPressParams,
			Std::nullOpt,
			consumed);
	});
}

bool GuiPlaneComponentSystem::DispatchCursorMoveFromRay(
	DispatchCursorMoveFromRay_Params const& params)
{
	DispatchMoveShared shared = {
		.scene = params.scene,
		.textEngine = params.textEngine,
		.rayOrigin = params.rayOrigin,
		.rayDir = params.rayDir, };

	return DispatchMoveFromRayCommon(
		shared,
		cursorPointerId,
		[&](DispatchMoveFromRayCommon_DispatchParamsT const& paramsInner)
	{
		// The plane now tracks a captured pointer's last hit pixel (heldPointerIds), so a delta
		// could be derived from it here; left as {} until a widget actually consumes the delta.
		Gui::Widget::CursorMoveParams cursorMoveParams = {
			.cursorPosition = { (i32)paramsInner.hitPixel.x, (i32)paramsInner.hitPixel.y },
			.cursorPositionDelta = {},
			.customData = Std::AnyRef{ params.scene },
			.rectCollection = paramsInner.rectCollection,
			.textEngine = params.textEngine,
			.transientAlloc = paramsInner.transientAlloc,
			.window = paramsInner.windowInfo, };

		// Frontmost layer occludes the base plane underneath it.
		bool occluded = false;
		if (paramsInner.guiPlane.frontmostLayer.Has()) {
			occluded = paramsInner.guiPlane.frontmostLayer.Get().widget->CursorMove(
				cursorMoveParams,
				Std::nullOpt,
				false);
		}

		paramsInner.guiPlane.widget->CursorMove(
			cursorMoveParams,
			Std::nullOpt,
			occluded);
	});
}

bool GuiPlaneComponentSystem::DispatchTouchMoveFromRay(
	DispatchTouchMoveFromRay_Params const& params)
{
	DispatchMoveShared shared = {
		.scene = params.scene,
		.textEngine = params.textEngine,
		.rayOrigin = params.rayOrigin,
		.rayDir = params.rayDir };

	return DispatchMoveFromRayCommon(
		shared,
		params.fingerId,
		[&](DispatchMoveFromRayCommon_DispatchParamsT const& paramsInner)
	{
		Gui::Widget::WidgetEvent_TouchMoveEventData event = {
			.id = params.fingerId,
			.position = { (f32)paramsInner.hitPixel.x, (f32)paramsInner.hitPixel.y }, };
		Gui::Widget::WidgetEvent_TouchMoveParams touchMoveParams = {
			.customData = Std::AnyRef{ params.scene },
			.event = event,
			.rectCollection = paramsInner.rectCollection,
			.textEngine = params.textEngine,
			.transientAlloc = paramsInner.transientAlloc,
			.window = paramsInner.windowInfo, };

		// Frontmost layer occludes the base plane underneath it.
		bool occluded = false;
		if (paramsInner.guiPlane.frontmostLayer.Has()) {
			auto layerResult = paramsInner.guiPlane.frontmostLayer.Get().widget->WidgetEvent_TouchMove(
				touchMoveParams,
				Std::nullOpt,
				false);
			occluded = layerResult.consumed;
		}

		paramsInner.guiPlane.widget->WidgetEvent_TouchMove(
			touchMoveParams,
			Std::nullOpt,
			occluded);
	});
}

void GuiPlaneComponentSystem::RenderGuiPlane(RenderGuiPlaneParams const& params) {
	auto const& appData = params.appData;
	auto const& guiPlane = params.guiPlane;
	auto& textEngine = params.textEngine;
	auto& transientAlloc = params.transientAlloc;
	auto const& transform = params.transform;

	auto windowInfoTemp = CalcWindowInfo(transform);
	auto windowWidth = windowInfoTemp.windowWidth;
	auto windowHeight = windowInfoTemp.windowHeight;
	auto windowInfo = windowInfoTemp.windowInfo;

	auto rectCollection = Gui::RectCollection{};

	PrepareRectCollection_Params prepareParams {
		.constAppData = appData,
		.eventWindowInfo = windowInfo,
		.frontmostLayer = guiPlane.frontmostLayer.ToPtr(),
		.textEngine = textEngine,
		.transientAlloc = transientAlloc,
		.widget = *guiPlane.widget,
		.windowWidth = windowWidth,
		.windowHeight = windowHeight, };
	PrepareRectCollection(
		prepareParams,
		true,
		rectCollection);

	// Render back plane first, then frontmost layer on top.
	{
		auto guiDrawCmdOffset = params.drawEngineParams.drawCmds.size();

		auto drawEngine = GfxGuiDrawEngineImpl {
			params.drawEngineParams,
			{ windowWidth, windowHeight },
			{}, };

		Gui::Widget::Render_Params renderParams {
			.appData = Std::ConstAnyRef{ appData },
			.drawEngine = drawEngine,
			.focusedWidget = CalcCurrentFocusWidgetData(guiPlane, rectCollection),
			.rectCollection = rectCollection,
			.textEngine = textEngine,
			.transientAlloc = transientAlloc,
			.window = windowInfo, };
		guiPlane.widget->WidgetEvent_Render(renderParams, Std::nullOpt);

		auto guiDrawCmdCount = params.drawEngineParams.drawCmds.size() - guiDrawCmdOffset;

		auto transformMat =
			Math::LinAlg3D::Translate(transform.position)
			* Math::LinAlg3D::Scale_Homo(transform.scale.AsVec3(1.f))
			* Math::LinAlg3D::Rotate_Homo(transform.rotation);

		Gfx::SceneGuiPlane guiPlaneDrawCmd = {};
		guiPlaneDrawCmd.transform = transformMat;
		guiPlaneDrawCmd.drawCmdOffset = guiDrawCmdOffset;
		guiPlaneDrawCmd.drawCmdCount = guiDrawCmdCount;
		guiPlaneDrawCmd.extentPx = { windowWidth, windowHeight };
		params.guiPlaneDrawCmds.push_back(guiPlaneDrawCmd);
	}

	if (guiPlane.frontmostLayer.Has()) {
		auto const& frontmostLayer = guiPlane.frontmostLayer.Get();
		auto const& widget = *frontmostLayer.widget;
		auto widgetEntry = rectCollection.GetEntry(widget).Get();
		auto rectPair = rectCollection.GetRect(widgetEntry);

		auto guiDrawCmdOffset = params.drawEngineParams.drawCmds.size();

		auto drawEngine = GfxGuiDrawEngineImpl{
			params.drawEngineParams,
			rectPair.widgetRect.extent,
			-frontmostLayer.positionOffset };

		Std::Opt<Gui::Widget::CurrentFocusWidgetData> currentFocusOpt;
		if (frontmostLayer.focusWidget.Has()) {
			auto const& focusWidget = frontmostLayer.focusWidget.Get();
			auto entryOpt = rectCollection.GetEntry(*focusWidget.widget);
			DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
			currentFocusOpt = Gui::Widget::CurrentFocusWidgetData {
				.widget = focusWidget.widget,
				.iter = entryOpt.Get(),
				.secondaryIndex = focusWidget.secondaryIndex };
		}

		Gui::Widget::Render_Params renderParams = {
			.appData = Std::ConstAnyRef{ appData },
			.drawEngine = drawEngine,
			.focusedWidget = currentFocusOpt,
			.rectCollection = rectCollection,
			.textEngine = textEngine,
			.transientAlloc = transientAlloc,
			.window = windowInfo, };
		widget.WidgetEvent_Render(renderParams, widgetEntry);

		auto guiDrawCmdCount = params.drawEngineParams.drawCmds.size() - guiDrawCmdOffset;

		auto layerExtent = rectPair.widgetRect.extent;

		// Place the layer inside the base plane's unit-quad local frame (XY in [-0.5, 0.5], z = 0),
		// then apply the base plane's full T*S*R on top. This keeps the layer coplanar with the
		// base plane for any rotation — the layer's vertices stay at z = 0 in local space, and
		// the outer transform is what defines the world-space plane.
		Math::Vec3 const layerLocalOffset = {
			((f32)frontmostLayer.positionOffset.x + (f32)layerExtent.width * 0.5f) / (f32)windowWidth - 0.5f,
			0.5f - ((f32)frontmostLayer.positionOffset.y + (f32)layerExtent.height * 0.5f) / (f32)windowHeight,
			0.f };
		Math::Vec3 const layerLocalScale = {
			(f32)layerExtent.width / (f32)windowWidth,
			(f32)layerExtent.height / (f32)windowHeight,
			1.f };

		auto transformMat =
			Math::LinAlg3D::Translate(transform.position)
			* Math::LinAlg3D::Scale_Homo(transform.scale.AsVec3(1.f))
			* Math::LinAlg3D::Rotate_Homo(transform.rotation)
			* Math::LinAlg3D::Translate(layerLocalOffset)
			* Math::LinAlg3D::Scale_Homo(layerLocalScale);

		Gfx::SceneGuiPlane guiPlaneDrawCmd = {};
		guiPlaneDrawCmd.transform = transformMat;
		guiPlaneDrawCmd.drawCmdOffset = guiDrawCmdOffset;
		guiPlaneDrawCmd.drawCmdCount = guiDrawCmdCount;
		guiPlaneDrawCmd.extentPx = { rectPair.widgetRect.extent.width, rectPair.widgetRect.extent.height };
		params.guiPlaneDrawCmds.push_back(guiPlaneDrawCmd);
	}
}
