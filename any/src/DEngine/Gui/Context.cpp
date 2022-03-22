#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Defer.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Gui/Layer.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/RectCollection.hpp>
#include <DEngine/Gui/TextManager.hpp>

#include <vector>

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

	void ImplData_PreDispatchStuff(Context::Impl& implData)
	{
		implData.transientAlloc.Reset();
	}

	void ImplData_FlushPostEventJobs(Context& ctx)
	{
		auto& implData = ctx.Internal_ImplData();
		for (auto const& job : implData.postEventJobs)
			job.invokeFn(job.ptr, ctx);
		auto const length = (int)implData.postEventJobs.size();
		for (int i = length; i != 0 ; i -= 1)
			implData.postEventAlloc.Free(implData.postEventJobs[i - 1].ptr);

		implData.postEventJobs.clear();
		implData.postEventAlloc.Reset();
	}

	// All rects should be relative to window position, not visible rect
	void BuildRectCollection(
		Context const& ctx,
		TextManager& textManager,
		RectCollection& rectCollection,
		bool includeRendering,
		Std::FrameAlloc& transientAlloc)
	{
		auto& implData = ctx.Internal_ImplData();

		rectCollection.Prepare(includeRendering);

		for (auto& windowNode : implData.windows)
		{
			if (!windowNode.data.topLayout)
				continue;

			auto const& windowRect = windowNode.data.rect;
			auto const& visibleOffset = windowNode.data.visibleOffset;
			auto const& visibleExtent = windowNode.data.visibleExtent;
			Rect const visibleRect = Rect::Intersection(
				{ {}, windowRect.extent },
				{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent });

			RectCollection::SizeHintPusher sizeHintPusher { rectCollection };

			if (windowNode.frontmostLayer)
			{
				Layer::BuildSizeHints_Params params = {
					.ctx = ctx,
					.textManager = textManager,
					.transientAlloc = transientAlloc,
					.pusher = sizeHintPusher };
				params.windowRect = visibleRect;
				params.safeAreaRect = visibleRect;

				auto const& layer = *windowNode.frontmostLayer;
				layer.BuildSizeHints(params);
			}

			Widget::GetSizeHint2_Params params = {
				.ctx = ctx,
				.textManager = textManager,
				.transientAlloc = transientAlloc,
				.pusher = sizeHintPusher, };

			auto const& widget = *windowNode.data.topLayout;
			widget.GetSizeHint2(params);

			transientAlloc.Reset();
		}


		// Then build rects
		for (auto& windowNode : implData.windows)
		{
			if (!windowNode.data.topLayout)
				continue;

			auto const& windowRect = windowNode.data.rect;
			auto const& visibleOffset = windowNode.data.visibleOffset;
			auto const& visibleExtent = windowNode.data.visibleExtent;
			Rect const visibleRect = Rect::Intersection(
				{ {}, windowRect.extent },
				{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent});

			RectCollection::RectPusher rectPusher {rectCollection };

			if (windowNode.frontmostLayer)
			{
				auto& layer = *windowNode.frontmostLayer;
				Layer::BuildRects_Params buildRectsParams {
					.ctx = ctx,
					.textManager = textManager,
					.transientAlloc = transientAlloc,
					.pusher = rectPusher };
				buildRectsParams.windowRect = visibleRect;
				buildRectsParams.visibleRect = visibleRect;

				layer.BuildRects(buildRectsParams);
			}

			//Widget::BuildChildRects_Params params{ *this, sizeHintPusher2, transientAlloc };
			Widget::BuildChildRects_Params params {
				.ctx = ctx,
				.textManager = textManager,
				.transientAlloc = transientAlloc,
				.pusher = rectPusher, };

			auto& widget = *windowNode.data.topLayout;
			params.pusher.Push(widget, { visibleRect, visibleRect });
			widget.BuildChildRects(
				params,
				visibleRect,
				visibleRect);

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
		implData.inputConnectionWidget->InputConnectionLost();
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

void Context::PushEvent(CharEnterEvent const& event)
{
	auto& implData = Internal_ImplData();

	for (auto& windowNode : implData.windows)
	{
		if (!windowNode.data.topLayout)
			continue;

		windowNode.data.topLayout->CharEnterEvent(*this);
	}
}

void Context::PushEvent(CharEvent const& event)
{
	auto& implData = Internal_ImplData();
	for (auto& windowNode : implData.windows)
	{
		if (!windowNode.data.topLayout)
			continue;

		windowNode.data.topLayout->CharEvent(
			*this,
			event.utfValue);
	}
}

void Context::PushEvent(CharRemoveEvent const& event)
{
	auto& implData = Internal_ImplData();
	auto& transientAlloc = implData.transientAlloc;
	Std::Defer _allocCleanup = [&]() { transientAlloc.Reset(); };

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	if (windowNode.data.topLayout)
	{
		//windowNode.data.topLayout->CharRemoveEvent(*this, transientAlloc);
	}
}

void Context::PushEvent(TextInputEvent const& event)
{
	auto& implData = Internal_ImplData();

	auto& transientAlloc = implData.transientAlloc;
	Std::Defer _allocCleanup = [&]() { transientAlloc.Reset(); };

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
	Std::Defer _allocCleanup = [&]() { transientAlloc.Reset(); };

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	if (windowNode.data.topLayout)
	{
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
		textManager,
		rectCollection,
		false,
		transientAlloc);

	if (windowNode.data.topLayout)
	{
		auto const localCursorPos = implData.cursorPosition - windowNode.data.rect.position;

		Rect const& windowRect = { {}, windowNode.data.rect.extent };
		auto const& visibleOffset = windowNode.data.visibleOffset;
		auto const& visibleExtent = windowNode.data.visibleExtent;
		Rect const visibleRect = Rect::Intersection(
			windowRect,
			{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent });

		bool eventConsumed = false;
		Layer::Press_Return pressReturn = {};
		if (windowNode.frontmostLayer)
		{
			Layer::CursorPressParams layerParams {
				.ctx = *this,
				.textManager = textManager,
				.rectCollection = rectCollection,
				.event = event, };
			layerParams.windowRect = windowRect;
			layerParams.safeAreaRect = visibleRect;
			layerParams.cursorPos = localCursorPos;

			auto& layer = *windowNode.frontmostLayer;
			pressReturn = layer.CursorPress(layerParams, eventConsumed);
			eventConsumed = eventConsumed || pressReturn.eventConsumed;
		}
		if (pressReturn.destroyLayer)
		{
			windowNode.frontmostLayer = {};
		}

		Widget::CursorPressParams widgetParams {
			.ctx = *this,
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
			{{}, windowRect.extent},
			{{(i32) visibleOffset.x, (i32) visibleOffset.y}, visibleExtent});

		bool cursorOccluded = false;
		if (windowNode.frontmostLayer)
		{
			Layer::CursorMoveParams layerParams {
				.ctx = *this,
				.textManager = textManager,
				.rectCollection = rectCollection,
				.event = modifiedEvent };
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
			.rectCollection = rectCollection,
			.textManager = textManager,
			.transientAlloc = transientAlloc,
			.windowId = windowNode.id,
			.event = modifiedEvent };

		widget.CursorMove(
			widgetParams,
			visibleRect,
			visibleRect,
			cursorOccluded);
	}
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
		textManager,
		rectCollection,
		true,
		transientAlloc);

	for (auto const& windowNode : implData.windows)
	{
		if (windowNode.data.isMinimized || !windowNode.data.topLayout)
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
			params.drawCmds };

		auto& widget = *windowNode.data.topLayout;

		Widget::Render_Params const renderParams {
			.ctx = *this,
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

void Context::AdoptWindow(
	WindowID id,
	Math::Vec4 clearColor,
	Rect rect,
	Math::Vec2UInt visibleOffset,
	Extent visibleExtent,
	Std::Box<Widget>&& widget)
{
	auto& implData = Internal_ImplData();

	impl::WindowNode newNode = {};
	newNode.id = id;
	newNode.data.rect = rect;
	newNode.data.visibleOffset = visibleOffset;
	newNode.data.visibleExtent = visibleExtent;
	newNode.data.clearColor = clearColor;
	newNode.data.topLayout = Std::Move(widget);

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
	PostEventJob_InitCallableFnT initCallableFn)
{
	auto& implData = Internal_ImplData();
	Impl::PostEventJob newJob = {};
	newJob.invokeFn = invokeFn;
	newJob.ptr = implData.postEventAlloc.Alloc(size, alignment);
	// Initialize the memory
	initCallableFn(newJob.ptr, callablePtr);

	implData.postEventJobs.emplace_back(newJob);
}

/*
void Context::PushEvent(TouchMoveEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		auto modifiedEvent = event;
		modifiedEvent.position -= Math::Vec2{
			(f32)windowNode.data.rect.position.x,
			(f32)windowNode.data.rect.position.y };

		auto const& rect = windowNode.data.rect;
		Math::Vec2Int visiblePos = {
			rect.position.x + (i32)windowNode.data.visibleOffset.x,
			rect.position.y + (i32)windowNode.data.visibleOffset.y };
		Rect const visibleRect = { visiblePos, windowNode.data.visibleExtent };

		bool touchOccluded = false;

		if (windowNode.frontmostLayer)
		{
			auto& layer = *windowNode.frontmostLayer;
			bool const temp = layer.TouchMove(
				*this,
				visibleRect,
				visibleRect,
				modifiedEvent,
				touchOccluded);

			touchOccluded = temp;
		}

		if (!windowNode.data.topLayout)
			continue;
		
		windowNode.data.topLayout->TouchMoveEvent(
			*this,
			windowNode.id,
			visibleRect,
			visibleRect,
			event,
			touchOccluded);
	}
}
*/

/*
void Context::PushEvent(TouchPressEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);

	for (auto& windowNode : implData.windows)
	{
		auto modifiedEvent = event;
		modifiedEvent.position -= Math::Vec2{
			(f32)windowNode.data.rect.position.x,
			(f32)windowNode.data.rect.position.y };

		auto const& visibleOffset = windowNode.data.visibleOffset;
		Math::Vec2Int const visiblePos =
			windowNode.data.rect.position +
			Math::Vec2Int{ (i32)visibleOffset.x, (i32)visibleOffset.y };

		Rect const windowRect = { {}, windowNode.data.rect.extent };
		auto visibleRect = windowNode.data.visibleRect;
		visibleRect.position -= windowNode.data.rect.position;

		bool eventConsumed = false;
		bool destroyMenu = false;

		if (windowNode.frontmostLayer)
		{
			auto& layer = *windowNode.frontmostLayer;
			auto const temp = layer.TouchPress(
				*this,
				windowRect,
				visibleRect,
				modifiedEvent);

			eventConsumed = temp.eventConsumed;
			destroyMenu = temp.destroy;
		}
		if (destroyMenu)
			windowNode.frontmostLayer = nullptr;

		if (!windowNode.data.topLayout || (eventConsumed && modifiedEvent.pressed))
			continue;

		windowNode.data.topLayout->TouchPressEvent(
			*this,
			windowNode.id,
			visibleRect,
			visibleRect,
			modifiedEvent);
	}
}

void Context::PushEvent(WindowCloseEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	WindowID windowId = windowIt->id;
	implData.windows.erase(windowIt);
	implData.windowHandler->CloseWindow(windowId);
}

void Context::PushEvent(WindowCursorEnterEvent const& event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
}

void Context::PushEvent(WindowMinimizeEvent const& event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
	windowData.isMinimized = event.wasMinimized;
}

void Context::PushEvent(WindowMoveEvent const& event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
	windowData.rect.position = event.position;
	windowData.visibleRect.position = windowData.rect.position;
}

void Context::PushEvent(WindowResizeEvent const& event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
	windowData.rect.extent = event.extent;
	windowData.visibleRect = event.visibleRect;
}

void Context::Render(
	std::vector<Gfx::GuiVertex>& vertices,
	std::vector<u32>& indices,
	std::vector<Gfx::GuiDrawCmd>& drawCmds,
	std::vector<Gfx::NativeWindowUpdate>& windowUpdates) const
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);

	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.isMinimized)
			continue;

		windowNode.data.drawCmdOffset = (u32)drawCmds.size();

		Rect const windowRect = { {}, windowNode.data.rect.extent };
		auto visibleRect = windowNode.data.visibleRect;
		visibleRect.position -= windowNode.data.rect.position;

		DrawInfo drawInfo = {
			windowNode.data.rect.extent,
			vertices,
			indices,
			drawCmds };

		if (windowNode.data.topLayout)
		{
			windowNode.data.topLayout->Render(
				*this,
				windowNode.data.rect.extent,
				visibleRect,
				visibleRect,
				drawInfo);
		}

		if (windowNode.frontmostLayer)
		{
			auto const& layer = *windowNode.frontmostLayer;
			layer.Render(
				*this,
				windowRect,
				visibleRect,
				drawInfo);
		}

		windowNode.data.drawCmdCount = (u32)drawCmds.size() - windowNode.data.drawCmdOffset;

		Gfx::NativeWindowUpdate newUpdate = {};
		newUpdate.id = Gfx::NativeWindowID(windowNode.id);
		newUpdate.clearColor = windowNode.data.clearColor;
		newUpdate.drawCmdOffset = windowNode.data.drawCmdOffset;
		newUpdate.drawCmdCount = windowNode.data.drawCmdCount;
		auto windowEvents = App::GetWindowEvents(App::WindowID(windowNode.id));
		if (windowEvents.restore)
			newUpdate.event = Gfx::NativeWindowEvent::Restore;
		else if (windowEvents.resize)
			newUpdate.event = Gfx::NativeWindowEvent::Resize;

		windowUpdates.push_back(newUpdate);
	}
}
*/