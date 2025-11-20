#include <DEngine/Platform/Platform.hpp>
#include <DEngine/Platform/PlatformImpl.hpp>

#include "DEngine/Editor/Editor.hpp"
#include "DEngine/Editor/PlatformToEditorEventForwarder.hpp"
#include "DEngine/Gfx/Gfx.hpp"

#include <DEngine/Scene.hpp>
#include <DEngine/GuiPlaneComponentSystem.hpp>
#include <DEngine/SceneWidgetThemeAdapters.hpp>
#include <DEngine/Time.hpp>


#include <DEngine/Math/LinearTransform3D.hpp>
#include <DEngine/GfxGuiDrawEngineImpl.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <print>

#include <DEngine/Gui/DebugLog.hpp>
#include <DEngine/Gui/StdWidgets/LineEdit.hpp>

#ifdef DENGINE_TRACY_LINKED
	#include <tracy/Tracy.hpp>
	#include <tracy/TracyC.h>
#endif

import DEngine.FixedWidthTypes;
import DEngine.Math.Common;

void DEngine::Move::Update(
	Platform::Context& appCtx,
	Entity entity,
	Scene& scene,
	f32 deltaTime) const
{
	auto const rbPtr = scene.GetComponent<Physics::Rigidbody2D>(entity);
	b2Body* physBodyPtr = nullptr;
	if (rbPtr != nullptr) {
		DENGINE_IMPL_ASSERT(rbPtr->b2BodyPtr);
		physBodyPtr = (b2Body*)rbPtr->b2BodyPtr;
	}
	if (!physBodyPtr)
		return;
	auto& physBody = *physBodyPtr;

	auto const gamepadOpt = appCtx.GetGamepad(0);
	if (!gamepadOpt.HasValue())
		return;

	auto const& gamepadState = gamepadOpt.Value();
	if (gamepadState.GetKeyEvent(Platform::GamepadKey::A) == Platform::KeyEventType::Pressed) {
		physBody.ApplyLinearImpulseToCenter({ 0.f, 5.f }, true);
	}

	auto leftStickX = gamepadState.GetAxisValue(Platform::GamepadAxis::LeftX);
	if (Math::Abs(leftStickX) > gamepadState.stickDeadzone) {
		leftStickX *= 2.f;

		auto const currVelocity = physBody.GetLinearVelocity();
		physBody.SetLinearVelocity({ leftStickX, currVelocity.y });
		physBody.SetAwake(true);
	}
}

namespace DEngine::impl {
	class GfxLogger : public Gfx::LogInterface {
	public:
		Platform::Context* appCtx = nullptr;

		virtual void Log(Level level, Std::Span<char const> msg) override {
			appCtx->Log(Platform::LogSeverity::Error, msg);
		}

		virtual ~GfxLogger() override {
		}
	};

	class GfxTexAssetInterfacer : public Gfx::TextureAssetInterface {
		virtual char const* get(Gfx::TextureID id) const override
		{
			switch ((int)id){
				case 0:
					//return "data/01.ktx";
				//case 1:
					return "assets/Crate.png";
				case 2:
					return "assets/02.png";
				default:
					return "assets/01.ktx";
			}
		}
	};

	struct GfxWsiConnection : public Gfx::WsiInterface
	{
		Platform::Context* appCtx = nullptr;

		virtual CreateVkSurface_ReturnT CreateVkSurface(
			Gfx::NativeWindowID windowId,
			void const* vkGetInstanceProcAddr,
			uSize vkInstance,
			void const* allocCallbacks) noexcept override
		{
			auto const result = appCtx->CreateVkSurface_ThreadSafe(
				(Platform::WindowID)windowId,
				vkGetInstanceProcAddr,
				vkInstance,
				allocCallbacks);

			CreateVkSurface_ReturnT returnValue = {};
			returnValue.vkResult = result.vkResult;
			returnValue.vkSurface = result.vkSurface;
			return returnValue;
		}
	};

