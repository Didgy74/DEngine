#pragma once

/**
	DEngine::Gui::Widget is the polymorphic interface every UI node implements.

	Concepts, behaviour and rationale live in the markdown next to this module:
		src/Gui/Design.md      core concepts, layouting pipeline, RectCollection, rationale
		src/Gui/Widget.md      this API, and how to implement a Widget correctly
		src/Gui/Navigation.md  UI navigation and axis input
	This header is the source of truth for signatures; the docs for behaviour and intent.

	TODO: widgetRect/visibleRect are passed as separate params while the RectCollection is
	passed too. A "RectCollectionAccessor" that resolves to iterator, raw values or nothing
	would be cleaner.

	TODO: treeOffset and windowExtent are threaded per-event; they should probably move into
	each event's Params struct.

	TODO: some interactions still need a reference up to the UI host (e.g. Widget_TextInputSession),
	which complicates moving nodes across hosts. We might need an event to reset such references.
 */

#include <DEngine/Gui/RectCollection.hpp>

import DEngine.Gui.DrawEngine;
import DEngine.Gui.Events;
import DEngine.Gui.SizeHint;
import DEngine.Gui.TextEngine;
import DEngine.Gui.TextInputType;
import DEngine.Gui.WindowID;
import DEngine.Std.AnyRef;
import DEngine.Std.Common;
import DEngine.Std.Box;

namespace DEngine::Gui {
	class Widget;
	struct TouchEventConsumption {
		bool consumed = false;
		bool claimDragPriority = false;
	};

	// TODO: It would probably make sense to keep adding fields to this. Extent should probably
	// be added, as well as margins (could be used similar to Qt SafeArea API)
	//
	// TODO: Some of these settings are strictly part of theming, but we don't have any theming
	// APIs yet.
	struct EventWindowInfo {
		u32 caretWidth = 2;
		f32 contentScale = 1.f;
		f32 dpi = 0;
		f32 fontScale = 1.f;
		f32 minimumHeightCm = 0.5f;
		Math::Vec4 textColor = { 0.9f, 0.9f, 0.9f, 1.f };

		// The distance in pixels that the user can drag before it should be considered a scroll.
		Std::Opt<u64> touchScrollSlopPx;
		// The preferred width of a scrollbar.
		Std::Opt<u64> scrollBarWidthPx;
	};

	// Opaque logical id for a continuous input control (see WidgetEvent_AxisInput and
	// src/Gui/Navigation.md). The host maps physical inputs onto these; the app configures
	// each Widget with the id it responds to. Values below are just the std-widget defaults.
	enum class AxisId : u32 {
		Primary,   // main directional control: left stick / D-pad / arrow keys
		Secondary, // auxiliary control: right stick / wheel
		COUNT,
	};

	class Widget {
	public:
		Widget() = default;
		Widget(Widget const&) = delete;
		Widget(Widget&&) = delete;
		inline virtual ~Widget() = 0;

		Widget& operator=(Widget const&) = delete;
		Widget& operator=(Widget&&) = delete;

		class Widget_TextInputSession;
		class WidgetEvent_WindowHandler;
		class WidgetEvent_DeferredJobQueueBackend;
		class WidgetEvent_DeferredJobQueue;

		struct WidgetEvent_Measure_ReturnT;
		struct WidgetEvent_Measure_Params;
		// Measure the Widget's desired size. Measure children too, and both push the result
		// onto the SizeHintPusher and return it.
		[[nodiscard]] virtual WidgetEvent_Measure_ReturnT WidgetEvent_Measure(
			WidgetEvent_Measure_Params const& params) const = 0;

		struct WidgetEvent_Layout_Params;
		// A Layout assigns the Rects of its immediate children (even off-screen ones, so the
		// UI anchor can translate one into view later). See src/Gui/Design.md.
		virtual void WidgetEvent_Layout(
			WidgetEvent_Layout_Params const& params,
			Rect const& widgetRect,
			Rect const& visibleRect,
			Std::Opt<RectCollection::Iter> const& rectIterOpt) const {}

		// Contents may not be null.
		struct CurrentFocusWidgetData;

