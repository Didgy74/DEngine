#include <DEngine/impl/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/Grid.hpp>
#include <DEngine/Gui/Text.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/LineFloatEdit.hpp>
#include <DEngine/Gui/StackLayout.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Allocator.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <iostream>
#include <vector>
#include <string>

namespace DEngine::impl
{
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTranslateArrowMesh2D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTorusMesh2D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoScaleArrowMesh2D();

	class GfxLogger : public Gfx::LogInterface
	{
	public:
		App::Context* appCtx = nullptr;

		virtual void Log(Gfx::LogInterface::Level level, Std::Span<char const> msg) override
		{
			appCtx->Log(App::LogSeverity::Error, msg);
		}

		virtual ~GfxLogger() override
		{

		}
	};

	class GfxTexAssetInterfacer : public Gfx::TextureAssetInterface
	{
		virtual char const* get(Gfx::TextureID id) const override
		{
			switch ((int)id)
			{
				case 0:
					return "data/01.ktx";
				case 1:
					return "data/Crate.png";
				case 2:
					return "data/02.png";
				default:
					return "data/01.ktx";
			}
		}
	};

	struct MyGfxWsiInterfacer : public Gfx::WsiInterface
	{
		App::Context* appCtx = nullptr;

		virtual CreateVkSurface_ReturnT CreateVkSurface(
			Gfx::NativeWindowID windowId,
			uSize vkInstance,
			void const* allocCallbacks) noexcept override
		{
			CreateVkSurface_ReturnT returnValue = {};

			auto test = appCtx->CreateVkSurface((App::WindowID)windowId, vkInstance, allocCallbacks);

			returnValue.vkResult = test.vkResult;
			returnValue.vkSurface = test.vkSurface;
			return returnValue;
		}
	};


	Gfx::Context CreateGfxContext(
		Gfx::NativeWindowID initialWindowId,
		Gfx::WsiInterface& wsiConnection,
		Gfx::TextureAssetInterface const& textureAssetConnection,
		Gfx::LogInterface& logger,
		Std::Span<char const*> requiredVkInstanceExtensions)
	{
		Gfx::InitInfo rendererInitInfo = {};
		rendererInitInfo.initialWindow = initialWindowId;
		rendererInitInfo.wsiConnection = &wsiConnection;
		rendererInitInfo.texAssetInterface = &textureAssetConnection;
		rendererInitInfo.optional_logger = &logger;
		rendererInitInfo.requiredVkInstanceExtensions = requiredVkInstanceExtensions;
		rendererInitInfo.gizmoArrowMesh = BuildGizmoTranslateArrowMesh2D();
		rendererInitInfo.gizmoCircleLineMesh = BuildGizmoTorusMesh2D();
		rendererInitInfo.gizmoArrowScaleMesh2d = BuildGizmoScaleArrowMesh2D();

		Std::Opt<Gfx::Context> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
		if (!rendererDataOpt.HasValue())
		{
			std::cout << "Could not initialize renderer." << std::endl;
			std::abort();
		}
		return static_cast<Gfx::Context&&>(rendererDataOpt.Value());
	}

	struct TempWindowHandler : public Gui::WindowHandler
	{
		App::Context* appCtx = nullptr;

		virtual void CloseWindow(Gui::WindowID) override
		{
		}

		virtual void SetCursorType(Gui::WindowID, Gui::CursorType) override
		{

		}

		virtual void HideSoftInput() override
		{
			appCtx->StopTextInputSession();
		}

		virtual void OpenSoftInput(
			Std::Span<char const> inputText,
			Gui::SoftInputFilter inputFilter) override
		{
			auto convert = [=]() {
				switch (inputFilter) {
					case Gui::SoftInputFilter::NoFilter:
						return App::SoftInputFilter::Text;
					case Gui::SoftInputFilter::SignedFloat:
						return App::SoftInputFilter::Float;
					default:
						return App::SoftInputFilter{};
				}
			};

			appCtx->StartTextInputSession(convert(), inputText);
		}
	};

	class GfxGuiEventForwarder : public App::EventForwarder
	{
	public:
		Gui::Context* guiCtx = nullptr;
		Gfx::Context* gfxCtx = nullptr;

		virtual void WindowMinimize(
			App::WindowID window,
			bool wasMinimized) override
		{
			Gui::WindowMinimizeEvent event {};
			event.windowId = (Gui::WindowID)window;
			event.wasMinimized = wasMinimized;
			guiCtx->PushEvent(event);
		}

		virtual bool WindowCloseSignal(
			App::Context& appCtx,
			App::WindowID window) override
		{
			guiCtx->DestroyWindow((Gui::WindowID)window);

			// Then destroy the window in the renderer.
			gfxCtx->DeleteNativeWindow((Gfx::NativeWindowID)window);

			// We return true to tell the appCtx to destroy the window.
			return true;
		}

