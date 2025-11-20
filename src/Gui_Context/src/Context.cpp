#include <DEngine/Gui/Context/Context.hpp>
#include "ImplData.hpp"

#include <DEngine/Std/Containers/Defer.hpp>

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/RectCollection.hpp>

#include "DEngine/Gui/DebugLog.hpp"

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl {

	// This object should never be alive for longer than exactly one event.
	// It allows us to forward events to the windowing system.
	class EventWindowHandlerImpl : public Widget::WidgetEvent_WindowHandler {
	public:
		Context* m_ctx = nullptr;
		WindowID m_windowId = WindowID::Invalid;
		WindowNode* m_windowNode = nullptr;
		// This can be null if we're processing the base UI plane.
		bool* m_removeCurrentLayer = nullptr;

		[[nodiscard]] static EventWindowHandlerImpl Create(Context& ctx, WindowNode& windowNode) {
			DENGINE_IMPL_GUI_ASSERT(windowNode.id != WindowID::Invalid);
			EventWindowHandlerImpl out = {};
			out.m_ctx = &ctx;
			out.m_windowId = windowNode.id;
			out.m_windowNode = &windowNode;
			return out;
		}

		[[nodiscard]] virtual bool SetFrontmostLayer(
			Std::Box<Widget>&& rootWidget,
			Math::Vec2Int relativePos,
			Extent extent,
			Std::Opt<Widget::Widget_FocusWidgetResult> const& focusItem) override
		{
			// TODO: If there is already an existing frontmost layer,
			// we need to remove this.
			DENGINE_IMPL_GUI_ASSERT(!this->m_windowNode->data.frontmostLayer.Has());

			// TODO: Does this need to be queued?
			DENGINE_IMPL_GUI_ASSERT(this->m_windowNode != nullptr);
			DENGINE_IMPL_GUI_ASSERT(rootWidget.Has());

			WindowData::FrontmostLayer newFrontmostLayer = {
				.extent = extent,
				.focusWidget = Std::nullOpt,
				.relativePosition = relativePos,
				.rootWidget = Std::Move(rootWidget)
			};

			if (focusItem.Has()) {
				newFrontmostLayer.focusWidget = WindowData::FocusWidget {
					.secondaryIndex = focusItem.Get().secondaryIndex,
					.widget = focusItem.Get().widget,
				};
			}

			this->m_windowNode->data.frontmostLayer = Std::Move(newFrontmostLayer);
			return true;
		}

		virtual void QueueRemoveCurrentLayer() override {
			DENGINE_IMPL_GUI_ASSERT(this->m_removeCurrentLayer != nullptr);
			DENGINE_IMPL_GUI_ASSERT(
				this->m_windowNode != nullptr
				&& this->m_windowNode->data.frontmostLayer.Has());
			*this->m_removeCurrentLayer = true;
		}

		[[nodiscard]] virtual Std::Box<Widget::Widget_TextInputSession> TakeTextInputConnection(
			Widget& widget,
			TextInputType textInputFilter,
			Std::Span<char const> currentText,
			u64 selStart,
			u64 selCount) override
		{
			DENGINE_IMPL_GUI_ASSERT(this->m_windowId != WindowID::Invalid);
			this->m_ctx->GetWindowHandler().OpenSoftInput(
				this->m_windowId,
				currentText,
				selStart,
				selCount,
				textInputFilter);

			// TODO: We probably need a pointer from ctx to this object?
			auto* newSession = new WidgetTextInputSessionImpl;
			newSession->m_ctx = this->m_ctx;

			auto& implData = this->m_ctx->Internal_ImplData();
			DENGINE_IMPL_GUI_ASSERT(!implData.activeTextInputSession.Has());
			implData.activeTextInputSession = ActiveTextInputSession {
				.session = newSession,
				.widget = &widget };

			return Std::BoxAdopt(newSession);
		}
	};

	struct DeferredJobQueueImpl : public Widget::WidgetEvent_DeferredJobQueueBackend {
		Context* m_ctx = nullptr;

		[[nodiscard]] static DeferredJobQueueImpl Create(Context& ctx) {
			DeferredJobQueueImpl out = {};
			out.m_ctx = &ctx;
			return out;
		}

		virtual void PushPostEventJob(
			int size,
			int alignment,
			InvokeFnT invokeFn,
			void* callablePtr,
			MoveFnT moveFn,
			DestroyFnT destroyFn) override
		{
			DENGINE_IMPL_GUI_ASSERT(this->m_ctx != nullptr);

			auto& implData = this->m_ctx->Internal_ImplData();
			Context::Impl::PostEventJob newJob = {};
			newJob.invokeFn = invokeFn;
			newJob.destroyFn = destroyFn;
			newJob.ptr = implData.postEventAlloc.Alloc(size, alignment);
			// Initialize the memory
			moveFn(newJob.ptr, callablePtr);

			implData.postEventJobs.emplace_back(newJob);
		}
	};
}

