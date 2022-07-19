#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/WindowHandler.hpp>
#include <DEngine/Gui/RectCollection.hpp>

#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Math/Vector.hpp>

#include <functional>
#include <vector>

namespace DEngine::Gui
{
	class TextManager;
}

namespace DEngine::Gui::impl
{
	struct WindowData
	{
		Rect rect = {};
		Math::Vec2UInt visibleOffset = {};
		Extent visibleExtent = {};
		bool isMinimized = {};
		Math::Vec4 clearColor = { 0, 0, 0, 1 };
		Std::Box<Widget> topLayout = {};
	};

	struct WindowNode
	{
		WindowID id;
		WindowData data;

		Std::Box<Layer> frontmostLayer = {};
	};
}

struct DEngine::Gui::Context::Impl
{
	Impl() = default;
	Impl(Impl const&) = delete;

	WindowHandler* windowHandler = nullptr;

	Widget* inputConnectionWidget = nullptr;

	// Windows are stored front to back in order of focus,
	// with the first element being the
	// frontmost window.
	std::vector<impl::WindowNode> windows;

	// The absolute position of the cursor.
	Math::Vec2Int cursorPosition = {};
	// Stores which window the cursor is inside, if any.
	Std::Opt<WindowID> cursorWindowId;



	RectCollection rectCollection;
	Std::FrameAlloc transientAlloc = Std::FrameAlloc::PreAllocate(1024 * 1024).Value();

	Std::FrameAlloc postEventAlloc = Std::FrameAlloc::PreAllocate(1024).Value();
	struct PostEventJob
	{
		void* ptr = nullptr;
		Context::PostEventJob_InvokeFnT invokeFn = nullptr;
	};
	std::vector<PostEventJob> postEventJobs;

	TextManager* textManager = nullptr;
};