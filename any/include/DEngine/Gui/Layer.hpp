#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/Utility.hpp>
#include <DEngine/Gui/RectCollection.hpp>

namespace DEngine::Gui
{
	class Context;
	class TextManager;
	class DrawInfo;

	class Layer
	{
	public:
		Layer() = default;
		Layer(Layer const&) = delete;
		Layer(Layer&&) = delete;
		inline virtual ~Layer() = 0;

		struct BuildSizeHints_Params
		{
			Context const& ctx;
			TextManager& textManager;
			Rect windowRect;
			Rect safeAreaRect;
			Std::FrameAllocator& transientAlloc;
			RectCollection::SizeHintPusher& pusher;
		};
		virtual void BuildSizeHints(BuildSizeHints_Params const& params) const {}

		struct BuildRects_Params
		{
			Context const& ctx;
			Rect windowRect;
			Rect visibleRect;
			TextManager& textManager;
			Std::FrameAllocator& transientAlloc;
			RectCollection::RectPusher& pusher;
		};
		virtual void BuildRects(BuildRects_Params const& params) const {}

		struct Render_Params
		{
			Context const& ctx;
			TextManager& textManager;
			Rect windowRect;
			Rect safeAreaRect;
			RectCollection const& rectCollection;
			DrawInfo& drawInfo;
		};
		virtual void Render(Render_Params const& params) const {}

		struct CursorMoveParams
		{
			Context& ctx;
			TextManager& textManager;
			Rect windowRect;
			Rect safeAreaRect;
			RectCollection const& rectCollection;
			CursorMoveEvent const& event;
		};
		// Returns true if the cursor was occluded
		[[nodiscard]] virtual bool CursorMove(
			CursorMoveParams const& params,
			bool occluded) { return false; }

		struct CursorPressParams
		{
			Context& ctx;
			TextManager& textManager;
			Rect windowRect;
			Rect safeAreaRect;
			RectCollection const& rectCollection;
			Math::Vec2Int cursorPos;
			CursorPressEvent const& event;
		};
		struct Press_Return
		{
			bool eventConsumed = false;
			bool destroyLayer = false;
		};
		[[nodiscard]] virtual Press_Return CursorPress(
			CursorPressParams const& params,
			bool eventConsumed) { return {}; }


		[[nodiscard]] virtual bool CursorMove(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			CursorMoveEvent const& event,
			bool occluded) { return false; }

		virtual void Render(
			Context const& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			DrawInfo& drawInfo) const {}


		[[nodiscard]] virtual Press_Return CursorPress(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			Math::Vec2Int cursorPos,
			CursorPressEvent const& event) { return {}; }

		[[nodiscard]] virtual bool TouchMove(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			TouchMoveEvent const& event,
			bool occluded) { return false; }

		[[nodiscard]] virtual Press_Return TouchPress(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			TouchPressEvent const& event) { return {}; }
	};

	inline Layer::~Layer() {}
}