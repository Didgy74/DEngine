#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Gui/Layer.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/RectCollection.hpp>
#include <DEngine/Gui/TextManager.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

Context::Impl& Context::Internal_ImplData()
{
	DENGINE_IMPL_GUI_ASSERT(pImplData);
	return *pImplData;
}
Context::Impl const& Context::Internal_ImplData() const
{
	DENGINE_IMPL_GUI_ASSERT(pImplData);
	return *pImplData;
}

namespace DEngine::Gui::impl
{
	[[nodiscard]] auto GetWindowNodeIt(decltype(Context::Impl::windows)& windows, WindowID id)
	{
		auto windowIt = Std::FindIf(
			windows.begin(),
			windows.end(),
			[&id](auto const& item) { return item.id == id; });
		return windowIt;
	}

	[[nodiscard]] auto GetWindowNodeIt(decltype(Context::Impl::windows) const& windows, WindowID id)
	{
		auto windowIt = Std::FindIf(
			windows.begin(),
			windows.end(),
			[&id](auto const& item) { return item.id == id; });
		return windowIt;
	}

	[[nodiscard]] WindowNode* GetWindowNodePtr(Context::Impl& implData, WindowID id)
	{
		auto& windows = implData.windows;
		auto windowIt = GetWindowNodeIt(windows, id);
		if (windowIt != windows.end())
			return &*windowIt;
		else
			return nullptr;
	}

	[[nodiscard]] WindowNode const* GetWindowNodePtr(Context::Impl const& implData, WindowID id)
	{
		auto& windows = implData.windows;
		auto windowIt = GetWindowNodeIt(windows, id);
		if (windowIt != windows.end())
			return &*windowIt;
		else
			return nullptr;
	}

	void ImplData_PreDispatchStuff(Context::Impl& implData) {
		implData.transientAlloc.Reset();
	}

	void ImplData_FlushPostEventJobs(Context& ctx) {
		auto& implData = ctx.Internal_ImplData();
		for (auto const& job : implData.postEventJobs)
			job.invokeFn(job.ptr, ctx);
		auto const length = (int)implData.postEventJobs.size();
		for (int i = length; i != 0 ; i -= 1) {
			auto& job = implData.postEventJobs[i - 1];
			job.destroyFn(job.ptr);
			implData.postEventAlloc.Free(job.ptr);
		}

		implData.postEventJobs.clear();
		implData.postEventAlloc.Reset();
	}