Gui::impl::WidgetTextInputSessionImpl::~WidgetTextInputSessionImpl() {
	// TODO: Should this be queued to happen after event dispatching?
	// If we do it after event dispatching, we can determine
	// whether another Widget is trying to steal the text input session.
	// In which case, we can instead make it switch widgets without
	// even actually closing the input session. We just update the
	// metadata of our input connection.
	DENGINE_IMPL_GUI_ASSERT(this->m_ctx != nullptr);
	this->m_ctx->GetWindowHandler().HideSoftInput();

	auto& implData = this->m_ctx->Internal_ImplData();
	implData.activeTextInputSession = Std::nullOpt;
}

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
	[[nodiscard]] static auto GetWindowNodeIt(decltype(Context::Impl::windows)& windows, WindowID id) {
		auto windowIt = Std::FindIf(
			windows.begin(),
			windows.end(),
			[&id](auto const& item) {
				return item.id == id;
			});
		return windowIt;
	}

	[[nodiscard]] static auto GetWindowNodeIt(decltype(Context::Impl::windows) const& windows, WindowID id) {
		auto windowIt = Std::FindIf(
			windows.begin(),
			windows.end(),
			[&id](auto const& item) {
				return item.id == id;
			});
		return windowIt;
	}

	[[nodiscard]] static WindowNode* GetWindowNodePtr(Context::Impl& implData, WindowID id) {
		auto& windows = implData.windows;
		auto windowIt = GetWindowNodeIt(windows, id);
		if (windowIt != windows.end()) {
			return &*windowIt;
		}
		return nullptr;
	}

	[[nodiscard]] static WindowNode const* GetWindowNodePtr(Context::Impl const& implData, WindowID id) {
		auto const& windows = implData.windows;
		auto windowIt = GetWindowNodeIt(windows, id);
		if (windowIt != windows.end()) {
			return &*windowIt;
		}
		return nullptr;
	}

	[[nodiscard]] static EventWindowInfo ToEventWindowInfo(
		WindowNode const& node,
		Context::WindowModifiers const& modifiers)
	{
		DENGINE_IMPL_GUI_ASSERT(modifiers.contentScaleMultiplier > 0.01);
		DENGINE_IMPL_GUI_ASSERT(modifiers.fontScaleMultiplier > 0.01);
		 return EventWindowInfo {
		 	.contentScale = node.data.contentScale * modifiers.contentScaleMultiplier,
		 	.dpi = node.data.dpi,
		 	.fontScale = node.data.fontScale * modifiers.fontScaleMultiplier,
		 	.minimumHeightCm = modifiers.minimumHeightCm,
			.touchScrollSlopPx = node.data.touchScrollSlopPx,
			.scrollBarWidthPx = node.data.scrollBarWidthPx };
	}

	[[nodiscard]] static Rect ApplyWindowInsetsToRect(
		Rect const& rect,
		WindowInsetValues const& windowInsets)
	{
		// Some error handling.
		// TODO: Having these values in the first place should be considered faulty state. Probably.
		if (windowInsets.left + windowInsets.right >= rect.extent.width) {
			return {};
		}
		if (windowInsets.top + windowInsets.bottom >= rect.extent.height) {
			return {};
		}

		return Rect {
			.position = {
				.x = (i32)((i64)rect.position.x + (i64)windowInsets.left),
				.y = (i32)((i64)rect.position.y + (i64)windowInsets.top) },
			.extent = {
				.width = rect.extent.width - windowInsets.left - windowInsets.right,
				.height = rect.extent.height - windowInsets.top - windowInsets.bottom, }, };
	}

	void ImplData_PreDispatchStuff(Context::Impl& implData) {
		implData.transientAlloc.Reset();
	}

	void ImplData_FlushPostEventJobs(Context& ctx, Std::AnyRef customData) {
		auto& implData = ctx.Internal_ImplData();
		for (auto const& job : implData.postEventJobs) {
			job.invokeFn(job.ptr, customData);
		}
		auto const length = (int)implData.postEventJobs.size();
		for (int i = length; i != 0 ; i -= 1) {
			auto& job = implData.postEventJobs[i - 1];
			job.destroyFn(job.ptr);
			implData.postEventAlloc.Free(job.ptr, job.allocSize);
		}

		implData.postEventJobs.clear();
		implData.postEventAlloc.Reset();
	}

	[[nodiscard]] constexpr Rect TopLevelLocalVisibleRect() {
		return { {-100000, -100000 }, {200000, 200000} };
	}

	// Calculate how to offset our UI tree such that the text-editing widget is moved into
	// view
	[[nodiscard]] Math::Vec2Int CalcTreeOffset(
		Context::Impl const& implData,
		RectCollection const& rectColl,
		Extent const& windowExtent,
		Math::Vec2Int visibleAreaOffset,
		Extent const& visibleAreaExtent)
	{
		if (!implData.activeTextInputSession.Has()) {
			return {};
		}
		auto const& textInputSession = implData.activeTextInputSession.Get();
		DENGINE_IMPL_GUI_ASSERT(textInputSession.widget != nullptr);

		auto rectPairOpt = rectColl.GetLocalRect(*textInputSession.widget);
		DENGINE_IMPL_GUI_ASSERT(rectPairOpt.Has());
		auto const& widgetRect = rectPairOpt.Value().widgetRect;
		Math::Vec2Int returnVal = {};

		// Minimize the offset such that the Widget does not end up under the touchscreen keyboard?
		returnVal.y = Std::Min(
			0,
			-widgetRect.position.y + (i32)visibleAreaExtent.height - (i32)widgetRect.extent.height);

		// TODO: We've implemented such that the text-editing Widget is pushed up into the view
		// from the bottom, but we probably want to fix this for pushing it down as well, and
		// to the side.

		return returnVal;
	}

	[[nodiscard]] static Rect CalcFrontmostLayerRect(
		WindowData::FrontmostLayer const& layer,
		Extent const& windowExtent)
	{
		i64 const maxWidth = (i64)windowExtent.width - (i64)layer.relativePosition.x;
		i64 const maxHeight = (i64)windowExtent.height - (i64)layer.relativePosition.y;
		if (maxWidth <= 0 || maxHeight <= 0) {
			return { layer.relativePosition, {} };
		}

		Rect out = { layer.relativePosition, layer.extent };
		if ((i64)out.extent.width > maxWidth)
			out.extent.width = (u32)maxWidth;
		if ((i64)out.extent.height > maxHeight)
			out.extent.height = (u32)maxHeight;
		return out;
	}

	void BuildRectCollection(
		TextEngine& textManager,
		Std::ConstAnyRef appData,
		RectCollection& rectCollection,
		bool includeRendering,
		WindowNode const& windowNode,
		EventWindowInfo const& eventWindowInfo,
		Std::FrameAlloc& transientAlloc)
	{
		rectCollection.Prepare(includeRendering);

		if (!windowNode.data.topLayout || windowNode.data.isMinimized) {
			return;
		}

		RectCollection::SizeHintPusher sizeHintPusher { rectCollection };

		Widget::GetSizeHint2_Params sizeHintWidgetParams = {
			.window = eventWindowInfo,
			.textEngine = textManager,
			.appData = appData,
			.transientAlloc = transientAlloc,
			.pusher = sizeHintPusher, };
		auto const& widget = *windowNode.data.topLayout;
		[[maybe_unused]] auto _ = widget.GetSizeHint2(sizeHintWidgetParams);

		if (windowNode.data.frontmostLayer.Has()) {
			auto const& layerWidget = *windowNode.data.frontmostLayer.Get().rootWidget;
			[[maybe_unused]] auto _layer = layerWidget.GetSizeHint2(sizeHintWidgetParams);
		}

		transientAlloc.Reset();

		auto const& windowExtent = windowNode.data.rect.extent;
		Rect const topLevelWidgetRect = ApplyWindowInsetsToRect(
			{ {}, windowExtent },
			windowNode.data.insets.GetSourceInsets(WindowInsetSource::SystemBar));
		auto localVisibleRect = TopLevelLocalVisibleRect();

		RectCollection::RectPusher rectPusher { rectCollection };

		Widget::BuildChildRects_Params widgetParams {
			.window = eventWindowInfo,
			.textEngine = textManager,
			.appData = appData,
			.transientAlloc = transientAlloc,
			.pusher = rectPusher, };

		auto childEntry = rectPusher.GetEntry(widget);
		rectPusher.SetLocalRectPair(childEntry, { topLevelWidgetRect, localVisibleRect });
		widget.BuildChildRects(
			widgetParams,
			topLevelWidgetRect,
			localVisibleRect,
			Std::nullOpt);

		if (windowNode.data.frontmostLayer.Has()) {
			auto const& layer = windowNode.data.frontmostLayer.Get();
			auto const& layerWidget = *layer.rootWidget;
			Rect const layerRect = CalcFrontmostLayerRect(layer, windowExtent);
			auto const layerVisibleRect = TopLevelLocalVisibleRect();
			auto layerEntry = rectPusher.GetEntry(layerWidget);
			rectPusher.SetLocalRectPair(layerEntry, { layerRect, layerVisibleRect });
			layerWidget.BuildChildRects(
				widgetParams,
				layerRect,
				layerVisibleRect,
				Std::nullOpt);
		}

		transientAlloc.Reset();
	}
}

