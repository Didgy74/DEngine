#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/WindowHandler.hpp>
#include <DEngine/Gui/DrawInfo.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/Layer.hpp>
#include <DEngine/Gui/AllocRef.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/AnyRef.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Span.hpp>

#include <functional>
#include <string>
#include <new>

// TEMPORARY
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Application.hpp>

namespace DEngine::Gui
{
	namespace impl { struct ImplData; }

	class RectCollection;

	class Context
	{
	public:
		TextManager& GetTextManager();

		static Context Create(
			WindowHandler& windowHandler,
			App::Context* appCtx,
			Gfx::Context* gfxCtx);
		Context(Context&&) noexcept;
		Context(Context const&) noexcept = delete;

		Context& operator=(Context const&) noexcept = delete;
		Context& operator=(Context&&) noexcept;

		struct Render2_Params {
			Gui::RectCollection& rectCollection;
			Std::BumpAllocator& transientAlloc;

			// Temporary stuff
			std::vector<Gfx::GuiVertex>& vertices;
			std::vector<u32>& indices;
			std::vector<Gfx::GuiDrawCmd>& drawCmds;
			std::vector<Gfx::NativeWindowUpdate>& windowUpdates;
			std::vector<u32>& utfValues;
			std::vector<Gfx::GlyphRect>& textGlyphRects;
		};
		void Render2(Render2_Params const& params, Std::ConstAnyRef appData) const;

		void Event_Accessibility(
			Std::ConstAnyRef appData,
			Std::BumpAllocator& transientAlloc,
			RectCollection& rectCollection,
			Widget::AccessibilityInfoPusher& pusher) const;

		void PushEvent(CursorPressEvent const&, Std::AnyRef appData);
		void PushEvent(CursorMoveEvent const&, Std::AnyRef appData);
		void PushEvent(TextInputEvent const&);
		void PushEvent(TextSelectionEvent const&);
		void PushEvent(TextDeleteEvent const&);
		void PushEvent(EndTextInputSessionEvent const&);
		void PushEvent(TouchMoveEvent const&, Std::AnyRef appData);
		void PushEvent(TouchPressEvent const&, Std::AnyRef appData);
		void PushEvent(WindowContentScaleEvent const&);
		void PushEvent(WindowCloseEvent const&);
		void PushEvent(WindowCursorExitEvent const&);
		void PushEvent(WindowFocusEvent const&);
		void PushEvent(WindowMinimizeEvent const&);
		void PushEvent(WindowMoveEvent const&);
		void PushEvent(WindowResizeEvent const&);

		using PostEventJobFnT = void(Context&, Std::AnyRef customData);
		template<class Callable>
		void PushPostEventJob(Callable const& in);

		struct AdoptWindowInfo {
			WindowID id;
			Math::Vec4 clearColor;
			Rect rect;
			Math::Vec2UInt visibleOffset;
			Extent visibleExtent;
			f32 dpiX;
			f32 dpiY;
			f32 contentScale;
			Std::Box<Widget> widget;
		};
		void AdoptWindow(AdoptWindowInfo&& windowInfo);

		void DestroyWindow(
			WindowID id);

		void TakeInputConnection(
			WindowID windowId,
			Widget& widget,
			SoftInputFilter softInputFilter,
			Std::Span<char const> currentText);
		void UpdateInputConnection(
			u64 selIndex,
			u64 selCount,
			Std::Span<u32 const> currentText);

		void ClearInputConnection(
			Widget& widget);

		[[nodiscard]] WindowHandler& GetWindowHandler() const;

		void SetFrontmostLayer(
			WindowID windowId,
			Std::Box<Layer>&& layer);

		f32 fontScale = 1.f;
		static constexpr float absoluteMinimumSize = 0.2f;
		f32 minimumHeightCm = 0.7f;
		f32 defaultMarginFactor = 0.25f;

	struct Impl;
	friend Impl;
	[[nodiscard]] Impl& Internal_ImplData();
	[[nodiscard]] Impl const& Internal_ImplData() const;

	protected:
		Context() = default;

		using PostEventJob_InvokeFnT = void(*)(
            void const* ptr,
            Context& context,
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