		struct Render_Params;
		virtual void WidgetEvent_Render(
			Render_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const = 0;

		struct CursorMoveParams;
		// Invoke on the root Widget. Return true if this Widget occludes the cursor point
		// (widgets behind it in Z should not react). The return feeds 'occluded' for later
		// widgets; layouts OR-accumulate it downward. Transparent overlays return false.
		// See src/Gui/Widget.md.
		virtual bool CursorMove(
			CursorMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded)
		{
			return false;
		}

		struct CursorPressParams;
		/*
			Should be invoked on the root Widget.

			Return true if event has been consumed.

		 	If the event was consumed already before this event was called,
		 	you can disregard the return-value.

		 	You can not end dispatching early. Pass all presses everywhere.
		*/
		virtual bool CursorPress2(
			CursorPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed)
		{
			return false;
		}

		struct WidgetEvent_TouchMoveEventData;
		struct WidgetEvent_TouchMoveParams;
		// Invoke on the root Widget. If a consumed event arrives for a finger-id you are
		// tracking, cancel that gesture. ("occluded" currently doubles as consumed.)
		// See src/Gui/Widget.md.
		// TODO: assumes TouchMove only fires for pressed fingers; styluses may differ.
		virtual TouchEventConsumption WidgetEvent_TouchMove(
			WidgetEvent_TouchMoveParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool occluded)
		{
			return {};
		}

		struct WidgetEvent_TouchPressEventData;
		struct WidgetEvent_TouchPressParams;
		// Should be invoked on the root Widget.
		virtual TouchEventConsumption WidgetEvent_TouchPress(
			WidgetEvent_TouchPressParams const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter,
			bool consumed)
		{
			return {};
		}

		struct WidgetEvent_TextInputParams;
		// Replace text at this Widget (mainly text-editing widgets). Broadcast to the tree;
		// the Widget owning the input session acts, Layouts just propagate. See src/Gui/Widget.md.
		// TODO: decide whether this should always carry the new selection.
		virtual void WidgetEvent_TextInput(
			AllocRef const& transientAlloc,
			WidgetEvent_TextInputParams const& event) {}
		struct WidgetEvent_TextSelectionParams;
		virtual void WidgetEvent_TextSelection(
			AllocRef const& transientAlloc,
			WidgetEvent_TextSelectionParams const& event) {}
		struct WidgetEvent_EndTextInputSessionParams;
		virtual void WidgetEvent_EndTextInputSession(
			AllocRef const& transientAlloc,
			WidgetEvent_EndTextInputSessionParams const& event) {}

		virtual void CursorExit() {}

		struct AccessibilityInfoElement {
			Rect rect;
			int textStart;
			int textCount;
			bool isClickable;
		};
		struct AccessibilityInfoPusher {
			virtual ~AccessibilityInfoPusher() = default;
			// Returns the offset that your text ended up at.
			[[nodiscard]] virtual int PushText(Std::Span<char const>) = 0;
			virtual void PushElement(u64, AccessibilityInfoElement const&) = 0;
		};
		struct AccessibilityTest_Params {
			EventWindowInfo const& window;
			RectCollection const& rectColl;
			AllocRef const& transientAlloc;
			TextEngine& textManager;
			AccessibilityInfoPusher& pusher;
			Math::Vec2Int const& treeOffset;
			Extent const& windowExtent;
		};
		// Should be invoked on the root Widget.
		virtual void AccessibilityTest(
			AccessibilityTest_Params const& params,
			Std::Opt<RectCollection::Iter> const& rectCollIter) const {}

		struct Widget_FocusWidgetResult {
			Widget* widget = nullptr;
			Std::Opt<u64> secondaryIndex;
		};

		enum class Widget_NavigationEventState : char;
		struct Widget_NavigationParams;
		struct Widget_NavigationState;
		// Invoke on the root Widget. Navigation must be thread-safe and immutable; it only
		// reports the new focus, returning a mutable Widget* so Activate can run on it later.
		// See src/Gui/Navigation.md.
		// TODO: on BoundaryHit, also return the focused item's Rect to thread into the next widget.
		virtual void WidgetEvent_Navigation(
			Widget_NavigationParams const&,
			Std::Opt<RectCollection::Iter> const&,
			Widget_NavigationState& event);
		/**
			For a Widget that is navigable, this function should be used if you only need the most
			trivial usecase, i.e your Widget functions as a standard button for navigation purposes.
		 */
		static void Widget_DefaultNavigationHandler(
			Widget& widget,
			Widget_NavigationParams const& param,
			Std::Opt<RectCollection::Iter> const& rectCollIterParam,
			Widget_NavigationState& event);

		struct WidgetEvent_ActivateParams;
		// Invoke the Widget's action, called directly on a single Widget. Mutable/effectful.
		// See src/Gui/Navigation.md.
		// TODO, RESEARCH: can activation be const-only? (It's why Navigation must yield a mutable ptr.)
		[[nodiscard]] virtual bool WidgetEvent_Activate(
			WidgetEvent_ActivateParams const&,
			Std::Opt<RectCollection::Iter> const&) { return false; }

		struct WidgetEvent_BackParams;
		// Semantic Back/Cancel on a subtree. Effectful, dispatched frontmost layer -> rear.
		// Return true = consumed (Android: the platform back action should not continue).
		// See src/Gui/Navigation.md.
		[[nodiscard]] virtual bool WidgetEvent_Back(
			WidgetEvent_BackParams const&,
			Std::Opt<RectCollection::Iter> const&) { return false; }

		class AxisInputSource;
		struct WidgetEvent_AxisInputParams;
		enum class WidgetEvent_AxisInputState_Enum : char;
		struct WidgetEvent_AxisInputEventState;
		virtual void WidgetEvent_AxisInput(
			WidgetEvent_AxisInputParams const&,
			Std::Opt<RectCollection::Iter> const&,
			WidgetEvent_AxisInputEventState&) {}
	};