Context Context::Create(WindowHandler& windowHandler)
{
	Context newCtx;
	newCtx.pImplData = new Impl;
	auto& implData = *newCtx.pImplData;
	
	implData.windowHandler = &windowHandler;

	return static_cast<Context&&>(newCtx);
}

Context::Context(Context&& other) noexcept :
	pImplData(other.pImplData)
{
	other.pImplData = nullptr;
}

Context::WindowHandler& Context::GetWindowHandler() const
{
	auto const& implData = Internal_ImplData();
	return *implData.windowHandler;
}

void Context::PushEvent(TextInputEvent const& event) {
	auto& implData = Internal_ImplData();

	auto& transientAlloc = implData.transientAlloc;
	Std::Defer _allocCleanup { [&]{ transientAlloc.Reset(); } };

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	if (windowNode.data.topLayout) {
		Widget::WidgetEvent_TextInputParams params = {
			.start = event.start,
			.count = event.count,
			.newText = event.newText,
			.newSelStart = event.newSelStart,
			.newSelCount = event.newSelCount, };

		windowNode.data.topLayout->WidgetEvent_TextInput(
			transientAlloc,
			params);
	}
}

void Context::PushEvent(TextSelectionEvent const& event) {
	auto& implData = Internal_ImplData();

	auto& transientAlloc = implData.transientAlloc;
	Std::Defer _allocCleanup { [&]{ transientAlloc.Reset(); } };

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	if (windowNode.data.topLayout) {
		Widget::WidgetEvent_TextSelectionParams params = {
			.start = event.start,
			.count = event.count, };

		windowNode.data.topLayout->WidgetEvent_TextSelection(
			transientAlloc,
			params);
	}
}

