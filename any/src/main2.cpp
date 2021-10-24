#include <DEngine/impl/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/ScrollArea.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/FrameAllocator.hpp>
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
		virtual void CloseWindow(Gui::WindowID) override
		{

		}

		virtual void SetCursorType(Gui::WindowID, Gui::CursorType) override
		{

		}

		virtual void HideSoftInput() override
		{

		}

		virtual void OpenSoftInput(Std::Span<char const> inputText, Gui::SoftInputFilter inputFilter) override
		{

		}
	};

	TempWindowHandler tempWindowHandler = {};

	struct GfxGuiEventForwarder : public App::EventForwarder
	{
	public:
		Gui::Context* guiCtx = nullptr;

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
	};

	void SetupWindowA(
		Gui::Context& guiCtx,
		App::Context::NewWindow_ReturnT const& windowInfo)
	{
		auto* layout = new Gui::StackLayout;
		//layout->direction = Gui::StackLayout::Direction::Vertical;
		layout->direction = Gui::StackLayout::Direction::Horizontal;
		//layout->padding = 25;
		//layout->padding = 5;
		layout->spacing = 10;

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
			Std::Box{ layout });

		auto* btnA = new Gui::Button;
		layout->AddWidget(Std::Box{ btnA });
		btnA->text = "A";
		btnA->activateFn = [layout](Gui::Button& btn)
		{
			std::cout << "A pressed" << std::endl;

			//layout->RemoveItem(1);

			auto* midA = new Gui::Button;
			layout->InsertWidget(1, Std::Box{ midA });
			midA->text = "A";
			midA->activateFn = [](auto& btn)
			{
				std::cout << "New pressed" << std::endl;
			};
		};

		{
			auto* scrollArea = new Gui::ScrollArea;
			layout->AddWidget(Std::Box{ scrollArea });

			auto* middleLayout = new Gui::StackLayout;
			scrollArea->widget = Std::Box{ middleLayout };
			//middleLayout->padding = 10;
			middleLayout->spacing = 10;
			//middleLayout->direction = Gui::StackLayout::Direction::Horizontal;
			middleLayout->direction = Gui::StackLayout::Direction::Vertical;

			auto* midA = new Gui::Button;
			middleLayout->AddWidget(Std::Box{ midA });
			midA->text = "AAAAAAA";
			midA->activateFn = [](Gui::Button& btn)
			{
				std::cout << "Middle A pressed" << std::endl;
			};

			auto* midB = new Gui::Button;
			middleLayout->AddWidget(Std::Box{ midB });
			midB->text = "A";
			midB->activateFn = [](Gui::Button& btn)
			{
				std::cout << "Middle B pressed" << std::endl;
			};

			for (int i = 0; i < 5; i++)
			{
				auto* midB = new Gui::Button;
				middleLayout->AddWidget(Std::Box{ midB });
				midB->text = std::to_string(i);
				midB->activateFn = [i](Gui::Button& btn)
				{
					std::cout << "Middle " << i << "pressed" << std::endl;
				};
			}
		}

		auto* btnB = new Gui::Button;
		layout->AddWidget(Std::Box{ btnB });
		btnB->text = "ASSSSSSeeeeeeexxx";
		btnB->activateFn = [](Gui::Button& btn)
		{
			std::cout << "B pressed" << std::endl;
		};



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

		guiCtx.AdoptWindow(
			(Gui::WindowID)windowInfo.windowId,
			{ 0.f, 0.25f, 0.f, 1.f },
			{ windowInfo.position, { windowInfo.extent.width, windowInfo.extent.height } },
			{ (u32)windowInfo.visibleOffset.x, (u32)windowInfo.visibleOffset.y },
			{ windowInfo.visibleExtent.width, windowInfo.visibleExtent.height },
			Std::Box{ layout });

		gfxCtx.AdoptNativeWindow((Gfx::NativeWindowID) windowInfo.windowId);

		auto* btnA = new Gui::Button;
		layout->AddWidget(Std::Box{ btnA });
		btnA->text = "A";
		btnA->activateFn = [layout](Gui::Button& btn)
		{
			std::cout << "A pressed" << std::endl;

			//layout->RemoveItem(1);

			auto* midA = new Gui::Button;
			layout->InsertWidget(1, Std::Box{ midA });
			midA->text = "A";
			midA->activateFn = [](auto& btn)
			{
				std::cout << "New pressed" << std::endl;
			};
		};

		{
			auto* middleLayout = new Gui::StackLayout;
			layout->AddWidget(Std::Box{ middleLayout });
			//middleLayout->padding = 10;
			middleLayout->spacing = 10;
			middleLayout->direction = Gui::StackLayout::Direction::Horizontal;
			//middleLayout->direction = Gui::StackLayout::Direction::Vertical;

			auto* midA = new Gui::Button;
			middleLayout->AddWidget(Std::Box{ midA });
			midA->text = "AAAAAAA";
			midA->activateFn = [](Gui::Button& btn)
			{
				std::cout << "Middle A pressed" << std::endl;
			};

			auto* midB = new Gui::Button;
			middleLayout->AddWidget(Std::Box{ midB });
			midB->text = "A";
			midB->activateFn = [](Gui::Button& btn)
			{
				std::cout << "Middle B pressed" << std::endl;
			};
		}

		auto* btnB = new Gui::Button;
		layout->AddWidget(Std::Box{ btnB });
		btnB->text = "ASSSSSSeeeeeeexxx";
		btnB->activateFn = [](Gui::Button& btn)
		{
			std::cout << "B pressed" << std::endl;
		};
	}
}