	Gfx::Context CreateGfxContext(
		Gfx::NativeWindowID windowId,
		Std::Opt<Gfx::Extent> const& windowSizeHint,
		Gfx::WsiInterface& wsiConnection,
		Gfx::TextureAssetInterface const& textureAssetConnection,
		Gfx::LogInterface& logger,
		Std::Span<char const*> requiredVkInstanceExtensions)
	{
		Gfx::InitInfo rendererInitInfo = {};
		rendererInitInfo.initialWindow = windowId;
		rendererInitInfo.windowSizeHint = windowSizeHint;
		rendererInitInfo.wsiConnection = &wsiConnection;
		rendererInitInfo.texAssetInterface = &textureAssetConnection;
		rendererInitInfo.optional_logger = &logger;
		rendererInitInfo.requiredVkInstanceExtensions = requiredVkInstanceExtensions;
		rendererInitInfo.gizmoArrowMesh = Editor::BuildGizmoTranslateArrowMesh2D();
		rendererInitInfo.gizmoCircleLineMesh = Editor::BuildGizmoTorusMesh2D();
		rendererInitInfo.gizmoArrowScaleMesh2d = Editor::BuildGizmoScaleArrowMesh2D();
		Std::Opt<Gfx::Context> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
		if (!rendererDataOpt.HasValue()) {
			std::cout << "Could not initialize renderer." << std::endl;
			std::abort();
		}
		return static_cast<Gfx::Context&&>(rendererDataOpt.Value());
	}

	[[nodiscard]] Std::Box<Gui::Widget> SetupGuiPlaneWidget(Component::GuiPlane::ConstructorId);

	void CopyTransformToPhysicsWorld(
		Scene& scene);

	void SetupInitialEntities(Scene& scene);

	void RunPhysicsStep(
		Scene& scene);
	
	void SubmitRendering(
		Gfx::Context& gfxData,
		Platform::Context& appCtx,
		Editor::Context& editorCtx,
		Scene const& scene);
}

int DENGINE_MAIN_ENTRYPOINT(int argc, char const* const* argv) {
	using namespace DEngine;

	Std::NameThisThread(Std::CStrToSpan("MainThread"));

	Time::Initialize();
	auto appCtx = Platform::impl::Initialize();

	Gui::SetDebugLogOutput([&](Std::Span<char const> in) {
		appCtx.Log(Platform::LogSeverity::Debug, in);
	});

	auto mainWindowCreateResultOpt = appCtx.NewWindow(
		Std::CStrToSpan("Main window"),
		{ 2500, 2000 });
	if (!mainWindowCreateResultOpt.Has()) {
		throw std::runtime_error{ "Unable to make main window" };
	}
	auto& mainWindowCreateResult = mainWindowCreateResultOpt.Get();

	impl::GfxWsiConnection gfxWsiConnection = {};
	gfxWsiConnection.appCtx = &appCtx;
	// Initialize the renderer
	auto requiredInstanceExtensions = Platform::GetRequiredVkInstanceExtensions();
	impl::GfxLogger gfxLogger = {};
	gfxLogger.appCtx = &appCtx;
	impl::GfxTexAssetInterfacer gfxTexAssetInterfacer = {};
	auto gfxCtx = impl::CreateGfxContext(
		(Gfx::NativeWindowID)mainWindowCreateResult.windowId,
		Gfx::Extent{ mainWindowCreateResult.extent.width, mainWindowCreateResult.extent.height },
		gfxWsiConnection,
		gfxTexAssetInterfacer,
		gfxLogger,
		requiredInstanceExtensions.ToSpan());

	Scene myScene;
	myScene.m_createGuiPlaneWidgetFn = &impl::SetupGuiPlaneWidget;

	impl::SetupInitialEntities(myScene);

	Editor::Context::CreateInfo editorCreateInfo = {
		.appCtx = appCtx,
		.gfxCtx = gfxCtx,
		.scene = myScene };
	editorCreateInfo.newWindowInfo = mainWindowCreateResult;
	auto editorCtx = Editor::Context::Create(editorCreateInfo);
	editorCtx.SelectEntity((Entity)0);

	while (true) {
#ifdef DENGINE_TRACY_LINKED
		TracyCZoneNS(tracy_mainTick, "Main tick", 20, true);
#endif

		Time::TickStart();
		{
			auto eventForwarder = Editor::PlatformToEditorEventForwarder(editorCtx);
			auto sceneEventForwarder = PlatformToSceneEventForwarder(
				myScene,
				editorCtx.GetTextEngineImpl());
			eventForwarder.m_extraEventForwarder = &sceneEventForwarder;
			Platform::impl::ProcessEvents(
				appCtx,
				false,
				0,
				false,
				&eventForwarder);
		}

		if (appCtx.GetWindowCount() == 0) {
			break;
		}

		editorCtx.ProcessEvents(Time::Delta());

		Scene* renderedScene = &myScene;

		if (editorCtx.IsSimulating()) {
			Scene& scene = editorCtx.GetActiveScene();
			renderedScene = &scene;

			// Editor can move stuff around, so we need to update the physics world.
			impl::CopyTransformToPhysicsWorld(scene);

			//Physics::Update(myScene, Time::Delta());

			for (auto const& [entity, moveComponent] : scene.GetAllComponents<Move>())
				moveComponent.Update(appCtx, entity, scene, Time::Delta());

			impl::RunPhysicsStep(scene);
		}

		impl::SubmitRendering(
			gfxCtx,
			appCtx,
			editorCtx, 
			*renderedScene);

		#ifdef DENGINE_TRACY_LINKED
			TracyCZoneEnd(tracy_mainTick);
		#endif

		#ifdef DENGINE_TRACY_LINKED
			FrameMark;
		#endif
	}

	return 0;
}