	void BuildRectCollection(
		Context const& ctx,
		Context::Impl const& implData,
		TextManager& textManager,
		RectCollection& rectCollection,
		bool includeRendering,
		Std::FrameAlloc& transientAlloc)
	{
		rectCollection.Prepare(includeRendering);

		for (auto const& windowNode : implData.windows)
		{
			if (!windowNode.data.topLayout || windowNode.data.isMinimized)
				continue;

			auto const& windowRect = windowNode.data.rect;
			auto const& visibleOffset = windowNode.data.visibleOffset;
			auto const& visibleExtent = windowNode.data.visibleExtent;
			Rect const visibleRect = Rect::Intersection(
				{ {}, windowRect.extent },
				{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent });

			EventWindowInfo eventWindowInfo {
				.contentScale = windowNode.data.contentScale,
				.dpiX = windowNode.data.dpiX,
				.dpiY = windowNode.data.dpiY, };

			RectCollection::SizeHintPusher sizeHintPusher { rectCollection };

			Widget::GetSizeHint2_Params widgetParams = {
				.ctx = ctx,
				.window = eventWindowInfo,
				.textManager = textManager,
				.transientAlloc = transientAlloc,
				.pusher = sizeHintPusher, };
			auto const& widget = *windowNode.data.topLayout;
			widget.GetSizeHint2(widgetParams);

			if (windowNode.frontmostLayer) {
				Layer::BuildSizeHints_Params layerParams = {
					.ctx = ctx,
					.textManager = textManager,
					.window = eventWindowInfo,
					.windowRect = { {}, windowRect.extent },
					.safeAreaRect = visibleRect,
					.transientAlloc = transientAlloc,
					.pusher = sizeHintPusher };
				auto const& layer = *windowNode.frontmostLayer;
				layer.BuildSizeHints(layerParams);
			}

			transientAlloc.Reset();
		}

		// Then build rects
		for (auto& windowNode : implData.windows)
		{
			if (!windowNode.data.topLayout || windowNode.data.isMinimized)
				continue;

			auto const& windowRect = windowNode.data.rect;
			auto const& visibleOffset = windowNode.data.visibleOffset;
			auto const& visibleExtent = windowNode.data.visibleExtent;
			Rect const visibleRect = Rect::Intersection(
				{ {}, windowRect.extent },
				{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent});

			EventWindowInfo eventWindowInfo {
				.contentScale = windowNode.data.contentScale,
				.dpiX = windowNode.data.dpiX,
				.dpiY = windowNode.data.dpiY, };
			RectCollection::RectPusher rectPusher { rectCollection };

			Widget::BuildChildRects_Params widgetParams {
				.ctx = ctx,
				.window = eventWindowInfo,
				.textManager = textManager,
				.transientAlloc = transientAlloc,
				.pusher = rectPusher, };
			auto& widget = *windowNode.data.topLayout;
			auto childEntry = rectPusher.GetEntry(widget);
			rectPusher.SetRectPair(childEntry, { visibleRect, visibleRect });
			widget.BuildChildRects(
				widgetParams,
				visibleRect,
				visibleRect);

			if (windowNode.frontmostLayer) {
				auto& layer = *windowNode.frontmostLayer;
				Layer::BuildRects_Params buildRectsParams {
					.ctx = ctx,
					.window = eventWindowInfo,
					.windowRect = { {}, windowRect.extent },
					.visibleRect = visibleRect,
					.textManager = textManager,
					.transientAlloc = transientAlloc,
					.pusher = rectPusher };
				layer.BuildRects(buildRectsParams);
			}

			transientAlloc.Reset();
		}
	}
}

Context Context::Create(
	WindowHandler& windowHandler,
	App::Context* appCtx,
	Gfx::Context* gfxCtx)
{
	Context newCtx;
	newCtx.pImplData = new Impl;
	auto& implData = *newCtx.pImplData;
	
	implData.windowHandler = &windowHandler;

	implData.textManager = new TextManager;
	impl::InitializeTextManager(
		*implData.textManager,
		appCtx,
		gfxCtx);

	return static_cast<Context&&>(newCtx);
}

Context::Context(Context&& other) noexcept :
	pImplData(other.pImplData)
{
	other.pImplData = nullptr;
}

void Context::TakeInputConnection(
	Widget& widget,
	SoftInputFilter softInputFilter,
	Std::Span<char const> currentText)
{
	auto& implData = Internal_ImplData();
	if (implData.inputConnectionWidget)
	{
		DENGINE_IMPL_GUI_UNREACHABLE();
		//implData.inputConnectionWidget->InputConnectionLost();
	}
	implData.windowHandler->OpenSoftInput(
		{ currentText.Data(), currentText.Size() },
		softInputFilter);
	implData.inputConnectionWidget = &widget;
}

void Context::ClearInputConnection(
	Widget& widget)
{
	auto& implData = Internal_ImplData();
	DENGINE_IMPL_GUI_ASSERT(&widget == implData.inputConnectionWidget);

	implData.windowHandler->HideSoftInput();

	implData.inputConnectionWidget = nullptr;
}

WindowHandler& Context::GetWindowHandler() const
{
	auto& implData = Internal_ImplData();
	return *implData.windowHandler;
}

void Context::SetFrontmostLayer(
	WindowID windowId,
	Std::Box<Layer>&& layer)
{
	auto& implData = Internal_ImplData();

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.frontmostLayer = static_cast<Std::Box<Layer>&&>(layer);
}

