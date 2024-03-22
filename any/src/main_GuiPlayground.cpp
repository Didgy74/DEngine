#include <DEngine/impl/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/Button.hpp>
#include <DEngine/Gui/ButtonGroup.hpp>
#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Gui/Text.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/CollapsingHeader.hpp>
#include <DEngine/Gui/ScrollArea.hpp>
#include <DEngine/Gui/LineList.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/Vector.hpp>


#include <iostream>
#include <vector>
#include <string>

namespace DEngine::GuiPlayground
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
		virtual char const* get(Gfx::TextureID id) const override {
			switch ((int)id) {
				case 0: return "data/01.ktx";
				case 1: return "data/Crate.png";
				case 2: return "data/02.png";
				default: return "data/01.ktx";
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

			auto test = appCtx->CreateVkSurface_ThreadSafe((App::WindowID)windowId, vkInstance, allocCallbacks);

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
		if (!rendererDataOpt.HasValue()) {
			std::cout << "Could not initialize renderer." << std::endl;
			std::abort();
		}
		return static_cast<Gfx::Context&&>(rendererDataOpt.Value());
	}

	struct TempWindowHandler : public Gui::WindowHandler
	{
		Platform::Context* appCtx = nullptr;

		virtual void CloseWindow(Gui::WindowID) override {
		}

		void SetCursorType(Gui::WindowID, Gui::CursorType) override {

		}

		void HideSoftInput() override {
			appCtx->StopTextInputSession();
		}

		void OpenSoftInput(
			Gui::WindowID windowId,
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
			appCtx->StartTextInputSession(
				(Platform::WindowID)windowId,
				convert(),
				inputText);
		}

		void UpdateTextInputConnection(
			u64 selIndex,
			u64 selCount,
			Std::Span<u32 const> inputText) override
		{
			appCtx->UpdateTextInputConnection(selIndex, selCount, inputText);
		}

		void UpdateTextInputConnectionSelection(u64 selIndex, u64 selCount) override {
			appCtx->UpdateTextInputConnectionSelection(selIndex, selCount);
		}
	};

	struct PlatformToGuiEventForwarder : Platform::EventForwarder {
		Std::FnScratchList<Gui::Context&> fnList;

		void WindowResize(
			Platform::Context& appCtx,
			Platform::WindowID window,
			Platform::Extent extent,
			Math::Vec2UInt visibleOffset,
			Platform::Extent visibleExtent) override
		{
			fnList.Push([=](Gui::Context& guiCtx) {
				Gui::WindowResizeEvent event = {};
				event.windowId = (Gui::WindowID)window;
				event.extent = { extent.width, extent.height };
				event.safeAreaOffset = { visibleOffset.x, visibleOffset.y };
				event.safeAreaExtent = { visibleExtent.width, visibleExtent.height };
				guiCtx.PushEvent(event);
			});
		}

		void CursorMove(
			Platform::Context& appCtx,
			Platform::WindowID window,
			Math::Vec2Int position,
			Math::Vec2Int positionDelta) override
		{
			fnList.Push([=](Gui::Context& guiCtx) {
				Gui::CursorMoveEvent event = {};
				event.windowId = (Gui::WindowID)window;
				event.position = { position.x, position.y };
				event.positionDelta = { positionDelta.x, positionDelta.y };
				guiCtx.PushEvent(event, {});
			});
		}

		void ButtonEvent(
			Platform::WindowID window,
			Platform::Button button,
			bool state) override
		{
			if (button == Platform::Button::LeftMouse) {
				fnList.Push([=](Gui::Context& guiCtx) {
					Gui::CursorPressEvent event = {};
					event.windowId = (Gui::WindowID)window;
					event.button = Gui::CursorButton::Primary;
					event.pressed = state;
					guiCtx.PushEvent(event, {});
				});
			}
		}

		void TextInputEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			u64 start,
			u64 count,
			Std::Span<u32 const> newString) override
		{
			std::vector<u32> inputText;
			inputText.resize(newString.Size());
			for (int i = 0; i < newString.Size(); i++) {
				inputText[i] = newString[i];
			}

			fnList.Push([=, text = Std::Move(inputText)](Gui::Context& guiCtx) {
				Gui::TextInputEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.start = start;
				event.count = count;
				event.newText = { text.data(), text.size() };
				guiCtx.PushEvent(event);
			});
		}

		void TextSelectionEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			u64 start,
			u64 count) override
		{
			fnList.Push([=](Gui::Context& guiCtx) {
				Gui::TextSelectionEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				event.start = start;
				event.count = count;
				guiCtx.PushEvent(event);
			});
		}

		void TextDeleteEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId) override
		{
			fnList.Push([=](Gui::Context& guiCtx) {
				Gui::TextDeleteEvent event = {};
				event.windowId = (Gui::WindowID)windowId;
				guiCtx.PushEvent(event);
			});
		}
	};

	void SetupWindowA(
		Gui::Context& guiCtx,
		App::Context::NewWindow_ReturnT const& windowInfo)
	{
		using namespace Gui;

		auto* layout = new StackLayout;
		layout->direction = StackLayout::Dir::Vertical;

		{
			auto* lineEdit = new LineEdit;
			//lineEdit->text = "Edit me";
			layout->AddWidget(Std::Box{ lineEdit });
		}


		{
			auto* guiText = new Text;
			guiText->text = "This is some text";
			guiText->color = { 1.f, 1.f, 1.f, 1.f };
			layout->AddWidget(Std::Box{ guiText });
		}


		{
			auto* innerLayout = new StackLayout;
			innerLayout->direction = StackLayout::Dir::Horizontal;

			auto* plusButton = new Button;
			plusButton->text = "Plus";
			plusButton->activateFn = [&guiCtx](Button& btn, auto) {
				guiCtx.fontScale += 0.25f;
			};
			innerLayout->AddWidget(Std::Box{ plusButton });

			auto* minusButton = new Button;
			minusButton->text = "Minus";
			minusButton->activateFn = [&guiCtx](Button& btn, auto) {
				guiCtx.fontScale -= 0.25f;
			};
			innerLayout->AddWidget(Std::Box{ minusButton });

			layout->AddWidget(Std::Box{ innerLayout });
		}

		{
			auto* guiText = new Text;
			guiText->text = "Last text";
			guiText->color = { 1.f, 1.f, 1.f, 1.f };
			layout->AddWidget(Std::Box{ guiText });
		}

		{
			auto* btnGroup = new ButtonGroup;
			btnGroup->AddButton("One");
			btnGroup->AddButton("Two");
			btnGroup->AddButton("Three");
			btnGroup->activeIndex = 0;
			layout->AddWidget(Std::Box{ btnGroup });
		}

		{
			auto* header = new CollapsingHeader();
			layout->AddWidget(Std::Box{ header });
			header->title = "Collapse me!";
			auto* innerLayout = new StackLayout(StackLayout::Dir::Vertical);
			header->child = Std::Box{ innerLayout };
			for (int i = 0; i < 3; i++) {
				auto* lineEdit = new LineEdit;
				lineEdit->text = "Edit me";
				innerLayout->AddWidget(Std::Box{ lineEdit});
			}
		}

		{
			auto* scrollArea = new ScrollArea;
			layout->AddWidget(Std::Box{ scrollArea });
			auto* lineList = new LineList;
			scrollArea->child = Std::Box{ lineList };
			for (int i = 0; i < 10; i++) {
				lineList->lines.push_back(std::to_string(i));
			}
		}

		Gui::Context::AdoptWindowInfo adoptWindowInfo {};
		adoptWindowInfo.id = (Gui::WindowID)windowInfo.windowId;
		adoptWindowInfo.rect = {
			{ windowInfo.position.x, windowInfo.position.y },
			{ windowInfo.extent.width, windowInfo.extent.height } };
		adoptWindowInfo.visibleOffset = windowInfo.visibleOffset;
		adoptWindowInfo.visibleExtent = { windowInfo.visibleExtent.width, windowInfo.visibleExtent.height };
		adoptWindowInfo.clearColor = { 0.1f, 0.1f, 0.1f, 1.f };
		adoptWindowInfo.dpiX = windowInfo.dpiX;
		adoptWindowInfo.dpiY = windowInfo.dpiY;
		adoptWindowInfo.contentScale = windowInfo.contentScale;
		adoptWindowInfo.widget = Std::Box{ layout };

		guiCtx.AdoptWindow(Std::Move(adoptWindowInfo));
	}

	void SetupWindowB(Platform::Context& platformCtx, Gui::Context& guiCtx, Gfx::Context& gfxCtx)
	{
		auto windowInfo = platformCtx.NewWindow(Std::CStrToSpan("Second Window"), { 800, 600 });

		using namespace Gui;

		auto* layout = new StackLayout;
		layout->direction = StackLayout::Dir::Vertical;

		{
			auto* lineEdit = new LineEdit;
			lineEdit->text = "Edit me";
			layout->AddWidget(Std::Box{ lineEdit });
		}

		Gui::Context::AdoptWindowInfo adoptWindowInfo {};
		adoptWindowInfo.id = (Gui::WindowID)windowInfo.windowId;
		adoptWindowInfo.rect = {
			{ windowInfo.position.x, windowInfo.position.y },
			{ windowInfo.extent.width, windowInfo.extent.height } };
		adoptWindowInfo.visibleOffset = windowInfo.visibleOffset;
		adoptWindowInfo.visibleExtent = { windowInfo.visibleExtent.width, windowInfo.visibleExtent.height };
		adoptWindowInfo.clearColor = { 0.1f, 0.1f, 0.1f, 1.f };
		adoptWindowInfo.dpiX = windowInfo.dpiX;
		adoptWindowInfo.dpiY = windowInfo.dpiY;
		adoptWindowInfo.contentScale = windowInfo.contentScale;
		adoptWindowInfo.widget = Std::Box{ layout };
		guiCtx.AdoptWindow(Std::Move(adoptWindowInfo));

		gfxCtx.AdoptNativeWindow((Gfx::NativeWindowID)windowInfo.windowId);
	}
}