void Context::PushEvent(EndTextInputSessionEvent const& event) {
	auto& implData = Internal_ImplData();

	auto& transientAlloc = implData.transientAlloc;
	Std::Defer _allocCleanup { [&]{ transientAlloc.Reset(); } };

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	if (windowNode.data.topLayout) {
		Widget::WidgetEvent_EndTextInputSessionParams params = {};
		windowNode.data.topLayout->WidgetEvent_EndTextInputSession(
			transientAlloc,
			params);
	}
}

void Context::PushEvent(
	CursorPressEvent const& event,
	TextEngine& textEngine,
	Std::AnyRef appData)
{
	auto& implData = Internal_ImplData();
	auto& rectCollection = implData.rectCollection;
	auto& transientAlloc = implData.transientAlloc;

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;
	implData.cursorWindowId = event.windowId;

	impl::ImplData_PreDispatchStuff(implData);

	if (windowNode.data.topLayout) {
		auto const eventWindowInfo = ToEventWindowInfo(windowNode, this->windowModifiers);

		impl::BuildRectCollection(
			textEngine,
			appData.ToConst(),
			rectCollection,
			false,
			windowNode,
			eventWindowInfo,
			transientAlloc);

		auto const localCursorPos =
			implData.cursorPosition - windowNode.data.rect.position;

		auto const& windowRect = windowNode.data.rect;
		Rect const textSystemVisibleRect = impl::ApplyWindowInsetsToRect(
			windowRect,
			windowNode.data.insets.GetSourceInsets(WindowInsetSource::TextInputSystem));
		Math::Vec2Int treeOffset = impl::CalcTreeOffset(
			implData,
			rectCollection,
			windowRect.extent,
			{ (i32)textSystemVisibleRect.position.x, (i32)textSystemVisibleRect.position.y },
			textSystemVisibleRect.extent);
		rectCollection.SetTreeOffsetAndWindowExtent(treeOffset, windowRect.extent);

		bool eventConsumed = false;
		bool removeFrontmostLayer = false;

		auto widgetWindowHandler = impl::EventWindowHandlerImpl::Create(*this, windowNode);
		widgetWindowHandler.m_removeCurrentLayer = &removeFrontmostLayer;
		auto deferredJobQueueBackend = impl::DeferredJobQueueImpl::Create(*this);
		auto deferredJobQueue = Widget::WidgetEvent_DeferredJobQueue{ deferredJobQueueBackend };

		Widget::CursorPressParams widgetParams = {
			.cursorButton = event.button,
			.cursorPos = localCursorPos,
			.cursorPressed = event.pressed,
			.customData = appData,
			.jobQueue = deferredJobQueue,
			.rectCollection = rectCollection,
			.textEngine = textEngine,
			.transientAlloc =  transientAlloc,
			.window = eventWindowInfo,
			.windowHandler = widgetWindowHandler, };

		// Frontmost layer gets first refusal; the base tree is then told whether the layer
		// already consumed the press. A dropdown/menu uses this to close on an outside press.
		if (windowNode.data.frontmostLayer.Has()) {
			eventConsumed = windowNode.data.frontmostLayer.Get().rootWidget->CursorPress2(
				widgetParams,
				Std::nullOpt,
				eventConsumed);
		}

		auto& widget = *windowNode.data.topLayout;

		widget.CursorPress2(
			widgetParams,
			{},
			eventConsumed);

		if (removeFrontmostLayer) {
			windowNode.data.frontmostLayer = Std::nullOpt;
		}
	}

	impl::ImplData_FlushPostEventJobs(*this, appData);
}