	inline Widget::~Widget() = default;

	/**
		Represents ownership of the active platform text input session (IME / soft keyboard).
		Destroying this object immediately cancels the session from the platform's perspective.

		The owning Widget stores this as Std::Opt<Std::Box<TextInputSession>>.
		When the Widget is destroyed, the session is cancelled implicitly via destruction.
		No notification is sent back to the Widget in this case, as the Widget is already gone.

		Precondition: The platform data backing this session must outlive this object.
		This is guaranteed as long as the platform owns the UI tree.

		TODO: We need a mechanism to notify the owning Widget when the session has been stolen
		or cancelled. Maybe an addition to Widget interface?
	*/
	class Widget::Widget_TextInputSession {
	public:
		/**
			The implementation of this destructor is responsible for canceling any on-going
			text-input session (assuming it's still the current one).

			TODO: We likely some interface to tell the owning Widget that another Widget is making
			an input session, and therefore this one should be cancelled and not send the
			stop signals when it dies.
		 */
		virtual ~Widget_TextInputSession() = 0;
	};
	inline Widget::Widget_TextInputSession::~Widget_TextInputSession() = default;

	/**
		This is a thin wrapper object for an actual implementation.
		It is only meant to be alive and valid within the context of an event dispatching.
		It is also usually recreated between layers.
		Do NOT store a long-lived reference to this object.

		TODO: Some of these might count as modifying the UI tree, which means the effects should likely
		be queued and executed after the event dispatching is complete. If so, we should document it.

		TODO: If there are cases where it's illegal to call these methods, we might want to include a method
		that tells us when that is the case.
	 */
	class Widget::WidgetEvent_WindowHandler {
	public:
		virtual ~WidgetEvent_WindowHandler() = 0;

		/**
			This tells the host of the UI tree to create a new layer that floats on top of the current one,
			with a given Widget as root.

			A layer might be implemented as a dialog floating on top of the current layer, but
			kept within the same window. It might also be implemented as a separate window entirely.

			The focusItem controls whether an item should receive navigation focus immediately within
			the newly spawned UI tree.

			If focusItem is not Std::nullOpt, then focusItem.widget must be a widget inside
			the subtree of 'rootWidget'

			The parameter "preferredExtent" must not be (0, 0). The extent of the actual layer might
			be different, depending on the implementation. A classic example is if the layer
			is implemented as a child window that reaches outside the main window, but hits the
			border of the display.

			Returns true if the layer was successfully set as frontmost, false otherwise.

			TODO: Determine if it's valid for this function to actually fail.

			TODO: Determine whether it is legal to call this from the rearmost layer when there is already a frontmost
			layer. This would potentially cause us to destroy the existing UI tree mid-dispatch.

			TODO: Research whether it should be possible to resize this layer after establishing.
			I suspect it cannot, since a layer can conceptually be backed by a child window, and
			I don't think we can control this on all platforms...
		 */
		[[nodiscard]] virtual bool SetFrontmostLayer(
			Std::Box<Widget>&& rootWidget,
			Math::Vec2Int relativePos,
			Extent preferredExtent,
			Std::Opt<Widget_FocusWidgetResult> const& focusItem) = 0;

