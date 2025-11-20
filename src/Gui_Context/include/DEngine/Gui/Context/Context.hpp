#pragma once

/**
	The Gui::Context class is a helper class for hosting UI planes inside native windows.
	While the Widget classes attempt to support multiple usecases, such as being hosted within
	3D scenes. The most common usecase is hosting UI planes inside native windows on a desktop
	or mobile display, where each UI plane is always inside a native window. The Gui::Context
	class helps us manage the UI planes in this scenario.
 */

#include <DEngine/Gui/CursorType.hpp>
#include <DEngine/Gui/Widget.hpp>

import DEngine.Gui.TextEngine;
import DEngine.Gui.WindowInsets;
import DEngine.Math.Vector;
import DEngine.Std.AnyRef;
import DEngine.Std.BumpAllocator;

namespace DEngine::Gui {
	namespace impl { struct ImplData; }

	class RectCollection;

	// I'm pretty sure this entire class is pretty shit and the data it manages should be managed by the user.
	// TODO: We only support one windows as a temporary measure while refactoring
	//
	// Note! This is deprecated and will be removed in the future. It should not be used. Any functionality currently
	// only existing here should be moved into Widget as a more flexible API. Several of the components of the Context
	// class should be directly implemented by application-specific code.
	class Context {
	public:
		class WindowHandler;

		static Context Create(WindowHandler& windowHandler);
		Context(Context&&) noexcept;
		Context(Context const&) noexcept = delete;

		Context& operator=(Context const&) noexcept = delete;
		Context& operator=(Context&&) noexcept;

		/**
			It's useful to be able to control the window settings more dynamically.
			These settings are applied to the window event info before it is passed onto the
			Widgets.

			TODO: They are currently designed as multipliers, maybe it would be better if we also
			could just directly override them.

			TODO: These are not allowed to be 0. We should probably include this in the
			data modeling somehow.

			TODO: We might just want to send in a lambda with every event to override/define these...
		 */
		struct WindowModifiers {
			f32 fontScaleMultiplier = 1.0f;
			f32 contentScaleMultiplier = 1.0f;
			f32 minimumHeightCm = 0.5f;
		};
		WindowModifiers windowModifiers;

		struct Render2_Params {
			// TODO: This function renders multiple windows. The RectCollection is unique for each
			// window. We need to rework this function.
			RectCollection& rectCollection;
			Std::BumpAllocator& transientAlloc;
			TextEngine& textEngine;
		};
		void Render2(
			Render2_Params const& params,
			DrawEngine& drawInfo,
			Std::ConstAnyRef appData) const;

		struct RenderWindowParams {
			Std::ConstAnyRef const& appData;
			DrawEngine& drawEngine;
			RectCollection& rectCollection;
			TextEngine& textEngine;
			Std::BumpAllocator& transientAlloc;
			WindowID const& windowId;
		};
		void RenderWindow(RenderWindowParams const&) const;

		void Event_Accessibility(
			Std::ConstAnyRef appData,
			Std::BumpAllocator& transientAlloc,
			RectCollection& rectCollection,
			TextEngine& textEngine,
			Widget::AccessibilityInfoPusher& pusher) const;

		void PushEvent(
			CursorPressEvent const&,
			TextEngine& textEngine,
			Std::AnyRef appData);
		void PushEvent(
			CursorMoveEvent const&,
			TextEngine& textEngine,
			Std::AnyRef appData);
		struct TextInputEvent {
			WindowID windowId = WindowID::Invalid;
			// The start-index of the substring that should have it's content replaced.
			u64 start = 0;
			// The length of the substring that should have it's content replaced.
			u64 count = 0;

			// The new substring to insert.
			// This may be a nullptr, in which case means the destination
			// substring should be completely removed and replaced with nothing.
			//
			// This substring is NOT null-terminated
			Std::Span<u32 const> newText;

			u64 newSelStart = 0;
			u64 newSelCount = 0;
		};
		void PushEvent(TextInputEvent const&);
		struct TextSelectionEvent {
			WindowID windowId = WindowID::Invalid;
			u64 start = 0;
			u64 count = 0;
		};
		void PushEvent(TextSelectionEvent const&);
		struct EndTextInputSessionEvent {
			WindowID windowId;
		};
		void PushEvent(EndTextInputSessionEvent const&);
		void PushEvent(
			TouchMoveEvent const&,
			TextEngine& textEngine,
			Std::AnyRef appData);
		void PushEvent(
			TouchPressEvent const&,
			TextEngine& textEngine,
			Std::AnyRef appData);
		void PushEvent(WindowContentScaleEvent const&);
		void PushEvent(WindowCloseEvent const&);
		void PushEvent(WindowCursorExitEvent const&);
		void PushEvent(WindowFocusEvent const&);
		void PushEvent(WindowMinimizeEvent const&);
		void PushEvent(WindowMoveEvent const&);
		void PushEvent(WindowResizeEvent const&);

