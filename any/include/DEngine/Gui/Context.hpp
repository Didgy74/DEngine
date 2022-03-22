#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/WindowHandler.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/FrameAllocator.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Containers/Span.hpp>

#include <functional>
#include <string>

// TEMPORARY
#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Application.hpp>

namespace DEngine::Gui::impl { struct CtxPlacementNewTag{}; }
template<class T>
constexpr void* operator new(
	decltype(sizeof(int)) size,
	T* data,
	DEngine::Gui::impl::CtxPlacementNewTag) noexcept
{
	return data;
}
// Having this delete operator silences a compiler warning.
[[maybe_unused]] constexpr void operator delete(void* data,DEngine::Gui::impl::CtxPlacementNewTag) noexcept {}

namespace DEngine::Gui
{
	namespace impl { struct ImplData; }

	class Widget;
	class Layer;
	class RectCollection;

	class Context
	{
	public:
		static Context Create(
			WindowHandler& windowHandler,
			App::Context* appCtx,
			Gfx::Context* gfxCtx);
		Context(Context&&) noexcept;
		Context(Context const&) noexcept = delete;

		Context& operator=(Context const&) noexcept = delete;
		Context& operator=(Context&&) noexcept;

		void Render(
			std::vector<Gfx::GuiVertex>& vertices,
			std::vector<u32>& indices,
			std::vector<Gfx::GuiDrawCmd>& drawCmds,
			std::vector<Gfx::NativeWindowUpdate>& windowUpdates) const;

		struct Render2_Params
		{
			Gui::RectCollection& rectCollection;
			Std::FrameAllocator& transientAlloc;

			// Temporary stuff
			std::vector<Gfx::GuiVertex>& vertices;
			std::vector<u32>& indices;
			std::vector<Gfx::GuiDrawCmd>& drawCmds;
			std::vector<Gfx::NativeWindowUpdate>& windowUpdates;
		};
		void Render2(Render2_Params const& params) const;

		void PushEvent(CharEnterEvent const&);
		void PushEvent(CharEvent const&);
		void PushEvent(CharRemoveEvent const&);
		void PushEvent(CursorPressEvent const&);
		void PushEvent(CursorMoveEvent const&);
		void PushEvent(TextInputEvent const&);
		void PushEvent(EndTextInputSessionEvent const&);
		void PushEvent(TouchMoveEvent const&);
		void PushEvent(TouchPressEvent const&);
		void PushEvent(WindowCloseEvent const&);
		void PushEvent(WindowCursorExitEvent const&);
		void PushEvent(WindowFocusEvent const&);
		void PushEvent(WindowMinimizeEvent const&);
		void PushEvent(WindowMoveEvent const&);
		void PushEvent(WindowResizeEvent const&);

		using PostEventJobFnT = void(Context&);
		template<class Callable>
		void PushPostEventJob(Callable const& in);

		void AdoptWindow(
			WindowID id,
			Math::Vec4 clearColor,
			Rect windowRect,
			Math::Vec2UInt visibleOffset,
			Extent visibleExtent,
			Std::Box<Widget>&& widget);

		void DestroyWindow(
			WindowID id);

		void TakeInputConnection(
			Widget& widget,
			SoftInputFilter softInputFilter,
			Std::Span<char const> currentText);

		void ClearInputConnection(
			Widget& widget);

		[[nodiscard]] WindowHandler& GetWindowHandler() const;

		void SetFrontmostLayer(
			WindowID windowId,
			Std::Box<Layer>&& layer);

	struct Impl;
	friend Impl;
	[[nodiscard]] Impl& Internal_ImplData();
	[[nodiscard]] Impl const& Internal_ImplData() const;

	protected:
		Context() = default;

		using PostEventJob_InvokeFnT = void(*)(void const* ptr, Context& context);
		using PostEventJob_InitCallableFnT = void(*)(void* dstPtr, void const* callablePtr);
		void PushPostEventJob_Inner(
			int size,
			int alignment,
			PostEventJob_InvokeFnT invokeFn,
			void const* callablePtr,
			PostEventJob_InitCallableFnT initCallableFn);

		Impl* pImplData = nullptr;
	};
}

template<class Callable>
void DEngine::Gui::Context::PushPostEventJob(Callable const& in)
{
	PostEventJob_InvokeFnT const invokeFn = [](void const* ptr, Context& context) {
		(*reinterpret_cast<Callable const*>(ptr))(context);
	};
	PostEventJob_InitCallableFnT const initFn = [](void* dstPtr, void const* callablePtr) {
		auto const& temp = *reinterpret_cast<Callable const*>(callablePtr);
		new(dstPtr, impl::CtxPlacementNewTag{}) Callable(temp);
	};
	PushPostEventJob_Inner(
		sizeof(Callable),
		alignof(Callable),
		invokeFn,
		&in,
		initFn);
}