		virtual void TextInputEvent(
			App::Context& ctx,
			App::WindowID windowId,
			uSize oldIndex,
			uSize oldCount,
			Std::Span<u32 const> newString) override
		{
			Gui::TextInputEvent event = {};
			event.windowId = (Gui::WindowID)windowId;
			event.oldIndex = oldIndex;
			event.oldCount = oldCount;
			event.newTextData = newString.Data();
			event.newTextSize = newString.Size();
			guiCtx->PushEvent(event);
		}

		virtual void WindowCursorEnter(
			App::WindowID window,
			bool entered) override
		{
			if (entered)
				return;
			Gui::WindowCursorExitEvent event = {};
			event.windowId = (Gui::WindowID)window;
			guiCtx->PushEvent(event);
		}

		virtual void WindowFocus(
			App::WindowID window,
			bool focused) override
		{
			if (focused)
			{
				Gui::WindowFocusEvent event = {};
				event.windowId = (Gui::WindowID)window;
				event.gainedFocus = focused;
				guiCtx->PushEvent(event);
			}
		}

		virtual void WindowMove(
			App::WindowID window,
			Math::Vec2Int position) override
		{
			Gui::WindowMoveEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.position = position;
			guiCtx->PushEvent(event);
		}

		virtual void WindowResize(
			App::Context& ctx,
			App::WindowID window,
			App::Extent extent,
			Math::Vec2UInt visibleOffset,
			App::Extent visibleExtent) override
		{
			Gui::WindowResizeEvent event = {};
			event.windowId = (Gui::WindowID)window;
			event.extent = { extent.width, extent.height };
			event.safeAreaOffset = visibleOffset;
			event.safeAreaExtent = { visibleExtent.width, visibleExtent.height };
			guiCtx->PushEvent(event);
		}

		virtual void ButtonEvent(
			App::WindowID windowId,
			App::Button button,
			bool state) override
		{
			if (button == App::Button::LeftMouse)
			{
				Gui::CursorPressEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.button = Gui::CursorButton::Primary;
				event.pressed = state;
				guiCtx->PushEvent(event);
			}
		}

		virtual void CursorMove(
			App::Context& ctx,
			App::WindowID windowId,
			Math::Vec2Int position,
			Math::Vec2Int positionDelta) override
		{
			Gui::CursorMoveEvent moveEvent = {};
			moveEvent.windowId = (Gui::WindowID)windowId;
			moveEvent.position = position;
			moveEvent.positionDelta = positionDelta;
			guiCtx->PushEvent(moveEvent);
		}

		virtual void TouchEvent(
			App::WindowID windowId,
			u8 id,
			App::TouchEventType type,
			Math::Vec2 position) override
		{
			if (type == App::TouchEventType::Down || type == App::TouchEventType::Up) {
				Gui::TouchPressEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.position = position;
				event.id = id;
				event.pressed = type == App::TouchEventType::Down;
				guiCtx->PushEvent(event);
			}
			else if (type == App::TouchEventType::Moved) {
				Gui::TouchMoveEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.position = position;
				event.id = id;
				guiCtx->PushEvent(event);
			}
		}
	};

	void SetupWindowA(
		Gui::Context& guiCtx,
		App::Context::NewWindow_ReturnT const& windowInfo)
	{
		auto* outerGrid = new Gui::Grid;
		outerGrid->SetWidth(3);

		Gui::Rect windowRect = {
			{ windowInfo.position.x, windowInfo.position.y },
			{ windowInfo.extent.width, windowInfo.extent.height } };
		Gui::Extent visibleExtent = { windowInfo.visibleExtent.width, windowInfo.visibleExtent.height };

		guiCtx.AdoptWindow(
			(Gui::WindowID)windowInfo.windowId,
			{ 0.25f, 0.f, 0.f, 1.f },
			windowRect,
			windowInfo.visibleOffset,
			visibleExtent,
			Std::Box{ outerGrid });

		{
			auto rowIndex = outerGrid->PushBackRow();

			auto* a = new Gui::Text;
			outerGrid->SetChild(0, rowIndex, Std::Box{ a });
			a->text = "Text A:";
			a->expandX = false;

			for (int i = 1; i < 2; i += 1)
			{
				auto* temp = new Gui::LineEdit;
				outerGrid->SetChild(i, rowIndex, Std::Box{ temp });
				temp->text = "Nils";
			}
		}
		{
			auto rowIndex = outerGrid->PushBackRow();

			auto* a = new Gui::Text;
			outerGrid->SetChild(0, rowIndex, Std::Box{ a });
			a->text = "B:";
			a->expandX = false;

			auto* temp = new Gui::LineFloatEdit;
			outerGrid->SetChild(1, rowIndex, Std::Box{ temp });
		}
	}