void Context::PushEvent(TextInputEvent const& event)
{
	auto& implData = Internal_ImplData();

	auto& transientAlloc = implData.transientAlloc;
	Std::Defer _allocCleanup = [&]{ transientAlloc.Reset(); };

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	if (windowNode.data.topLayout)
	{
		windowNode.data.topLayout->TextInput(
			*this,
			transientAlloc,
			event);
	}
}

void Context::PushEvent(EndTextInputSessionEvent const& event)
{
	auto& implData = Internal_ImplData();

	auto& transientAlloc = implData.transientAlloc;
	Std::Defer _allocCleanup = [&]{ transientAlloc.Reset(); };

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	if (windowNode.data.topLayout) {
		windowNode.data.topLayout->EndTextInputSession(
			*this,
			transientAlloc,
			event);
	}
}

void Context::PushEvent(CursorPressEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& textManager = *implData.textManager;
	auto& rectCollection = implData.rectCollection;
	auto& transientAlloc = implData.transientAlloc;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	implData.cursorWindowId = event.windowId;

	impl::ImplData_PreDispatchStuff(implData);
	impl::BuildRectCollection(
		*this,
		implData,
		textManager,
		rectCollection,
		false,
		transientAlloc);

	if (windowNode.data.topLayout)
	{
		auto const localCursorPos =
			implData.cursorPosition - windowNode.data.rect.position;

		Rect const& windowRect = { {}, windowNode.data.rect.extent };
		auto const& visibleOffset = windowNode.data.visibleOffset;
		auto const& visibleExtent = windowNode.data.visibleExtent;
		Rect const visibleRect = Rect::Intersection(
			windowRect,
			{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent });

		EventWindowInfo eventWindowInfo {
			.contentScale = windowNode.data.contentScale,
			.dpiX = windowNode.data.dpiX,
			.dpiY = windowNode.data.dpiY, };

		bool eventConsumed = false;
		Layer::Press_Return pressReturn = {};
		if (windowNode.frontmostLayer)
		{
			Layer::CursorPressParams layerParams {
				.ctx = *this,
				.textManager = textManager,
				.window = eventWindowInfo,
				.rectCollection = rectCollection,
				.transientAlloc = transientAlloc,
				.event = event, };
			layerParams.windowRect = windowRect;
			layerParams.safeAreaRect = visibleRect;
			layerParams.cursorPos = localCursorPos;

			auto& layer = *windowNode.frontmostLayer;
			pressReturn = layer.CursorPress(layerParams, eventConsumed);
			eventConsumed = eventConsumed || pressReturn.eventConsumed;
		}
		if (pressReturn.destroyLayer) {
			windowNode.frontmostLayer = {};
		}

		Widget::CursorPressParams widgetParams {
			.ctx = *this,
			.window = eventWindowInfo,
			.rectCollection = rectCollection,
			.textManager = textManager,
			.transientAlloc =  transientAlloc };
		widgetParams.windowId = windowNode.id;
		widgetParams.cursorPos = localCursorPos;
		widgetParams.event = event;

		auto& widget = *windowNode.data.topLayout;

		widget.CursorPress2(
			widgetParams,
			visibleRect,
			visibleRect,
			eventConsumed);
	}

	impl::ImplData_FlushPostEventJobs(*this);
}