void Context::PushEvent(
	TouchMoveEvent const& event,
	TextEngine& textEngine,
	Std::AnyRef appData)
{
	// We can validate whether this touch-id is currently reported as being held down.

	auto& implData = Internal_ImplData();
	auto& rectCollection = implData.rectCollection;
	auto& transientAlloc = implData.transientAlloc;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	if (windowNode.data.topLayout) {
		auto eventWindowInfo = ToEventWindowInfo(windowNode, this->windowModifiers);

		impl::ImplData_PreDispatchStuff(implData);
		impl::BuildRectCollection(
			textEngine,
			appData.ToConst(),
			rectCollection,
			false,
			windowNode,
			eventWindowInfo,
			transientAlloc);

		auto& widget = *windowNode.data.topLayout;

		auto modifiedEvent = event;

		auto const& windowRect = windowNode.data.rect;

		Rect const textSystemVisibleRect = impl::ApplyWindowInsetsToRect(
			windowRect,
			windowNode.data.insets.GetSourceInsets(WindowInsetSource::TextInputSystem));
		Math::Vec2Int treeOffset = impl::CalcTreeOffset(
			implData,
			rectCollection,
			windowRect.extent,
			{ (i32)textSystemVisibleRect.position.x, (i32)textSystemVisibleRect.position.y },
			textSystemVisibleRect.extent);
		rectCollection.SetTreeOffsetAndWindowExtent(treeOffset, windowRect.extent);

		bool cursorOccluded = false;

		Widget::WidgetEvent_TouchMoveParams widgetParams {
			.customData = appData,
			.event = {
				.id = modifiedEvent.id,
				.position = modifiedEvent.position, },
			.rectCollection = rectCollection,
			.textEngine = textEngine,
			.transientAlloc = transientAlloc,
			.window = eventWindowInfo, };

		// Frontmost layer occludes the base tree underneath it.
		if (windowNode.data.frontmostLayer.Has()) {
			auto layerResult = windowNode.data.frontmostLayer.Get().rootWidget->WidgetEvent_TouchMove(
				widgetParams,
				Std::nullOpt,
				cursorOccluded);
			cursorOccluded = layerResult.consumed;
		}

		widget.WidgetEvent_TouchMove(
			widgetParams,
			{},
			cursorOccluded);
	}

	impl::ImplData_FlushPostEventJobs(*this, appData);
}

void Context::PushEvent(
	TouchPressEvent const& event,
	TextEngine& textEngine,
	Std::AnyRef appData)
{
	auto& implData = Internal_ImplData();
	auto& rectCollection = implData.rectCollection;
	auto& transientAlloc = implData.transientAlloc;

	auto* windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	if (windowNode.data.topLayout) {
		auto eventWindowInfo = ToEventWindowInfo(windowNode, this->windowModifiers);

		impl::ImplData_PreDispatchStuff(implData);
		impl::BuildRectCollection(
			textEngine,
			appData.ToConst(),
			rectCollection,
			false,
			windowNode,
			eventWindowInfo,
			transientAlloc);

		auto const& windowRect = windowNode.data.rect;
		Rect const textSystemVisibleRect = impl::ApplyWindowInsetsToRect(
			windowRect,
			windowNode.data.insets.GetSourceInsets(WindowInsetSource::TextInputSystem));
		Math::Vec2Int treeOffset = impl::CalcTreeOffset(
			implData,
			rectCollection,
			windowRect.extent,
			{ (i32)textSystemVisibleRect.position.x, (i32)textSystemVisibleRect.position.y },
			textSystemVisibleRect.extent);
		rectCollection.SetTreeOffsetAndWindowExtent(treeOffset, windowRect.extent);

		bool eventConsumed = false;
		bool removeFrontmostLayer = false;
		auto widgetWindowHandler = impl::EventWindowHandlerImpl::Create(*this, windowNode);
		widgetWindowHandler.m_removeCurrentLayer = &removeFrontmostLayer;
		auto deferredJobQueueBackend = impl::DeferredJobQueueImpl::Create(*this);
		auto deferredJobQueue = Widget::WidgetEvent_DeferredJobQueue{ deferredJobQueueBackend };

		Widget::WidgetEvent_TouchPressEventData eventData = {
			.id = event.id,
			.position = event.position,
			.pressed = event.pressed, };
		Widget::WidgetEvent_TouchPressParams widgetParams = {
			.customData = appData,
			.event = eventData,
			.jobQueue = deferredJobQueue,
			.rectCollection = rectCollection,
			.textEngine = textEngine,
			.transientAlloc =  transientAlloc,
			.window = eventWindowInfo,
			.windowHandler = widgetWindowHandler, };

		// Frontmost layer gets first refusal (see the cursor-press path for the rationale).
		if (windowNode.data.frontmostLayer.Has()) {
			auto layerResult = windowNode.data.frontmostLayer.Get().rootWidget->WidgetEvent_TouchPress(
				widgetParams,
				Std::nullOpt,
				eventConsumed);
			eventConsumed = layerResult.consumed;
		}

		auto& widget = *windowNode.data.topLayout;

		widget.WidgetEvent_TouchPress(
			widgetParams,
			{},
			eventConsumed);

		if (removeFrontmostLayer) {
			windowNode.data.frontmostLayer = Std::nullOpt;
		}
	}

	impl::ImplData_FlushPostEventJobs(*this, appData);
}