	void SetupWindowB(App::Context& appCtx, Gui::Context& guiCtx, Gfx::Context& gfxCtx)
	{
		char windowName[] = "Other window";
		auto windowInfo = appCtx.NewWindow(
			{ windowName, sizeof(windowName) - 1 },
			{ 250, 250 });

		auto* layout = new Gui::StackLayout(Gui::StackLayout::Direction::Vertical);
		//layout->padding = 25;
		//layout->padding = 5;
		layout->spacing = 10;

	}
}

int DENGINE_APP_MAIN_ENTRYPOINT(int argc, char** argv)
{
	using namespace DEngine;

	Std::NameThisThread(Std::CStrToSpan("MainThread"));

	auto appCtx = App::impl::Initialize();

	auto mainWindowInfo = appCtx.NewWindow(
		Std::CStrToSpan("Main window"),
		{ 900, 900 });

	// Initialize the renderer
	auto gfxWsiConnection = impl::MyGfxWsiInterfacer{};
	gfxWsiConnection.appCtx = &appCtx;
	auto requiredInstanceExtensions = App::GetRequiredVkInstanceExtensions();
	impl::GfxLogger gfxLogger = {};
	gfxLogger.appCtx = &appCtx;
	impl::GfxTexAssetInterfacer gfxTexAssetInterfacer{};
	Gfx::Context gfxCtx = impl::CreateGfxContext(
		(Gfx::NativeWindowID)mainWindowInfo.windowId,
		gfxWsiConnection,
		gfxTexAssetInterfacer,
		gfxLogger,
		requiredInstanceExtensions);

	impl::TempWindowHandler tempWindowHandler = {};
	tempWindowHandler.appCtx = &appCtx;

	auto guiCtx = Gui::Context::Create(tempWindowHandler, &appCtx, &gfxCtx);

	impl::SetupWindowA(guiCtx, mainWindowInfo);
	//impl::SetupWindowB(appCtx, guiCtx, gfxCtx);

	impl::GfxGuiEventForwarder gfxGuiEventForwarder;
	gfxGuiEventForwarder.guiCtx = &guiCtx;
	gfxGuiEventForwarder.gfxCtx = &gfxCtx;
	appCtx.InsertEventForwarder(gfxGuiEventForwarder);

	Gui::RectCollection sizeHintCollection;
	Std::FrameAllocator transientAlloc;

	while (true)
	{
		// This will in turn call the appropriate callbacks into the
		// GfxGuiEventForwarder object.
		App::impl::ProcessEvents(appCtx, true, 0, true);
		if (appCtx.GetWindowCount() == 0)
			break;


		// Temporary stuff
		std::vector<Gfx::GuiVertex> vertices;
		std::vector<u32> indices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;

		Gui::Context::Render2_Params renderParams {
			.rectCollection = sizeHintCollection,
			.transientAlloc = transientAlloc,
			.vertices = vertices,
			.indices = indices,
			.drawCmds = drawCmds,
			.windowUpdates = windowUpdates, };
		guiCtx.Render2(renderParams);
		if (!windowUpdates.empty()) {
			Gfx::DrawParams drawParams = {};
			drawParams.guiDrawCmds = drawCmds;
			drawParams.guiIndices = indices;
			drawParams.guiVertices = vertices;
			drawParams.nativeWindowUpdates = windowUpdates;

			for (auto& windowUpdate : drawParams.nativeWindowUpdates)
			{
				auto const windowEvents = appCtx.GetWindowEvents((App::WindowID)windowUpdate.id);
				if (windowEvents.resize)
					windowUpdate.event = Gfx::NativeWindowEvent::Resize;
				if (windowEvents.restore)
					windowUpdate.event = Gfx::NativeWindowEvent::Restore;
			}
			gfxCtx.Draw(drawParams);
		}
	}

	return 0;
}

std::vector<DEngine::Math::Vec3> DEngine::impl::BuildGizmoTranslateArrowMesh2D()
{
	std::vector<Math::Vec3> vertices;

	vertices.push_back({});
	vertices.push_back({});
	vertices.push_back({});

	return vertices;
}

std::vector<DEngine::Math::Vec3> DEngine::impl::BuildGizmoTorusMesh2D()
{
	std::vector<Math::Vec3> vertices;

	vertices.push_back({});
	vertices.push_back({});
	vertices.push_back({});

	return vertices;
}

std::vector<DEngine::Math::Vec3> DEngine::impl::BuildGizmoScaleArrowMesh2D()
{
	std::vector<Math::Vec3> vertices;

	vertices.push_back({});
	vertices.push_back({});
	vertices.push_back({});

	return vertices;
}