		/**
			This method will commonly be called in the middle of event dispatching, and often from a Widget that exists
			within the UI tree of the layer that is currently processing the event dispatching.

			This method must be implemented such that the layer is only deleted after any event dispatching
			is done for all layers.

			Ordinarily this function will be called from inside the event dispatching loop, and therefore
			immediately deleting the layer while dispatching will cause us to keep iterating over the layer
			after it has been deleted.

			It is generally considered a bug if this function is called multiple times within a single event dispatch.

			It is generally considered a bug to call this on the rearmost layer.
		*/
		virtual void QueueRemoveCurrentLayer() = 0;

		/**
			Establishes a connection for text input with the specified Widget, allowing interaction
			through a sof text input mechanism (such as an on-screen keyboard).

			The text is always ASCII-encoded (or extended ASCII)

			If the Widget with current focus has been destroyed, the system hosting the UI tree
			should generally handle it by scanning the RectCollection after the SizeHint
			phase is done.

			The implementation is recommended to set a pointer in the UI host that will remember
			which Widget called this function.

			The returned Widget_TextInputSession will stop the text-input session when it is
			destroyed. It should commonly be stored as Std::Opt<Box<Widget_TextInputSession>>> in
			the Widget implementation.

			Note! While the WidgetEvent_WindowHandler object is intended to be short-lived and
			live only for the duration of an event dispatch, the returned Widget_TextInputSession
			is intended to be long-lived.

			TODO: Determine if this function can fail. Until then, we assume it always returns
			a valid object.
		 */
		[[nodiscard]] virtual Std::Box<Widget_TextInputSession> TakeTextInputConnection(
			Widget& widget,
			TextInputType textInputFilter,
			Std::Span<char const> currentText,
			u64 selStart,
			u64 selCount) = 0;
	};

	inline Widget::WidgetEvent_WindowHandler::~WidgetEvent_WindowHandler() = default;

	/**
		This class is responsible for managing a queue of deferred jobs that are to be invoked
		after the current event dispatch is complete.

		This object is intended to be alive only for the duration of a single event dispatch.
	*/
	class Widget::WidgetEvent_DeferredJobQueueBackend {
	public:
		virtual ~WidgetEvent_DeferredJobQueueBackend() = default;

		using InvokeFnT = void(*)(
			void const* ptr,
			Std::AnyRef customData);
		using MoveFnT = void(*)(void* dstPtr, void* srcPtr);
		using DestroyFnT = void(*)(void* ptr);
		/**
			The registered callable is guaranteed to be movable.

			The registered callable must only be invoked exactly once.

			The implementation should allocate space for the callable given size and alignment.

			The implementation is expected to call moveFn immediately to move the callable into
			the allocated space.

			UI hosts are expected to call invokeFn on all registered jobs after event dispatch is
			complete. A job can be destroyed once the job has been invoked.
		*/
		virtual void PushPostEventJob(
			int size,
			int alignment,
			InvokeFnT invokeFn,
			void* callablePtr,
			MoveFnT moveFn,
			DestroyFnT destroyFn) = 0;
	};

	/**
		This class is responsible for managing a queue of deferred jobs that are to be invoked
		after the current event dispatch is complete.

		This object is intended to be alive only for the duration of a single event dispatch.

		This class is a wrapper for WidgetEvent_DeferredJobQueueBackend, which contains the actual
		implementation, implemented by the system hosting the UI.
	*/
	class Widget::WidgetEvent_DeferredJobQueue {
	public:
		WidgetEvent_DeferredJobQueue() = delete;
		WidgetEvent_DeferredJobQueue(WidgetEvent_DeferredJobQueueBackend& backend)
			: m_backend(&backend) {}
		WidgetEvent_DeferredJobQueue(WidgetEvent_DeferredJobQueue const&) = delete;
		WidgetEvent_DeferredJobQueue(WidgetEvent_DeferredJobQueue&&) = delete;
		WidgetEvent_DeferredJobQueue& operator=(WidgetEvent_DeferredJobQueue const&) = delete;
		WidgetEvent_DeferredJobQueue& operator=(WidgetEvent_DeferredJobQueue&&) = delete;