		// TODO: I'm pretty sure this functionality is a requirement. But we need to move it from
		// the Context onto the Widget interface somehow.
		/*
		using PostEventJobFnT = void(Context&, Std::AnyRef customData);
		template<class Callable>
		void PushPostEventJob(Callable const& in);
		*/

		struct AdoptWindowInfo {
			WindowID id;
			Math::Vec4 clearColor;
			Rect rect;
			WindowInsets insets;
			f32 dpiX;
			f32 contentScale;

			// Note: These should probably be moved into a common struct somewhere.
			Std::Opt<u64> touchScrollSlopPx;
			Std::Opt<u64> scrollBarWidthPx;

			Std::Box<Widget> widget;
		};
		void AdoptWindow(AdoptWindowInfo&& windowInfo);

		[[nodiscard]] std::vector<WindowID> GetWindowIds() const;
		void DestroyWindow(
			WindowID id);

		[[nodiscard]] WindowHandler& GetWindowHandler() const;

		// TODO: What is this used for?
		//static constexpr float absoluteMinimumSize = 0.2f;
		// Minimum size for an interactable thing, in order to fit finger size.
		// This allows every button to have a fixed minimum size, so they are never too small to
		// use with fingers.
		//f32 minimumHeightCm = 0.5f;
		// Describes what kind of margin to apply around text. Is described as a factor, where 0 equals no margin,
		// and 1 describes margin equal to the size of the text.
		// TODO: This is a dumb design, it's mostly to try to keep things consistent. In the future
		// we probably need something like a theming system.
		//f32 defaultMarginFactor = 0.25f;
		// The width, in pixels, of a text editing caret.
		//u32 caretWidth = 2;
		//Math::Vec4 textColor = { 0.9f, 0.9f, 0.9f, 1.f };

	struct Impl;
	friend Impl;
	[[nodiscard]] Impl& Internal_ImplData();
	[[nodiscard]] Impl const& Internal_ImplData() const;

	protected:
		Context() = default;

		using PostEventJob_InvokeFnT = void(*)(
            void const* ptr,
            Std::AnyRef customData);
		using PostEventJob_InitCallableFnT = void(*)(void* dstPtr, void const* callablePtr);
		using PostEventJob_DestroyFnT = void(*)(void* ptr);
		void PushPostEventJob_Inner(
			int size,
			int alignment,
			PostEventJob_InvokeFnT invokeFn,
			void const* callablePtr,
			PostEventJob_InitCallableFnT initCallableFn,
			PostEventJob_DestroyFnT destroyFn);

		Impl* pImplData = nullptr;
	};
}

class DEngine::Gui::Context::WindowHandler {
public:
	virtual ~WindowHandler() noexcept = 0;

	virtual void CloseWindow(WindowID) = 0;

	virtual void SetCursorType(WindowID, CursorType) = 0;

	virtual void HideSoftInput() = 0;
	virtual void OpenSoftInput(
		WindowID windowId,
		Std::Span<char const> inputText,
		u64 selectionStart,
		u64 selectionCount,
		TextInputType inputFilter) = 0;
	virtual void UpdateTextInputConnection(
		WindowID windowId,
		u64 selIndex,
		u64 selCount,
		Std::Span<u32 const> inputText) = 0;
	virtual void UpdateTextInputConnectionSelection(
		WindowID windowId,
		u64 selIndex,
		u64 selCount) = 0;
};

inline DEngine::Gui::Context::WindowHandler::~WindowHandler() noexcept = default;

/*
template<class Callable>
void DEngine::Gui::Context::PushPostEventJob(Callable const& in)
{
	PostEventJob_InvokeFnT const invokeFn = [](
        void const* ptr,
        Context& context,
        Std::AnyRef customData)
    {
		(*reinterpret_cast<Callable const*>(ptr))(context, customData);
	};
	PostEventJob_InitCallableFnT const initFn = [](void* dstPtr, void const* callablePtr) {
		auto const& temp = *reinterpret_cast<Callable const*>(callablePtr);
		new(dstPtr) Callable(temp);
	};
	PostEventJob_DestroyFnT const destroyFn = [](void* ptr){
		auto const& temp = *reinterpret_cast<Callable*>(ptr);
		temp.~Callable();
	};

	PushPostEventJob_Inner(
		sizeof(Callable),
		alignof(Callable),
		invokeFn,
		&in,
		initFn,
		destroyFn);
}
*/