void Context::PushEvent(
	CursorMoveEvent const& event,
	TextEngine& textEngine,
	Std::AnyRef appData)
{
	auto& implData = Internal_ImplData();
	auto& rectCollection = implData.rectCollection;
	auto& transientAlloc = implData.transientAlloc;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	// Update the globally stored cursor position
	implData.cursorPosition = event.position + windowNode.data.rect.position;
	implData.cursorWindowId = event.windowId;

	if (windowNode.data.topLayout) {
		auto eventWindowInfo = ToEventWindowInfo(windowNode, this->windowModifiers);

		impl::ImplData_PreDispatchStuff(implData);
		impl::BuildRectCollection(
			textEngine,
			appData.ToConst(),
			rectCollection,
			false,
			windowNode,
			eventWindowInfo,
			transientAlloc);

		auto& widget = *windowNode.data.topLayout;

		auto modifiedEvent = event;
		modifiedEvent.position = implData.cursorPosition - windowNode.data.rect.position;

		auto const& windowRect = windowNode.data.rect;
		Rect const textSystemVisibleRect = impl::ApplyWindowInsetsToRect(
			windowRect,
			windowNode.data.insets.GetSourceInsets(WindowInsetSource::TextInputSystem));
		Math::Vec2Int treeOffset = impl::CalcTreeOffset(
			implData,
			rectCollection,
			windowRect.extent,
			{ (i32)textSystemVisibleRect.position.x, (i32)textSystemVisibleRect.position.y },
			textSystemVisibleRect.extent);
		rectCollection.SetTreeOffsetAndWindowExtent(treeOffset, windowRect.extent);

		bool cursorOccluded = false;

		Widget::CursorMoveParams widgetParams = {
			.cursorPosition = modifiedEvent.position,
			.cursorPositionDelta = modifiedEvent.positionDelta,
			.customData = appData,
			.rectCollection = rectCollection,
			.textEngine = textEngine,
			.transientAlloc = transientAlloc,
			.window = eventWindowInfo, };

		if (windowNode.data.frontmostLayer.Has()) {
			cursorOccluded = windowNode.data.frontmostLayer.Get().rootWidget->CursorMove(
				widgetParams,
				Std::nullOpt,
				cursorOccluded);
		}

		widget.CursorMove(
			widgetParams,
			{},
			cursorOccluded);
	}

	impl::ImplData_FlushPostEventJobs(*this, appData);
}

void Context::PushEvent(WindowContentScaleEvent const& event) {
	auto& implData = Internal_ImplData();

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	windowNode.data.contentScale = event.scale;
}