int DENGINE_MAIN_ENTRYPOINT(int argc, char** argv)
{
	using namespace DEngine;

	Std::NameThisThread(Std::CStrToSpan("MainThread"));

	auto platformCtx = Platform::impl::Initialize();

	auto mainWindowInfo = platformCtx.NewWindow(
		Std::CStrToSpan("Main window"),
		{ 900, 900 });

	// Initialize the renderer
	auto gfxWsiConnection = GuiPlayground::MyGfxWsiInterfacer{};
	gfxWsiConnection.appCtx = &platformCtx;
	auto requiredInstanceExtensions = App::GetRequiredVkInstanceExtensions();
	GuiPlayground::GfxLogger gfxLogger = {};
	gfxLogger.appCtx = &platformCtx;
	GuiPlayground::GfxTexAssetInterfacer gfxTexAssetInterfacer{};
	Gfx::Context gfxCtx = GuiPlayground::CreateGfxContext(
		(Gfx::NativeWindowID)mainWindowInfo.windowId,
		gfxWsiConnection,
		gfxTexAssetInterfacer,
		gfxLogger,
		requiredInstanceExtensions);

	GuiPlayground::TempWindowHandler tempWindowHandler = {};
	tempWindowHandler.appCtx = &platformCtx;

	auto guiCtx = Gui::Context::Create(tempWindowHandler, &platformCtx, &gfxCtx);

	GuiPlayground::SetupWindowA(guiCtx, mainWindowInfo);
	//GuiPlayground::SetupWindowB(platformCtx, guiCtx, gfxCtx);



	Gui::RectCollection sizeHintCollection;
	Std::BumpAllocator transientAlloc;

	while (true)
	{
		GuiPlayground::PlatformToGuiEventForwarder eventForwarder = {};
		Platform::impl::ProcessEvents(platformCtx, true, 0, true, &eventForwarder);
		if (platformCtx.GetWindowCount() == 0)
			break;

		eventForwarder.fnList.Consume(guiCtx);

		// Temporary stuff
		std::vector<Gfx::GuiVertex> vertices;
		std::vector<u32> indices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;
		std::vector<u32> utfValues;
		std::vector<Gfx::GlyphRect> textGlyphRects;
		Gui::Context::Render2_Params renderParams {
			.rectCollection = sizeHintCollection,
			.transientAlloc = transientAlloc,
			.vertices = vertices,
			.indices = indices,
			.drawCmds = drawCmds,
			.windowUpdates = windowUpdates,
			.utfValues = utfValues,
			.textGlyphRects = textGlyphRects };
		guiCtx.Render2(renderParams, {});
		if (!windowUpdates.empty()) {
			{
				// Submit all queued bitmap upload jobs
				auto& textManager = guiCtx.GetTextManager();
				textManager.FlushQueuedJobs(gfxCtx);
			}

			Gfx::DrawParams drawParams = {};
			drawParams.guiDrawCmds = drawCmds;
			drawParams.guiIndices = indices;
			drawParams.guiVertices = vertices;
			drawParams.nativeWindowUpdates = windowUpdates;
			drawParams.guiUtfValues = utfValues;
			drawParams.guiTextGlyphRects = textGlyphRects;

			for (auto& windowUpdate : drawParams.nativeWindowUpdates)
			{
				auto windowEventFlags = platformCtx.GetWindowEventFlags((App::WindowID)windowUpdate.id);
				if (Platform::Contains(windowEventFlags, Platform::WindowEventFlag::Resize))
					windowUpdate.event = Gfx::NativeWindowEvent::Resize;
				if (Platform::Contains(windowEventFlags, Platform::WindowEventFlag::Restore))
					windowUpdate.event = Gfx::NativeWindowEvent::Restore;
			}
			gfxCtx.Draw(drawParams);
		}
	}

	return 0;
}

std::vector<DEngine::Math::Vec3> DEngine::GuiPlayground::BuildGizmoTranslateArrowMesh2D()
{
	std::vector<Math::Vec3> vertices;

	vertices.push_back({});
	vertices.push_back({});
	vertices.push_back({});

	return vertices;
}

std::vector<DEngine::Math::Vec3> DEngine::GuiPlayground::BuildGizmoTorusMesh2D()
{
	std::vector<Math::Vec3> vertices;

	vertices.push_back({});
	vertices.push_back({});
	vertices.push_back({});

	return vertices;
}

std::vector<DEngine::Math::Vec3> DEngine::GuiPlayground::BuildGizmoScaleArrowMesh2D()
{
	std::vector<Math::Vec3> vertices;

	vertices.push_back({});
	vertices.push_back({});
	vertices.push_back({});

	return vertices;
}