		/**
			This function posts a callable to be executed after the current event dispatch is done.

			This allows us to modify the UI tree without currently iterating it.

			The given callable is required to be movable.

			The callable is guaranteed to be invoked exactly once, after the current event dispatch is complete.

			It is considered code-smell to have many deferred jobs, as it can lead to complex and hard-to-debug behavior.

			TODO: Need to investigate design improvements regarding possible lifetime issues,
			such as when a callable captures a reference to a Widget which has since been destroyed.
			For now we assume that a Widget is always alive by the time we invoke these.
		 */
		template<class Callable> requires
			requires(Callable const& callable, Std::AnyRef customData) { callable(customData); }
		void PushPostEventJob(Callable&& input) {
			WidgetEvent_DeferredJobQueueBackend::InvokeFnT const invokeFn = [](void const* ptr, Std::AnyRef customData) {
				(*reinterpret_cast<Callable const*>(ptr))(customData);
			};
			WidgetEvent_DeferredJobQueueBackend::MoveFnT const moveFn = [](void* dstPtr, void* srcPtr) {
				auto const& temp = *reinterpret_cast<Callable const*>(srcPtr);
				new(dstPtr) Callable(Std::Move(temp));
			};
			WidgetEvent_DeferredJobQueueBackend::DestroyFnT const destroyFn = [](void* ptr){
				Callable& temp = *reinterpret_cast<Callable*>(ptr);
				temp.~Callable();
			};

			DENGINE_IMPL_GUI_ASSERT(m_backend != nullptr);
			m_backend->PushPostEventJob(
				sizeof(Callable),
				alignof(Callable),
				invokeFn,
				&input,
				moveFn,
				destroyFn);
		}

	private:
		WidgetEvent_DeferredJobQueueBackend* m_backend = nullptr;
	};

	struct Widget::CurrentFocusWidgetData {
		Widget* widget = nullptr;

		RectCollection::Iter iter;
		Std::Opt<u64> secondaryIndex;
	};

	struct Widget::WidgetEvent_Measure_ReturnT {
		// The RectCollection iterator of the inserted child
		RectCollection::Iter iter;
		SizeHint sizeHint;
	};

	struct Widget::WidgetEvent_Measure_Params {
		EventWindowInfo const& window;
		TextEngine& textEngine;
		Std::ConstAnyRef appData;
		AllocRef const& transientAlloc;
		RectCollection::SizeHintPusher& pusher;

		[[nodiscard]] auto FontScale() const noexcept { return window.fontScale; }
		[[nodiscard]] auto ContentScale() const noexcept { return window.contentScale; }
		[[nodiscard]] auto MinimumHeightCm() const noexcept { return window.minimumHeightCm; }
	};

	struct Widget::WidgetEvent_Layout_Params {
		EventWindowInfo const& window;
		TextEngine& textEngine;
		Std::ConstAnyRef appData;
		AllocRef const& transientAlloc;
		RectCollection::RectPusher& pusher;
	};

	struct Widget::CursorMoveParams {
		Math::Vec2Int cursorPosition;
		Math::Vec2Int cursorPositionDelta;
		Std::AnyRef customData;
		RectCollection const& rectCollection;
		TextEngine& textEngine;
		AllocRef const& transientAlloc;
		EventWindowInfo const& window;

		[[nodiscard]] auto CursorPos() const noexcept { return cursorPosition; }
		[[nodiscard]] auto CursorPosDelta() const noexcept { return cursorPosition; }
		[[nodiscard]] auto MinimumSizeCm() const noexcept { return window.minimumHeightCm; }
	};

	struct Widget::CursorPressParams {
		CursorButton const& cursorButton;
		Math::Vec2Int const& cursorPos;
		bool const& cursorPressed;
		Std::AnyRef const& customData;
		WidgetEvent_DeferredJobQueue& jobQueue;
		RectCollection const& rectCollection;
		TextEngine& textEngine;
		AllocRef const& transientAlloc;
		EventWindowInfo const& window;
		WidgetEvent_WindowHandler& windowHandler;

		[[nodiscard]] auto CursorButton() const noexcept { return cursorButton; }
		[[nodiscard]] auto CursorPos() const noexcept { return cursorPos; }
		[[nodiscard]] auto CursorPressed() const noexcept { return cursorPressed; }

		[[nodiscard]] auto MinimumSizeCm() const noexcept { return window.minimumHeightCm; }
	};

	struct Widget::WidgetEvent_ActivateParams {
		Std::AnyRef const& customData;
		RectCollection const& rectCollection;
		AllocRef const& transientAlloc;
		WidgetEvent_WindowHandler& windowHandler;
		Std::Opt<uSize> const& focusSecondaryIndex;