void DEngine::impl::CopyTransformToPhysicsWorld(Scene& scene) {
	// First copy our positions into every physics body
	for (auto const& physicsBodyPair : scene.GetAllComponents<Physics::Rigidbody2D>()) {
		Entity a = physicsBodyPair.a;

		Transform const* transformPtr = scene.GetComponent<Transform>(a);
		DENGINE_IMPL_ASSERT(transformPtr != nullptr);
		Transform const& transform = *transformPtr;
		auto* pBody = static_cast<b2Body*>(physicsBodyPair.b.b2BodyPtr);
		pBody->SetTransform(
			{ transform.position.x, transform.position.y },
			Math::degToRad * transform.rotation.ToEulerAngles().z);
	}
}

void DEngine::impl::SetupInitialEntities(Scene& scene) {
	{
		Entity ent = scene.NewEntity();

		Transform transform = {};
		transform.position.x = 0.f;
		transform.position.y = 0.f;
		transform.rotation = Math::UnitQuat::FromEulerAngles(0, 30, 0);
		scene.AddComponent(ent, transform);

		Component::GuiPlane guiPlane = {};
		guiPlane.constructorId = (Component::GuiPlane::ConstructorId)0;
		guiPlane.widget = impl::SetupGuiPlaneWidget(guiPlane.constructorId);

		scene.AddComponent(ent, Std::Move(guiPlane));

		Physics::Rigidbody2D rb = {};
		rb.type = Physics::Rigidbody2D::Type::Dynamic;
		scene.AddComponent(ent, rb);
	}

	{
		Entity ent = scene.NewEntity();

		Transform transform = {};
		transform.position.x = 1.5f;
		transform.position.y = 0.f;
		transform.scale.x = 1.5f;
		//transform.rotation = 0.707f;
		scene.AddComponent(ent, transform);

		Component::GuiPlane guiPlane = {};
		guiPlane.constructorId = (Component::GuiPlane::ConstructorId)0;
		guiPlane.widget = impl::SetupGuiPlaneWidget(guiPlane.constructorId);

		scene.AddComponent(ent, Std::Move(guiPlane));

		Physics::Rigidbody2D rb = {};
		rb.type = Physics::Rigidbody2D::Type::Static;
		scene.AddComponent(ent, rb);
	}

	{
		Entity ent = scene.NewEntity();

		Transform transform = {};
		transform.position.x = 0.5f;
		transform.position.y = -1.5f;
		//transform.rotation = 0.707f;

		//transform.scale = { 1.f, 1.f };
		scene.AddComponent(ent, transform);

		Gfx::TextureID textureId{ 0 };
		scene.AddComponent(ent, textureId);

		Physics::Rigidbody2D rb = {};
		rb.type = Physics::Rigidbody2D::Type::Static;
		scene.AddComponent(ent, rb);

		Move move = {};
		scene.AddComponent(ent, move);
	}

	/*
	{
		Entity ent = myScene.NewEntity();

		Transform transform = {};
		transform.position.x = 0.f;
		transform.position.y = -2.f;
		//transform.rotation = 0.707f;
		transform.scale.x = 5.f;
		myScene.AddComponent(ent, transform);

		Gfx::TextureID textureId{ 0 };
		myScene.AddComponent(ent, textureId);

		Physics::Rigidbody2D rb = {};
		rb.type = Physics::Rigidbody2D::Type::Static;
		myScene.AddComponent(ent, rb);

		Move move = {};
		myScene.AddComponent(ent, move);
	}
	*/
}