int DENGINE_APP_MAIN_ENTRYPOINT(int argc, char** argv)
{
	using namespace DEngine;

	Std::NameThisThread("MainThread");

	auto appCtx = App::impl::Initialize();

	App::Extent windowExtent = { 900, 200 };
	constexpr char mainWindowTitle[] = "Main window";
	auto mainWindowInfo = appCtx.NewWindow({ mainWindowTitle, sizeof(mainWindowTitle) - 1 }, windowExtent);

	// Initialize the renderer
	auto gfxWsiConnection = impl::MyGfxWsiInterfacer{};
	gfxWsiConnection.appCtx = &appCtx;
	auto requiredInstanceExtensions = App::RequiredVulkanInstanceExtensions();
	impl::GfxLogger gfxLogger = {};
	gfxLogger.appCtx = &appCtx;
	impl::GfxTexAssetInterfacer gfxTexAssetInterfacer{};
	Gfx::Context gfxCtx = impl::CreateGfxContext(
		(Gfx::NativeWindowID)mainWindowInfo.windowId,
		gfxWsiConnection,
		gfxTexAssetInterfacer,
		gfxLogger,
		requiredInstanceExtensions);


	auto guiCtx = Gui::Context::Create(impl::tempWindowHandler, &gfxCtx);

	impl::SetupWindowA(guiCtx, mainWindowInfo);
	//impl::SetupWindowB(appCtx, guiCtx, gfxCtx);

	impl::GfxGuiEventForwarder gfxGuiEventForwarder;
	gfxGuiEventForwarder.guiCtx = &guiCtx;
	appCtx.InsertEventInterface(gfxGuiEventForwarder);

	Gui::SizeHintCollection sizeHintCollection;
	Std::FrameAllocator transientAlloc;

	while (true)
	{
		App::impl::ProcessEvents(appCtx, App::impl::PollMode::Immediate);
		if (appCtx.GetWindowCount() == 0)
			break;

		// Temporary stuff
		std::vector<Gfx::GuiVertex> vertices;
		std::vector<u32> indices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;

		Gui::Context::Render2_Params renderParams {
			.sizeHintCollection = sizeHintCollection,
			.transientAlloc = transientAlloc,
			.vertices = vertices,
			.indices = indices,
			.drawCmds = drawCmds,
			.windowUpdates = windowUpdates, };
		guiCtx.Render2(renderParams);

		Gfx::DrawParams drawParams = {};
		drawParams.guiDrawCmds = drawCmds;
		drawParams.guiIndices = indices;
		drawParams.guiVertices = vertices;
		drawParams.nativeWindowUpdates = windowUpdates;
		gfxCtx.Draw(drawParams);
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