		[[nodiscard]] bool SetFrontmostLayer(
			Std::Box<Widget>&& widget,
			Math::Vec2Int relativePosition,
			Extent extent,
			Std::Opt<Widget_FocusWidgetResult> const& focusItem) const {
			return windowHandler.SetFrontmostLayer(Std::Move(widget), relativePosition, extent, focusItem);
		}
		void QueueRemoveFrontmostLayer() const {
			windowHandler.QueueRemoveCurrentLayer();
		}
		[[nodiscard]] Std::Box<Widget_TextInputSession> TakeTextInputSession(
			Widget& widget,
			TextInputType textInputFilter,
			Std::Span<char const> currentText,
			u64 selStart,
			u64 selCount) const
		{
			return windowHandler.TakeTextInputConnection(
				widget,
				textInputFilter,
				currentText,
				selStart,
				selCount);
		}
	};

	struct Widget::WidgetEvent_BackParams {
		Std::AnyRef const& customData;
		RectCollection const& rectCollection;
		AllocRef const& transientAlloc;
		WidgetEvent_WindowHandler& windowHandler;
		Std::Opt<CurrentFocusWidgetData> const& focusedWidget;

		void QueueRemoveFrontmostLayer() const {
			windowHandler.QueueRemoveCurrentLayer();
		}
	};

	enum class Widget::Widget_NavigationEventState : char {
		/**
			If there is no focused Widget:
			The algorithm will search for a Widget that can consume the
			navigation event.

			If there is already a focused Widget:
			The focused Widget has not yet been found in the Widget tree.
			The algorithm will keep searching.
		 */
		NotConsumed,
		/**
			The algorithm found the existing focused Widget, but encountered
			a local boundary where the currently processed Widget cannot
			consume the navigation event. The parent Widget must try to
			cross the boundary and search for a new Widget to put in focus.
		 */
		HitBoundary,
		/**
			This means the algorithm found a new suitable focused Widget.
			No need to keep iterating, just return the result.
		 */
		Consumed,
	};

	// This struct is only used during WidgetEvent_Navigation, to store variables
	// that are set once when starting event dispatching, at the UI tree root
	struct Widget::Widget_NavigationParams {
		EventWindowInfo const& window;
		AllocRef const& transientAlloc;
		RectCollection const& rectCollection;

		// Set to true if we want to use the vector navigation.
		// Set to false if we want to use back/forward navigation.
		bool directional = false;
		// Holds the angle of the navigation, where 0 points up and 1 implies a full
		// revolve. Rotates in counter-clockwise direction.
		f32 angle = 0.f;
		bool forward = false;

		Std::Opt<CurrentFocusWidgetData> currentFocusOpt;

		[[nodiscard]] Widget* CurrentFocusWidget() const noexcept {
			DENGINE_IMPL_GUI_ASSERT(!currentFocusOpt.Has() || currentFocusOpt.Get().widget != nullptr);
			return currentFocusOpt.Has() ? currentFocusOpt.Get().widget : nullptr;
		}

		[[nodiscard]] bool IsDirectionQuadrantUp() const noexcept {
			return this->directional && Gui::DirAngleIsQuadrantUp(this->angle);
		}
		[[nodiscard]] bool IsDirectionQuadrantDown() const noexcept {
			return this->directional && Gui::DirAngleIsQuadrantDown(this->angle);
		}
		[[nodiscard]] bool IsDirectionQuadrantLeft() const noexcept {
			return this->directional && Gui::DirAngleIsQuadrantLeft(this->angle);
		}
		[[nodiscard]] bool IsDirectionQuadrantRight() const noexcept {
			return this->directional && Gui::DirAngleIsQuadrantRight(this->angle);
		}
	};

	struct Widget::Widget_NavigationState {
	private:
		Widget_NavigationEventState m_state = Widget_NavigationEventState::NotConsumed;
	public:
		// Static analysis trips up if we initialize this inline.
		Widget_NavigationState() = default;
		Widget_NavigationState(Widget_NavigationState const&) = delete;
		Widget_NavigationState(Widget_NavigationState&&) = delete;
		Widget_NavigationState& operator=(Widget_NavigationState const&) = delete;
		Widget_NavigationState& operator=(Widget_NavigationState&&) = delete;