void DEngine::impl::RunPhysicsStep(Scene& scene)
{
	CopyTransformToPhysicsWorld(scene);

	scene.physicsWorld->Step(Time::Delta(), 8, 8);

	// Then copy the stuff back
	for (auto const& physicsBodyPair : scene.GetAllComponents<Physics::Rigidbody2D>()) {
		Entity a = physicsBodyPair.a;
		auto* transformPtr = scene.GetComponent<Transform>(a);
		DENGINE_IMPL_ASSERT(transformPtr != nullptr);
		auto& transform = *transformPtr;

		auto* pBody = static_cast<b2Body*>(physicsBodyPair.b.b2BodyPtr);
		auto physicsBodyTransform = pBody->GetTransform();
		transform.position = { physicsBodyTransform.p.x, physicsBodyTransform.p.y };
		transform.rotation = Math::UnitQuat::FromEulerAngles(
			0,
			0,
			Math::radToDeg * physicsBodyTransform.q.GetAngle());
	}
}

void DEngine::impl::SubmitRendering(
	Gfx::Context& gfxData,
	Platform::Context& appCtx,
	Editor::Context& editorCtx,
	Scene const& scene)
{
	Gfx::DrawParams params = {};

	for (auto const& item : scene.GetAllComponents<Gfx::TextureID>()) {
		auto const& entity = item.a;

		// First check if this entity has a position
		auto const* transformPtr = scene.GetComponent<Transform>(entity);
		if (transformPtr == nullptr) {
			continue;
		}
		auto const& transform = *transformPtr;

		params.textureIDs.push_back(item.b);

		auto transformMat =
			Math::LinAlg3D::Translate(transform.position)
			* Math::LinAlg3D::Rotate_Homo(transform.rotation)
			* Math::LinAlg3D::Scale_Homo(transform.scale.AsVec3(1.f));
		params.transforms.push_back(transformMat);
	}

	auto editorDrawData = editorCtx.GetDrawInfo();
	params.guiVertices = editorDrawData.vertices;
	params.guiIndices = editorDrawData.indices;
	params.guiDrawCmds = editorDrawData.drawCmds;
	params.nativeWindowUpdates = editorDrawData.windowUpdates;
	params.viewportUpdates = editorDrawData.viewportUpdates;
	params.lineVertices = editorDrawData.lineVertices;
	params.lineDrawCmds = editorDrawData.lineDrawCmds;
	params.guiUtfValues = editorDrawData.utfValues;
	params.guiTextGlyphRects = editorDrawData.textGlyphRects;
	GfxGuiDrawEngineImpl::Params gfxGuiDrawEngineParams = {
		.drawCmds = params.guiDrawCmds,
		.indices = params.guiIndices,
		.textGlyphRects = params.guiTextGlyphRects,
		.utfValues = params.guiUtfValues,
		.vertices = params.guiVertices, };

	auto transientAlloc = Std::FrameAlloc::PreAllocate(1024 * 1024).Get();
	for (auto const& [entity, guiPlane] : scene.GetAllComponents<Component::GuiPlane>()) {
		// First check if this entity has a position
		auto const* transformPtr = scene.GetComponent<Transform>(entity);
		if (transformPtr == nullptr) {
			continue;
		}
		auto const& transform = *transformPtr;

		auto& textEngine = editorCtx.GetTextEngineImpl();

		GuiPlaneComponentSystem::RenderGuiPlaneParams renderGuiParams {
			.appData = Std::ConstAnyRef{ scene },
			.drawEngineParams = gfxGuiDrawEngineParams,
			.guiPlane = guiPlane,
			.guiPlaneDrawCmds = params.guiPlanes,
			.textEngine = textEngine,
			.transform = transform,
			.transientAlloc = transientAlloc, };

		GuiPlaneComponentSystem::RenderGuiPlane(renderGuiParams);
		transientAlloc.Reset();
	}

	for (auto& windowUpdate : params.nativeWindowUpdates) {
		auto windowEventFlags = appCtx.GetWindowEventFlags((Platform::WindowID)windowUpdate.id);
		if ((u64)windowEventFlags > 0) { // Some event did happen.
			if (Platform::Contains(windowEventFlags, Platform::WindowEventFlag::Restore)) {
				windowUpdate.event = Gfx::NativeWindowEvent::Restore;
			} else if (Platform::Contains(windowEventFlags, Platform::WindowEventFlag::Resize)) {
				windowUpdate.event = Gfx::NativeWindowEvent::Resize;
			}
		}
	}

	{
		// Submit all queued bitmap upload jobs
		auto& textEngine = editorCtx.GetTextEngineImpl();
		textEngine.FlushQueuedJobs(gfxData);
	}

	if (!params.nativeWindowUpdates.empty()) {
		gfxData.Draw(params);
	}
}

