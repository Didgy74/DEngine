#pragma once

#include <DEngine/Gui/RectCollection.hpp>

#include <vector>

import DEngine.Std.Opt;

namespace DEngine::Gui::impl
{
	struct WindowData {
		// TODO: Possible improvement could be to never store the position of a window, just the
		// extent of the window.
		Rect rect = {};
		Math::Vec4 clearColor = { 0, 0, 0, 1 };
		// The content scale as reported by the OS. Owned by the platform; updated by
		// WindowContentScaleEvent and seeded at AdoptWindow. Never overwritten by user settings,
		// so the OS value is always recoverable.
		f32 contentScale = 1.f;
		f32 dpi = 0.f;
		f32 fontScale = 1.f;
		WindowInsets insets;
		bool isMinimized = {};
		Std::Box<Widget> topLayout;

		struct FocusWidget {
			Std::Opt<u64> secondaryIndex;
			// Should not be null if we have a valid FocusWidget object.
			Widget* widget = nullptr;
		};

		// TODO: I think we need to store the secondary index here as well.
		Std::Opt<FocusWidget> focusWidget;

		// TODO: Should eventually be reworked such that we can support child windows.
		struct FrontmostLayer {
			Extent extent;
			Std::Opt<FocusWidget> focusWidget;
			Math::Vec2Int relativePosition;
			Std::Box<Widget> rootWidget;
		};
		Std::Opt<FrontmostLayer> frontmostLayer;

		Std::Opt<u64> touchScrollSlopPx;
		Std::Opt<u64> scrollBarWidthPx;

		// TODO: We should track all finger-IDs that are being held per window,
		// so that we can access them during event dispatching and we can verify that
		// the events are happening in the correct order.
	};

	struct WindowNode {
		WindowID id;
		WindowData data;
	};

	// TODO: Right now we are assuming that our Gui::Context always outlives the Widget that ended
	// up with this session. This might not always be the case. For instance if the Widget is moved
	// out of the UI tree, and into a separate UI host? Such a case might warrant a dedicated
	// Widget event.
	struct WidgetTextInputSessionImpl : public Widget::Widget_TextInputSession {
		Context* m_ctx = nullptr;

		~WidgetTextInputSessionImpl();
	};

	struct ActiveTextInputSession {
		// Session lifetime is tied to the Widget that owns it.
		// We just need to know it's active, and to nullify its reference to Context
		// Cannot be null whenever this object is valid.
		WidgetTextInputSessionImpl* session = nullptr;

		// Cannot be null whenever this object is valid.
		Widget* widget = nullptr;
	};
}

struct DEngine::Gui::Context::Impl
{
	Impl() = default;
	Impl(Impl const&) = delete;

	WindowHandler* windowHandler = nullptr;

	Std::Opt<impl::ActiveTextInputSession> activeTextInputSession;

	// Windows are stored front to back in order of focus,
	// with the first element being the
	// frontmost window.
	std::vector<impl::WindowNode> windows;

	// The absolute position of the cursor.
	Math::Vec2Int cursorPosition = {};
	// Stores which window the cursor is inside, if any.
	Std::Opt<WindowID> cursorWindowId;



	// TODO: This was mostly to reuse the memory backing a RectCollection.
	// Might want to change this mechanism in the future.
	RectCollection rectCollection;
	Std::FrameAlloc transientAlloc = Std::FrameAlloc::PreAllocate(1024 * 1024).Value();

	Std::FrameAlloc postEventAlloc = Std::FrameAlloc::PreAllocate(1024).Value();
	struct PostEventJob {
		void* ptr = nullptr;
		uSize allocSize = 0;
		Context::PostEventJob_InvokeFnT invokeFn = nullptr;
		Context::PostEventJob_DestroyFnT destroyFn = nullptr;
	};
	std::vector<PostEventJob> postEventJobs;
};