		[[nodiscard]] Widget_NavigationEventState State() const noexcept { return m_state; }
		[[nodiscard]] bool IsConsumed() const noexcept { return m_state == Widget_NavigationEventState::Consumed; }
		[[nodiscard]] bool IsHitBoundary() const noexcept { return m_state == Widget_NavigationEventState::HitBoundary; }

		// Valid when state == HitBoundary.
		// As output from a widget: the rect of the focused item that hit the boundary.
		// As input to the next sibling: the rect of the item that triggered the boundary upstream.
		// These are the same value — the child's output rect becomes the next sibling's origin rect.
		Rect sourceRectHitBoundary = {};

		// Only valid when state == Consumed.
		Widget* consumedWidget = nullptr;
		Std::Opt<u64> consumedSecondaryIndex;

		void Consume(Widget* widget, Std::Opt<u64> secondaryIndex = Std::nullOpt) {
			DENGINE_IMPL_GUI_ASSERT(this->m_state == Widget_NavigationEventState::NotConsumed || this->m_state == Widget_NavigationEventState::HitBoundary);
			this->m_state = Widget_NavigationEventState::Consumed;
			DENGINE_IMPL_GUI_ASSERT(widget != nullptr);
			this->consumedWidget = widget;
			this->consumedSecondaryIndex = Std::Move(secondaryIndex);
		}

		void HitBoundary(Rect sourceRect) {
			DENGINE_IMPL_GUI_ASSERT(this->m_state == Widget_NavigationEventState::NotConsumed);
			this->m_state = Widget_NavigationEventState::HitBoundary;
			this->sourceRectHitBoundary = sourceRect;
		}
	};

	struct Widget::Render_Params {
		Std::ConstAnyRef appData;
		DrawEngine& drawEngine;
		/**
			References the Widget that is currently in navigation focus.
		 */
		Std::Opt<CurrentFocusWidgetData> const& focusedWidget;
		RectCollection const& rectCollection;
		TextEngine& textEngine;
		AllocRef const& transientAlloc;
		EventWindowInfo const& window;

		[[nodiscard]] auto MinimumSizeCm() const noexcept { return window.minimumHeightCm; }
		[[nodiscard]] auto CaretWidth() const noexcept { return window.caretWidth; }
		[[nodiscard]] auto TextColor() const noexcept { return window.textColor; }
	};

	struct Widget::WidgetEvent_TextInputParams {
		// The start-index of the substring that should have it's content replaced.
		u64 start = 0;
		// The length of the substring that should have it's content replaced.
		u64 count = 0;

		/**
			The new substring to insert.
			This may be a nullptr, in which case means the destination
			substring should be completely removed and replaced with nothing.

			This substring is NOT null-terminated
		*/
		Std::Span<u32 const> newText;

		u64 newSelStart = 0;
		u64 newSelCount = 0;
	};

	struct Widget::WidgetEvent_TextSelectionParams {
		u64 start = 0;
		u64 count = 0;
	};

	struct Widget::WidgetEvent_EndTextInputSessionParams {};

	struct Widget::WidgetEvent_TouchMoveEventData {
		/**
			The ID of the finger/point associated with this touch-move event.

			TODO: Maybe use an enum class for this in the future.
		*/
		u8 id;
		// In pixel-space. Relative to the origin of this UI plane.
		Math::Vec2 position;
	};
	struct Widget::WidgetEvent_TouchMoveParams {
		Std::AnyRef customData;
		WidgetEvent_TouchMoveEventData const& event;
		RectCollection const& rectCollection;
		TextEngine& textEngine;
		AllocRef const& transientAlloc;
		EventWindowInfo const& window;

		[[nodiscard]] auto MinimumSizeCm() const noexcept { return window.minimumHeightCm; }
		[[nodiscard]] auto TouchId() const noexcept { return event.id; }
		[[nodiscard]] auto TouchPos() const noexcept { return event.position; }
	};

	struct Widget::WidgetEvent_TouchPressEventData {
		u8 id;
		Math::Vec2 position;
		bool pressed;
	};
	struct Widget::WidgetEvent_TouchPressParams {
		Std::AnyRef const& customData;
		WidgetEvent_TouchPressEventData const& event;
		WidgetEvent_DeferredJobQueue& jobQueue;
		RectCollection const& rectCollection;
		TextEngine& textEngine;
		AllocRef const& transientAlloc;
		EventWindowInfo const& window;
		WidgetEvent_WindowHandler& windowHandler;