void Context::PushEvent(TouchMoveEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& textManager = *implData.textManager;
	auto& rectCollection = implData.rectCollection;
	auto& transientAlloc = implData.transientAlloc;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	impl::ImplData_PreDispatchStuff(implData);
	impl::BuildRectCollection(
		*this,
		implData,
		textManager,
		rectCollection,
		false,
		transientAlloc);

	if (windowNode.data.topLayout)
	{
		auto& widget = *windowNode.data.topLayout;

		auto modifiedEvent = event;

		auto const& windowRect = windowNode.data.rect;
		auto const& visibleOffset = windowNode.data.visibleOffset;
		auto const& visibleExtent = windowNode.data.visibleExtent;
		Rect const visibleRect = Rect::Intersection(
			{{}, windowRect.extent},
			{{(i32) visibleOffset.x, (i32) visibleOffset.y}, visibleExtent});

		EventWindowInfo eventWindowInfo {
			.contentScale = windowNode.data.contentScale,
			.dpiX = windowNode.data.dpiX,
			.dpiY = windowNode.data.dpiY, };

		bool cursorOccluded = false;
		if (windowNode.frontmostLayer)
		{
			Layer::TouchMoveParams layerParams {
				.ctx = *this,
				.textManager = textManager,
				.window = eventWindowInfo,
				.windowRect = windowRect,
				.safeAreaRect = visibleRect,
				.rectCollection = rectCollection,
				.transientAlloc = transientAlloc,
				.event = modifiedEvent };

			auto& layer = *windowNode.frontmostLayer;
			bool const newOccluded = layer.TouchMove2(
				layerParams,
				cursorOccluded);
			cursorOccluded = cursorOccluded || newOccluded;
		}

		Widget::TouchMoveParams widgetParams {
			.ctx = *this,
			.window = eventWindowInfo,
			.rectCollection = rectCollection,
			.textManager = textManager,
			.transientAlloc = transientAlloc,
			.windowId = windowNode.id,
			.event = modifiedEvent };

		widget.TouchMove2(
			widgetParams,
			visibleRect,
			visibleRect,
			cursorOccluded);
	}
}

void Context::PushEvent(TouchPressEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& textManager = *implData.textManager;
	auto& rectCollection = implData.rectCollection;
	auto& transientAlloc = implData.transientAlloc;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	impl::ImplData_PreDispatchStuff(implData);
	impl::BuildRectCollection(
		*this,
		implData,
		textManager,
		rectCollection,
		false,
		transientAlloc);

	if (windowNode.data.topLayout)
	{
		Rect const& windowRect = { {}, windowNode.data.rect.extent };
		auto const& visibleOffset = windowNode.data.visibleOffset;
		auto const& visibleExtent = windowNode.data.visibleExtent;
		Rect const visibleRect = Rect::Intersection(
			windowRect,
			{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent });

		EventWindowInfo eventWindowInfo {
			.contentScale = windowNode.data.contentScale,
			.dpiX = windowNode.data.dpiX,
			.dpiY = windowNode.data.dpiY, };

		bool eventConsumed = false;
		Layer::Press_Return pressReturn = {};
		if (windowNode.frontmostLayer) {
			Layer::TouchPressParams layerParams {
				.ctx = *this,
				.textManager = textManager,
				.window = eventWindowInfo,
				.windowRect = windowRect,
				.safeAreaRect = visibleRect,
				.rectCollection = rectCollection,
				.transientAlloc = transientAlloc,
				.event = event, };

			auto& layer = *windowNode.frontmostLayer;
			pressReturn = layer.TouchPress2(layerParams, eventConsumed);
			eventConsumed = eventConsumed || pressReturn.eventConsumed;
		}
		if (pressReturn.destroyLayer)
		{
			windowNode.frontmostLayer = {};
		}

		Widget::TouchPressParams widgetParams {
			.ctx = *this,
			.window = eventWindowInfo,
			.rectCollection = rectCollection,
			.textManager = textManager,
			.transientAlloc =  transientAlloc,
			.windowId = windowNode.id,
			.event = event, };
		auto& widget = *windowNode.data.topLayout;
		widget.TouchPress2(
			widgetParams,
			visibleRect,
			visibleRect,
			eventConsumed);
	}

	impl::ImplData_FlushPostEventJobs(*this);
}