DEngine::Std::Box<DEngine::Gui::Widget> DEngine::impl::SetupGuiPlaneWidget(
	Component::GuiPlane::ConstructorId)
{
	using namespace Gui;

	Widget* topWidget = nullptr;

	auto* outerHorizontalStack = new StackLayout;
	topWidget = outerHorizontalStack;
	outerHorizontalStack->direction = StackLayout::Dir::Horizontal;
	outerHorizontalStack->expandNonDirection = true;
	outerHorizontalStack->getThemeFn = SceneWidgetThemeAdapters::stackLayout;

	{
		auto* leftVerticalStack = new StackLayout;
		outerHorizontalStack->AddWidget(Std::BoxAdopt(leftVerticalStack));
		leftVerticalStack->direction = StackLayout::Dir::Vertical;
		leftVerticalStack->getThemeFn = SceneWidgetThemeAdapters::stackLayout;

		auto* scrollArea = new ScrollArea;
		leftVerticalStack->AddWidget(Std::BoxAdopt(scrollArea));
		//scrollArea->getThemeFn = SceneWidgetThemeAdapters::scrollArea;

		auto* innerScrollStack = new StackLayout;
		scrollArea->child = Std::BoxAdopt(innerScrollStack);
		innerScrollStack->direction = StackLayout::Dir::Vertical;
		innerScrollStack->getThemeFn = SceneWidgetThemeAdapters::stackLayout;

		for (int i = 0; i < 25; i++) {
			auto* label = new Text;
			innerScrollStack->AddWidget(Std::BoxAdopt(label));
			label->getThemeFn = SceneWidgetThemeAdapters::text;
			label->text = std::format("Label {}", i);
		}

		auto* btn = new Button;
		leftVerticalStack->AddWidget(Std::BoxAdopt(btn));
		btn->text = std::format("Left half button");
		btn->getThemeFn = SceneWidgetThemeAdapters::button;
		btn->activateFn = [](Gui::Button::ActivateParams const& params) {
			std::cout << "Left half button activated";
		};

		auto* lineEdit = new LineEdit;
		leftVerticalStack->AddWidget(Std::BoxAdopt(lineEdit));
		lineEdit->text = "Edit me!";
	}

	auto* verticalStack = new StackLayout;
	outerHorizontalStack->AddWidget(Std::BoxAdopt(verticalStack));
	verticalStack->direction = StackLayout::Dir::Vertical;
	verticalStack->getThemeFn = SceneWidgetThemeAdapters::stackLayout;
	{
		auto* buttonGroup = new ButtonGroup;
		verticalStack->AddWidget(Std::BoxAdopt(buttonGroup));
		buttonGroup->getThemeFn = SceneWidgetThemeAdapters::buttonGroup;

		ButtonGroup::InternalButton left = {};
		left.title = "Left";
		buttonGroup->buttons.push_back(Std::Move(left));

		ButtonGroup::InternalButton center = {};
		center.title = "Center";
		buttonGroup->buttons.push_back(Std::Move(center));

		ButtonGroup::InternalButton right = {};
		right.title = "Right";
		buttonGroup->buttons.push_back(Std::Move(right));
	}

	{
		auto* btn = new Button;
		verticalStack->AddWidget(Std::BoxAdopt(btn));
		btn->text = "Top button";
		btn->getThemeFn = SceneWidgetThemeAdapters::button;
		btn->activateFn = [](Gui::Button::ActivateParams const& params) {
			std::println("Top button activated");
		};
	}

	{
		auto* collapsingHeader = new CollapsingHeader;
		verticalStack->AddWidget(Std::BoxAdopt(collapsingHeader));
		collapsingHeader->title = "Collapsible Section";
		collapsingHeader->collapsed = true;
		collapsingHeader->getThemeFn = SceneWidgetThemeAdapters::collapsingHeader;
		auto* childStack = new StackLayout;
		collapsingHeader->child = Std::BoxAdopt(childStack);
		childStack->direction = StackLayout::Dir::Vertical;
		childStack->getThemeFn = SceneWidgetThemeAdapters::stackLayout;
		for (int i = 0; i < 3; i++) {
			auto* btn = new Button;
			childStack->AddWidget(Std::BoxAdopt(btn));
			btn->text = std::format("Nested {}", i);
			btn->getThemeFn = SceneWidgetThemeAdapters::button;
			btn->activateFn = [i](Gui::Button::ActivateParams const& params) {
				std::println("Nested button {} activated", i);
			};
		}
	}

	for (int i = 0; i < 2; i++) {
		auto* text = new Text;
		verticalStack->AddWidget(Std::BoxAdopt(text));
		text->getThemeFn = SceneWidgetThemeAdapters::text;
		text->text = std::format("Text {}", i);

		auto* btn = new Button;
		verticalStack->AddWidget(Std::BoxAdopt(btn));
		btn->text = std::format("Button {}", i);
		btn->getThemeFn = SceneWidgetThemeAdapters::button;

		auto* dropdown = new Dropdown;
		dropdown->getThemeFn = SceneWidgetThemeAdapters::dropdown;
		verticalStack->AddWidget(Std::BoxAdopt(dropdown));
		dropdown->items.push_back("Item 1");
		dropdown->items.push_back("Item 2");
		dropdown->items.push_back("Item 3");
		dropdown->items.push_back("Item 4");
		dropdown->items.push_back("Item 5");
	}

	return Std::BoxAdopt(topWidget);
}