		[[nodiscard]] auto MinimumSizeCm() const noexcept { return window.minimumHeightCm; }
		[[nodiscard]] auto TouchId() const noexcept { return event.id; }
		[[nodiscard]] auto TouchPos() const noexcept { return event.position; }
	};

	inline void Widget::WidgetEvent_Navigation(
		Widget_NavigationParams const& params,
		Std::Opt<RectCollection::Iter> const&,
		Widget_NavigationState&)
	{
		DENGINE_IMPL_ASSERT(params.CurrentFocusWidget() != this);
	}

	inline void Widget::Widget_DefaultNavigationHandler(
		Widget& widget,
		Widget_NavigationParams const& param,
		Std::Opt<RectCollection::Iter> const& rectCollIterParam,
		Widget_NavigationState& event)
	{
		auto const& rectColl = param.rectCollection;
		auto rectCollIterOpt = rectColl.GetEntry(widget, rectCollIterParam);
		DENGINE_IMPL_GUI_ASSERT(rectCollIterOpt.Has());
		auto rectCollIter = rectCollIterOpt.Get();
		auto widgetRect = rectColl.GetLocalRect(rectCollIter).widgetRect;

		if (event.State() == Widget_NavigationEventState::NotConsumed) {
			if (param.CurrentFocusWidget() == nullptr) {
				event.Consume(&widget, Std::nullOpt);
			} else if (param.CurrentFocusWidget() == &widget) {
				DENGINE_IMPL_GUI_ASSERT(!param.currentFocusOpt.Get().secondaryIndex.Has());
				event.HitBoundary(widgetRect);
			}
			return;
		}

		if (event.State() == Widget_NavigationEventState::HitBoundary) {
			DENGINE_IMPL_GUI_ASSERT(param.CurrentFocusWidget() != &widget);
			event.Consume(&widget, Std::nullOpt);
			return;
		}

		DENGINE_IMPL_GUI_UNREACHABLE();
	}

	class Widget::AxisInputSource {
	public:
		// Instantaneous deadzone-corrected value, [-1, 1] per component. See src/Gui/Navigation.md.
		// Positive X direction goes towards right.
		// Positive Y direction goes down.
		Math::Vec2 analog = {};

		// Signed discrete steps this frame, per component (0 or +-1 normally). Threshold and
		// auto-repeat are host-internal; the count is a hint. See src/Gui/Navigation.md.
		// Positive X direction goes towards right.
		// Positive Y direction goes down.
		Math::Vec2Int steps = {};
	};

	enum class Widget::WidgetEvent_AxisInputState_Enum : char {
		Unconsumed,
		Consumed,
	};

	struct Widget::WidgetEvent_AxisInputEventState {
	private:
		WidgetEvent_AxisInputState_Enum m_state = WidgetEvent_AxisInputState_Enum::Unconsumed;
	public:
		WidgetEvent_AxisInputEventState() = default;
		WidgetEvent_AxisInputEventState(WidgetEvent_AxisInputEventState const&) = delete;
		WidgetEvent_AxisInputEventState(WidgetEvent_AxisInputEventState&&) = delete;
		WidgetEvent_AxisInputEventState& operator=(WidgetEvent_AxisInputEventState const&) = delete;
		WidgetEvent_AxisInputEventState& operator=(WidgetEvent_AxisInputEventState&&) = delete;

		[[nodiscard]] WidgetEvent_AxisInputState_Enum State() const noexcept { return m_state; }
		[[nodiscard]] bool IsConsumed() const noexcept { return m_state == WidgetEvent_AxisInputState_Enum::Consumed; }

		void Consume() noexcept {
			DENGINE_IMPL_GUI_ASSERT(m_state != WidgetEvent_AxisInputState_Enum::Consumed);
			m_state = WidgetEvent_AxisInputState_Enum::Consumed;
		}
	};

	struct Widget::WidgetEvent_AxisInputParams {
		AxisInputSource const& axisSource;
		AxisId axisId;
		Std::AnyRef customData;
		// Amount of seconds since the last time this axis value was emitted.
		f64 deltaTime;
		Std::Opt<CurrentFocusWidgetData> const& focusedWidget;
		WidgetEvent_DeferredJobQueue& jobQueue;
		RectCollection const& rectCollection;
		TextEngine& textEngine;
		AllocRef const& transientAlloc;
		EventWindowInfo const& window;
		WidgetEvent_WindowHandler& windowHandler;
	};
}