void Context::PushEvent(WindowCursorExitEvent const& event) {
	auto& implData = Internal_ImplData();

	auto windowNodePtr = impl::GetWindowNodePtr(implData, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	if (implData.cursorWindowId.HasValue()) {
		auto const prevWindowId = implData.cursorWindowId.Value();
		if (event.windowId == prevWindowId) {
			implData.cursorWindowId = Std::nullOpt;
			if (windowNode.data.topLayout) {
				auto& widget = *windowNode.data.topLayout;
				widget.CursorExit();
			}
		}
	}
}

void Context::PushEvent(WindowFocusEvent const& event) {
	auto& implData = Internal_ImplData();
	auto& windowNodes = implData.windows;

	auto const windowNodeIt = impl::GetWindowNodeIt(windowNodes, event.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodeIt != windowNodes.end());

	if (event.gainedFocus) {
		// Move our window to the front
		auto tempWindowNode = Std::Move(*windowNodeIt);
		windowNodes.erase(windowNodeIt);
		windowNodes.emplace(windowNodes.begin(), Std::Move(tempWindowNode));
		// Call window focus gained event on widget?
	}
}

void Context::PushEvent(WindowMinimizeEvent const& event) {
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
	windowNode.data.insets = event.insets;
}

void Context::Event_Accessibility(
	Std::ConstAnyRef appData,
	Std::BumpAllocator& transientAlloc,
	RectCollection& rectCollection,
	TextEngine& textEngine,
	Widget::AccessibilityInfoPusher& pusher) const
{
	auto& implData = Internal_ImplData();

	for (auto const& windowNode : implData.windows) {
		auto const& windowData = windowNode.data;
		if (windowData.topLayout.Has()) {
			auto const eventWindowInfo = ToEventWindowInfo(windowNode, this->windowModifiers);

			impl::BuildRectCollection(
				textEngine,
				appData,
				rectCollection,
				false,
				windowNode,
				eventWindowInfo,
				transientAlloc);

			auto const& windowRect = windowNode.data.rect;
			Rect const visibleRect = impl::ApplyWindowInsetsToRect(
				windowRect,
				windowNode.data.insets.GetSourceInsets(WindowInsetSource::SystemBar));

			Rect const textSystemVisibleRect = impl::ApplyWindowInsetsToRect(
				windowRect,
				windowNode.data.insets.GetSourceInsets(WindowInsetSource::TextInputSystem));
			Math::Vec2Int treeOffset = impl::CalcTreeOffset(
				implData,
				rectCollection,
				windowRect.extent,
				{ (i32)textSystemVisibleRect.position.x, (i32)textSystemVisibleRect.position.y },
				textSystemVisibleRect.extent);

			auto const& root = *windowData.topLayout;
			Widget::AccessibilityTest_Params tempParams = {
				.window = eventWindowInfo,
				.rectColl = rectCollection,
				.transientAlloc = transientAlloc,
				.textManager = textEngine,
				.pusher = pusher,
				.treeOffset = treeOffset,
				.windowExtent = windowRect.extent, };
			root.AccessibilityTest(
				tempParams,
				{});
		}
	}
}

void Context::AdoptWindow(AdoptWindowInfo&& windowInfo)
{
	auto& implData = Internal_ImplData();

	impl::WindowNode newNode = {};
	newNode.id = windowInfo.id;

	newNode.data.clearColor = windowInfo.clearColor;
	newNode.data.contentScale = windowInfo.contentScale;
	newNode.data.dpi = windowInfo.dpiX;
	newNode.data.rect = windowInfo.rect;
	newNode.data.insets = windowInfo.insets;
	newNode.data.topLayout = Std::Move(windowInfo.widget);
	newNode.data.touchScrollSlopPx = windowInfo.touchScrollSlopPx;
	newNode.data.scrollBarWidthPx = windowInfo.scrollBarWidthPx;

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

void Context::Render2(
	Render2_Params const& params,
	DrawEngine& drawInfo,
	Std::ConstAnyRef customData) const
{
	DENGINE_IMPL_GUI_UNREACHABLE();
	auto const& implData = Internal_ImplData();
	auto& textEngine = params.textEngine;
	auto& rectCollection = params.rectCollection;
	auto& transientAlloc = params.transientAlloc;

	// TODO: Pass the RectCollIter for each top-level Widget.

	for (auto const& windowNode : implData.windows) {
		if (!windowNode.data.topLayout || windowNode.data.isMinimized) {
			continue;
		}

		auto eventWindowInfo = ToEventWindowInfo(windowNode, this->windowModifiers);

		impl::BuildRectCollection(
			textEngine,
			customData,
			rectCollection,
			true,
			windowNode,
			eventWindowInfo,
			transientAlloc);

		auto const& windowExtent = windowNode.data.rect.extent;

		auto& widget = *windowNode.data.topLayout;

		Rect const textSystemVisibleRect = impl::ApplyWindowInsetsToRect(
			{ {}, windowExtent },
			windowNode.data.insets.GetSourceInsets(WindowInsetSource::TextInputSystem));
		Math::Vec2Int treeOffset = impl::CalcTreeOffset(
			implData,
			rectCollection,
			windowExtent,
			{ (i32)textSystemVisibleRect.position.x, (i32)textSystemVisibleRect.position.y },
			textSystemVisibleRect.extent);
		rectCollection.SetTreeOffsetAndWindowExtent(treeOffset, windowExtent);

		Std::Opt<Widget::CurrentFocusWidgetData> currentFocusWidgetData;
		if (windowNode.data.focusWidget.Has()) {
			auto entryOpt = rectCollection.GetEntry(*windowNode.data.focusWidget.Get().widget);
			DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
			currentFocusWidgetData = Widget::CurrentFocusWidgetData {
				.widget = windowNode.data.focusWidget.Get().widget,
				.iter = entryOpt.Get(), };
		}

		Widget::Render_Params const renderParams {
			.appData = customData,
			.drawEngine = drawInfo,
			.focusedWidget = currentFocusWidgetData,
			.rectCollection = rectCollection,
			.textEngine = textEngine,
			.transientAlloc = transientAlloc,
			.window = eventWindowInfo, };
		widget.Render2(
			renderParams,
			{});
	}
}

void Context::RenderWindow(RenderWindowParams const& params) const {
	auto const& implData = Internal_ImplData();
	auto& appData = params.appData;
	auto& drawEngine = params.drawEngine;
	auto& textEngine = params.textEngine;
	auto& rectCollection = params.rectCollection;
	auto& transientAlloc = params.transientAlloc;

	auto windowNodePtr = impl::GetWindowNodePtr(implData, params.windowId);
	DENGINE_IMPL_GUI_ASSERT(windowNodePtr);
	auto& windowNode = *windowNodePtr;

	if (!windowNode.data.topLayout || windowNode.data.isMinimized) {
		return;
	}

	auto const eventWindowInfo = ToEventWindowInfo(windowNode, this->windowModifiers);

	impl::BuildRectCollection(
		textEngine,
		appData,
		rectCollection,
		true,
		windowNode,
		eventWindowInfo,
		transientAlloc);

	auto const& windowExtent = windowNode.data.rect.extent;

	Rect const textSystemVisibleRect = impl::ApplyWindowInsetsToRect(
		{ {}, windowExtent },
		windowNode.data.insets.GetSourceInsets(WindowInsetSource::TextInputSystem));
	Math::Vec2Int treeOffset = impl::CalcTreeOffset(
		implData,
		rectCollection,
		windowExtent,
		{ (i32)textSystemVisibleRect.position.x, (i32)textSystemVisibleRect.position.y },
		textSystemVisibleRect.extent);
	rectCollection.SetTreeOffsetAndWindowExtent(treeOffset, windowExtent);

	Std::Opt<Widget::CurrentFocusWidgetData> currentFocusWidgetData;
	if (windowNode.data.focusWidget.Has()) {
		auto entryOpt = rectCollection.GetEntry(*windowNode.data.focusWidget.Get().widget);
		DENGINE_IMPL_GUI_ASSERT(entryOpt.Has());
		currentFocusWidgetData = Widget::CurrentFocusWidgetData {
			.widget = windowNode.data.focusWidget.Get().widget,
			.iter = entryOpt.Get(), };
	}

	auto const& widget = *windowNode.data.topLayout;

	Widget::Render_Params const renderParams {
		.appData = appData,
		.drawEngine = drawEngine,
		.focusedWidget = currentFocusWidgetData,
		.rectCollection = rectCollection,
		.textEngine = textEngine,
		.transientAlloc = transientAlloc,
		.window = eventWindowInfo, };
	widget.Render2(
		renderParams,
		{});

	if (windowNode.data.frontmostLayer.Has()) {
		auto const& layer = windowNode.data.frontmostLayer.Get();
		auto const& layerWidget = *layer.rootWidget;
		auto layerEntryOpt = rectCollection.GetEntry(layerWidget);
		DENGINE_IMPL_GUI_ASSERT(layerEntryOpt.Has());
		auto const layerEntry = layerEntryOpt.Get();

		Std::Opt<Widget::CurrentFocusWidgetData> layerFocus;
		if (layer.focusWidget.Has()) {
			auto const& focusWidget = layer.focusWidget.Get();
			auto focusEntryOpt = rectCollection.GetEntry(*focusWidget.widget);
			DENGINE_IMPL_GUI_ASSERT(focusEntryOpt.Has());
			layerFocus = Widget::CurrentFocusWidgetData {
				.widget = focusWidget.widget,
				.iter = focusEntryOpt.Get(),
				.secondaryIndex = focusWidget.secondaryIndex, };
		}

		Widget::Render_Params const layerRenderParams {
			.appData = appData,
			.drawEngine = drawEngine,
			.focusedWidget = layerFocus,
			.rectCollection = rectCollection,
			.textEngine = textEngine,
			.transientAlloc = transientAlloc,
			.window = eventWindowInfo, };

		// Hard-bound the layer to the window framebuffer, on top of the extent clamp.
		drawEngine.PushScissor(Rect{ {}, windowExtent });
		layerWidget.Render2(layerRenderParams, layerEntry);
		drawEngine.PopScissor();
	}
}