void Context::PushEvent(CursorMoveEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& textManager = *implData.textManager;
	auto& rectCollection = implData.rectCollection;
	auto& transientAlloc = implData.transientAlloc;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	impl::ImplData_PreDispatchStuff(implData);
	impl::BuildRectCollection(
		*this,
		implData,
		textManager,
		rectCollection,
		false,
		transientAlloc);

	// Update the globally stored cursor position
	implData.cursorPosition = event.position + windowNode.data.rect.position;
	implData.cursorWindowId = event.windowId;

	if (windowNode.data.topLayout)
	{
		auto& widget = *windowNode.data.topLayout;

		auto modifiedEvent = event;
		modifiedEvent.position = implData.cursorPosition - windowNode.data.rect.position;

		auto const& windowRect = windowNode.data.rect;
		auto const& visibleOffset = windowNode.data.visibleOffset;
		auto const& visibleExtent = windowNode.data.visibleExtent;
		Rect const visibleRect = Rect::Intersection(
			{ {}, windowRect.extent },
			{ { (i32) visibleOffset.x, (i32) visibleOffset.y }, visibleExtent } );
		EventWindowInfo eventWindowInfo {
			.contentScale = windowNode.data.contentScale,
			.dpiX = windowNode.data.dpiX,
			.dpiY = windowNode.data.dpiY, };

		bool cursorOccluded = false;
		if (windowNode.frontmostLayer)
		{
			Layer::CursorMoveParams layerParams {
				.ctx = *this,
				.textManager = textManager,
				.window = eventWindowInfo,
				.rectCollection = rectCollection,
				.event = modifiedEvent,
				.transientAlloc = transientAlloc };
			layerParams.windowRect = windowRect;
			layerParams.safeAreaRect = visibleRect;

			auto& layer = *windowNode.frontmostLayer;
			bool const newOccluded = layer.CursorMove(
				layerParams,
				cursorOccluded);
			cursorOccluded = cursorOccluded || newOccluded;
		}

		Widget::CursorMoveParams widgetParams {
			.ctx = *this,
			.window = eventWindowInfo,
			.rectCollection = rectCollection,
			.textManager = textManager,
			.transientAlloc = transientAlloc,
			//.windowId = windowNode.id,
			.event = modifiedEvent };

		widget.CursorMove(
			widgetParams,
			visibleRect,
			visibleRect,
			cursorOccluded);
	}
}

void Context::PushEvent(WindowContentScaleEvent const& event)
{
	auto& implData = Internal_ImplData();

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.data.contentScale = event.scale;
}

void Context::PushEvent(WindowCursorExitEvent const& event)
{
	auto& implData = Internal_ImplData();

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	if (implData.cursorWindowId.HasValue())
	{
		auto const prevWindowId = implData.cursorWindowId.Value();

		if (event.windowId == prevWindowId)
		{
			implData.cursorWindowId = Std::nullOpt;

			if (windowNode.data.topLayout)
			{
				auto& widget = *windowNode.data.topLayout;
				widget.CursorExit(*this);
			}
		}
	}
}

void Context::PushEvent(WindowFocusEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& windowNodes = implData.windows;

	auto const windowNodeIt = impl::GetWindowNodeIt(windowNodes, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodeIt != windowNodes.end());

	if (event.gainedFocus)
	{
		// Move our window to the front
		auto tempWindowNode = Std::Move(*windowNodeIt);
		windowNodes.erase(windowNodeIt);
		windowNodes.emplace(windowNodes.begin(), Std::Move(tempWindowNode));

		// Call window focus gained event on widget?
	}
}

void Context::PushEvent(WindowMinimizeEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& windowNodes = implData.windows;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.data.isMinimized = event.wasMinimized;
}

void Context::PushEvent(WindowMoveEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& windowNodes = implData.windows;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.data.rect.position = event.position;
}

void Context::PushEvent(WindowResizeEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& windowNodes = implData.windows;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.data.rect.extent = event.extent;
	windowNode.data.visibleOffset = event.safeAreaOffset;
	windowNode.data.visibleExtent = event.safeAreaExtent;
}

