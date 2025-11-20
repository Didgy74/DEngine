#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Platform/PlatformImpl.hpp>

#include <DEngine/Gui/Context/Context.hpp>
#include <DEngine/Gui/DebugLog.hpp>
#include <DEngine/Gui/StdWidgets/Button.hpp>
#include <DEngine/Gui/StdWidgets/StackLayout.hpp>

#include <DEngine/Std/Utility.hpp>

#include <DEngine/PlatformGuiGlue.hpp>

#include <format>
#include <iostream>
#include <streambuf>

#include <DEngine/GfxGuiDrawEngineImpl.hpp>
#include "DEngine/GuiTextEngineImpl.hpp"

namespace DEngine::GuiPlayground {
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTranslateArrowMesh2D();

	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTorusMesh2D();

	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoScaleArrowMesh2D();

	class GfxLogger : public Gfx::LogInterface {
	public:
		Platform::Context* appCtx = nullptr;

		virtual void Log(Level level, Std::Span<char const> msg) override
		{
			appCtx->Log(Platform::LogSeverity::Error, msg);
		}

		virtual ~GfxLogger() override {}
	};

	class GfxTexAssetInterfacer : public Gfx::TextureAssetInterface {
		virtual char const* get(Gfx::TextureID id) const override
		{
			switch ((int) id) {
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

	struct MyGfxWsiInterfacer : public Gfx::WsiInterface {
		Platform::Context* appCtx = nullptr;

		virtual CreateVkSurface_ReturnT CreateVkSurface(
			Gfx::NativeWindowID windowId,
			void const* vkGetInstanceProcAddr,
			uSize vkInstance,
			void const* allocCallbacks) noexcept override
		{
			CreateVkSurface_ReturnT returnValue = {};

			auto test = appCtx->CreateVkSurface_ThreadSafe(
				(Platform::WindowID)windowId,
				vkGetInstanceProcAddr,
				vkInstance,
				allocCallbacks);

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


	struct TempWindowHandler : public Gui::WindowHandler {
		Platform::Context* appCtx = nullptr;

		virtual void CloseWindow(Gui::WindowID) override {}

		virtual void SetCursorType(Gui::WindowID, Gui::CursorType) override {}

		virtual void HideSoftInput() override { appCtx->StopTextInputSession(); }

		virtual void OpenSoftInput(
			Gui::WindowID windowId,
			Std::Span<char const> inputText,
			u64 selectionStart,
			u64 selectionCount,
			Gui::SoftInputFilter inputFilter) override
		{
			appCtx->StartTextInputSession(
				(Platform::WindowID) windowId,
				PlatformGuiGlue::ToPlatformSoftInputFilter(inputFilter),
				inputText,
				selectionStart,
				selectionCount);
		}

		virtual void UpdateTextInputConnection(
			Gui::WindowID windowId,
			u64 selIndex,
			u64 selCount,
			Std::Span<u32 const> inputText) override
		{
			appCtx->UpdateTextInputConnection(
				(Platform::WindowID) windowId, selIndex, selCount, inputText);
		}

		virtual void UpdateTextInputConnectionSelection(
			Gui::WindowID windowId,
			u64 selIndex,
			u64 selCount) override
		{
			appCtx->UpdateTextInputConnectionSelection(selIndex, selCount);
		}
	};

	void SetupWindowA(Gui::Context& guiCtx, Platform::Context::NewWindow_ReturnT const& windowInfo);

	void SetupWindowB(Platform::Context& platformCtx, Gui::Context& guiCtx, Gfx::Context& gfxCtx)
	{
		/*
		auto windowInfoOpt = platformCtx.NewWindow(Std::CStrToSpan("Second Window"), {800, 600});
		if (!windowInfoOpt.Has()) {
			throw std::runtime_error("Couldn't make window oof");
		}
		auto &windowInfo = windowInfoOpt.Get();

		using namespace Gui;

		auto *layout = new StackLayout;
		layout->direction = StackLayout::Dir::Vertical; {
			auto *lineEdit = new LineEdit;
			lineEdit->text = "Edit me";
			layout->AddWidget(Std::Box{lineEdit});
		}

		Gui::Context::AdoptWindowInfo adoptWindowInfo{};
		adoptWindowInfo.id = (Gui::WindowID) windowInfo.windowId;
		adoptWindowInfo.rect = {
			{windowInfo.position.x, windowInfo.position.y},
			{windowInfo.extent.width, windowInfo.extent.height}
		};
		adoptWindowInfo.visibleOffset = windowInfo.visibleOffset;
		adoptWindowInfo.visibleExtent = {windowInfo.visibleExtent.width,
		windowInfo.visibleExtent.height}; adoptWindowInfo.clearColor = {0.1f, 0.1f, 0.1f, 1.f};
		adoptWindowInfo.dpiX = windowInfo.dpiX;
		adoptWindowInfo.dpiY = windowInfo.dpiY;
		adoptWindowInfo.contentScale = windowInfo.contentScale;
		adoptWindowInfo.widget = Std::Box{layout};
		guiCtx.AdoptWindow(Std::Move(adoptWindowInfo));

		gfxCtx.AdoptNativeWindow((Gfx::NativeWindowID) windowInfo.windowId);
		*/
	}

	void RunAccessibilityStuff(
		Platform::Context& platformCtx,
		Gui::Context const& guiCtx,
		Gui::RectCollection& guiRectCollection,
		Gui::TextEngine& textEngine,
		Std::BumpAllocator& guiTransientAlloc)
	{
		struct TempPusher : public Gui::Widget::AccessibilityInfoPusher {
			std::vector<char> text;
			std::vector<std::pair<u64, Gui::Widget::AccessibilityInfoElement>> elements;

			virtual int PushText(Std::Span<char const> in) override
			{
				int returnVal = (int) this->text.size();
				this->text.resize(this->text.size() + in.Size());
				std::memcpy(this->text.data() + returnVal, in.Data(), in.Size());
				return returnVal;
			}

			[[nodiscard]] bool ContainsId(u64 id) const
			{
				for (auto const& item : this->elements) {
					if (item.first == id)
						return true;
				}
				return false;
			}

			virtual void
			PushElement(u64 id, Gui::Widget::AccessibilityInfoElement const& in) override
			{
				DENGINE_IMPL_ASSERT(!this->ContainsId(id));
				this->elements.emplace_back(id, in);
			}
		};
		TempPusher pusher;

		guiCtx.Event_Accessibility({}, guiTransientAlloc, guiRectCollection, textEngine, pusher);

		platformCtx.UpdateAccessibilityData(
			(Platform::WindowID) 0,
			{ (int) pusher.elements.size(),
			  [&](auto index) {
				  auto const& id = pusher.elements[index].first;
				  auto const& item = pusher.elements[index].second;
				  return PlatformGuiGlue::MakePlatformAccessibilityElement(id, item);
			  } },
			{ pusher.text.data(), pusher.text.size() });
	}
} // namespace DEngine::GuiPlayground

int DENGINE_MAIN_ENTRYPOINT(int argc, char const* const* argv)
{
	using namespace DEngine;

	Std::NameThisThread(Std::CStrToSpan("MainThread"));

	auto platformCtx = Platform::impl::Initialize();

	// Forward the Gui debug log to our Platform.
	Gui::SetDebugLogOutput([&](Std::Span<char const> msg) {
		platformCtx.Log(Platform::LogSeverity::Debug, msg);
	});


	auto mainWindowInfoOpt = platformCtx.NewWindow(Std::CStrToSpan("Main window"), { 900, 900 });
	if (!mainWindowInfoOpt.Has()) {
		throw std::runtime_error("Unable to make main window oof");
	}
	auto& mainWindowInfo = mainWindowInfoOpt.Get();

	// Initialize the renderer
	auto gfxWsiConnection = GuiPlayground::MyGfxWsiInterfacer{};
	gfxWsiConnection.appCtx = &platformCtx;
	auto requiredInstanceExtensions = Platform::GetRequiredVkInstanceExtensions();
	GuiPlayground::GfxLogger gfxLogger = {};
	gfxLogger.appCtx = &platformCtx;
	GuiPlayground::GfxTexAssetInterfacer gfxTexAssetInterfacer{};


	Gfx::Context gfxCtx = GuiPlayground::CreateGfxContext(
		(Gfx::NativeWindowID) mainWindowInfo.windowId,
		gfxWsiConnection,
		gfxTexAssetInterfacer,
		gfxLogger,
		requiredInstanceExtensions);

	GuiPlayground::TempWindowHandler windowHandler = {};
	windowHandler.appCtx = &platformCtx;

	auto guiCtx = Gui::Context::Create(windowHandler);

	GuiPlayground::SetupWindowA(guiCtx, mainWindowInfo);
	// Conditionally open up WindowB if we can
	// if (platformCtx.CanCreateNewWindow())
	// GuiPlayground::SetupWindowB(platformCtx, guiCtx, gfxCtx);*/

	GuiTextEngineImpl guiTextEngine = {};
	Gui::RectCollection guiRectCollection;
	Std::BumpAllocator guiTransientAlloc;

	while (true) {
		PlatformGuiGlue::BasicPlatformToGuiEventForwarder eventForwarder = {};
		eventForwarder.emulateCursorAsTouch = true;

		Platform::impl::ProcessEvents(
			platformCtx,
			false,
			0,
			true,
			&eventForwarder);
		if (platformCtx.GetWindowCount() == 0)
			break;

		eventForwarder.Consume(guiCtx, guiTextEngine);

		/*
		GuiPlayground::RunAccessibilityStuff(
			platformCtx, guiCtx, guiRectCollection, guiTextEngine, guiTransientAlloc);
		*/


		// Temporary stuff
		std::vector<Gfx::GuiVertex> vertices;
		std::vector<u32> indices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;
		std::vector<u32> utfValues;
		std::vector<Gfx::GlyphRect> textGlyphRects;
		GfxGuiDrawEngineImpl gfxGuiRenderingGlue(
			vertices, indices, drawCmds, utfValues, textGlyphRects, windowUpdates);

		Gui::Context::Render2_Params renderParams{
			.rectCollection = guiRectCollection,
			.transientAlloc = guiTransientAlloc,
			.textEngine = guiTextEngine,
		};
		guiCtx.Render2(renderParams, gfxGuiRenderingGlue, {});
		if (!windowUpdates.empty()) {
			// Submit all queued bitmap upload jobs
			guiTextEngine.FlushQueuedJobs(gfxCtx);

			Gfx::DrawParams drawParams = {};
			drawParams.guiDrawCmds = drawCmds;
			drawParams.guiIndices = indices;
			drawParams.guiVertices = vertices;
			drawParams.nativeWindowUpdates = windowUpdates;
			drawParams.guiUtfValues = utfValues;
			drawParams.guiTextGlyphRects = textGlyphRects;

			for (auto& windowUpdate : drawParams.nativeWindowUpdates) {
				auto windowEventFlags =
					platformCtx.GetWindowEventFlags((Platform::WindowID) windowUpdate.id);
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

void DEngine::GuiPlayground::SetupWindowA(
	Gui::Context& guiCtx,
	Platform::Context::NewWindow_ReturnT const& windowInfo)
{
	using namespace Gui;

	Widget* topWidget = nullptr;

	auto* outerHorizontalStack = new StackLayout;
	topWidget = outerHorizontalStack;
	outerHorizontalStack->direction = StackLayout::Dir::Horizontal;
	outerHorizontalStack->getThemeFn = [](Gui::StackLayout::GetThemeParamsT const& in) {
		Gui::StackLayout::Theme theme = {};
		theme.spacing = Gui::Distance::Mm(1.5, true);
		return theme;
	};

	{
		auto* btn = new Button;
		outerHorizontalStack->AddWidget(Std::BoxAdopt(btn));
		btn->text = std::format("Right half button");
	}

	auto* verticalStack = new StackLayout;
	outerHorizontalStack->AddWidget(Std::BoxAdopt(verticalStack));
	verticalStack->direction = StackLayout::Dir::Vertical;
	verticalStack->getThemeFn = [](Gui::StackLayout::GetThemeParamsT const& in) {
		Gui::StackLayout::Theme theme = {};
		theme.spacing = Gui::Distance::Mm(1.5, true);
		return theme;
	};

	{
		auto* horiStack = new StackLayout;
		verticalStack->AddWidget(Std::BoxAdopt(horiStack));
		horiStack->direction = StackLayout::Dir::Horizontal;
		horiStack->getThemeFn = [](Gui::StackLayout::GetThemeParamsT const& in) {
			Gui::StackLayout::Theme theme = {};
			theme.spacing = Gui::Distance::Mm(1.5, true);
			return theme;
		};

		for (int i = 0; i < 3; i++) {
			auto* btn = new Button;
			horiStack->AddWidget(Std::BoxAdopt(btn));
			btn->text = std::format("Hori {}", i);
		}
	}

	{
		auto* btn = new Button;
		verticalStack->AddWidget(Std::BoxAdopt(btn));
		btn->text = "Top button";
	}

	for (int i = 0; i < 10; i++) {
		auto* btn = new Button;
		verticalStack->AddWidget(Std::BoxAdopt(btn));
		btn->text = std::format("Button {}", i);
	}

	auto adoptWindowInfo = PlatformGuiGlue::MakeGuiWindowAdoptInfo(
		windowInfo,
		{ 0.1f, 0.1f, 0.1f, 1.f },
		Std::BoxAdopt(topWidget));
	adoptWindowInfo.contentScale = 1.5;

	if (Platform::activeOS == Platform::OS::Windows) {
		/*
		auto& systemInsets = adoptWindowInfo.insets.m_insets[(int)WindowInsetSource::SystemBar];
		systemInsets.top = 50;
		systemInsets.bottom = 50;
		*/
	}

	guiCtx.AdoptWindow(Std::Move(adoptWindowInfo));
}