void Context::Render2(Render2_Params const& params) const
{
	auto& implData = Internal_ImplData();
	auto& textManager = *implData.textManager;
	auto& rectCollection = params.rectCollection;
	auto& transientAlloc = params.transientAlloc;

	impl::BuildRectCollection(
		*this,
		implData,
		textManager,
		rectCollection,
		true,
		transientAlloc);

	for (auto const& windowNode : implData.windows)
	{
		if (!windowNode.data.topLayout || windowNode.data.isMinimized)
			continue;

		auto const& windowRect = windowNode.data.rect;
		auto const& visibleOffset = windowNode.data.visibleOffset;
		auto const& visibleExtent = windowNode.data.visibleExtent;
		Rect const visibleRect = Rect::Intersection(
			{ {}, windowRect.extent },
			{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent });

		DrawInfo drawInfo = {
			windowNode.data.rect.extent,
			params.vertices,
			params.indices,
			params.drawCmds,
			params.utfValues,
			params.textGlyphRects };

		EventWindowInfo eventWindowInfo {
			.contentScale = windowNode.data.contentScale,
			.dpiX = windowNode.data.dpiX,
			.dpiY = windowNode.data.dpiY, };

		auto& widget = *windowNode.data.topLayout;

		Widget::Render_Params const renderParams {
			.ctx = *this,
			.window = eventWindowInfo,
			.textManager = textManager,
			.rectCollection = rectCollection,
			.framebufferExtent = windowNode.data.rect.extent,
			.transientAlloc = transientAlloc,
			.drawInfo = drawInfo };

		u32 const drawCmdOffset = params.drawCmds.size();

		widget.Render2(
			renderParams,
			visibleRect,
			visibleRect);

		if (windowNode.frontmostLayer)
		{
			auto const& frontLayer = *windowNode.frontmostLayer;

			Layer::Render_Params layerRenderParams {
				.ctx = *this,
				.textManager = textManager,
				.transientAlloc = transientAlloc,
				.window = eventWindowInfo,
				.windowRect = windowRect,
				.safeAreaRect = visibleRect,
				.rectCollection = rectCollection,
				.drawInfo = drawInfo };
			frontLayer.Render(layerRenderParams);
		}

		u32 const drawCmdCount = params.drawCmds.size() - drawCmdOffset;

		Gfx::NativeWindowUpdate newUpdate = {};
		newUpdate.id = (Gfx::NativeWindowID)windowNode.id;
		newUpdate.clearColor = windowNode.data.clearColor;
		newUpdate.drawCmdOffset = drawCmdOffset;
		newUpdate.drawCmdCount = drawCmdCount;

		params.windowUpdates.push_back(newUpdate);
	}
}

void Context::AdoptWindow(AdoptWindowInfo&& windowInfo)
{
	auto& implData = Internal_ImplData();

	impl::WindowNode newNode = {};
	newNode.id = windowInfo.id;

	newNode.data.clearColor = windowInfo.clearColor;
	newNode.data.contentScale = windowInfo.contentScale;
	newNode.data.dpiX = windowInfo.dpiX;
	newNode.data.dpiY = windowInfo.dpiY;
	newNode.data.rect = windowInfo.rect;
	newNode.data.visibleOffset = windowInfo.visibleOffset;
	newNode.data.visibleExtent = windowInfo.visibleExtent;
	newNode.data.topLayout = Std::Move(windowInfo.widget);

	implData.windows.emplace(implData.windows.begin(), Std::Move(newNode));
}

void Context::DestroyWindow(WindowID id)
{
	auto& implData = Internal_ImplData();
	auto& windows = implData.windows;
	auto windowNodeIt = impl::GetWindowNodeIt(windows, id);
	DENGINE_IMPL_GUI_ASSERT(windowNodeIt != windows.end());

	windows.erase(windowNodeIt);
}

void Context::PushPostEventJob_Inner(
	int size,
	int alignment,
	PostEventJob_InvokeFnT invokeFn,
	void const* callablePtr,
	PostEventJob_InitCallableFnT initCallableFn,
	PostEventJob_DestroyFnT destroyFn)
{
	auto& implData = Internal_ImplData();
	Impl::PostEventJob newJob = {};
	newJob.invokeFn = invokeFn;
	newJob.destroyFn = destroyFn;
	newJob.ptr = implData.postEventAlloc.Alloc(size, alignment);
	// Initialize the memory
	initCallableFn(newJob.ptr, callablePtr);

	implData.postEventJobs.emplace_back(newJob);
}