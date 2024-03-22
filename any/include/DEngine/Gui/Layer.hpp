#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/Utility.hpp>
#include <DEngine/Gui/RectCollection.hpp>
#include <DEngine/Gui/AllocRef.hpp>

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

		struct BuildSizeHints_Params {
			Context const& ctx;
			TextManager& textManager;
			EventWindowInfo const& window;
			Rect windowRect;
			Rect safeAreaRect;
			AllocRef transientAlloc;
			RectCollection::SizeHintPusher& pusher;
		};
		virtual void BuildSizeHints(BuildSizeHints_Params const& params) const {}

		struct BuildRects_Params {
			Context const& ctx;
			EventWindowInfo const& window;
			Rect windowRect;
			Rect visibleRect;
			TextManager& textManager;
			AllocRef transientAlloc;
			RectCollection::RectPusher& pusher;
		};
		virtual void BuildRects(BuildRects_Params const& params) const {}

		struct Render_Params {
			Context const& ctx;
			TextManager& textManager;
			AllocRef transientAlloc;
			EventWindowInfo const& window;
			Rect windowRect;
			Rect safeAreaRect;
			RectCollection const& rectCollection;
			DrawInfo& drawInfo;
		};
		virtual void Render(Render_Params const& params) const {}

		struct CursorMoveParams {
			Context& ctx;
			TextManager& textManager;
			EventWindowInfo const& window;
			Rect windowRect;
			Rect safeAreaRect;
			RectCollection const& rectCollection;
			CursorMoveEvent const& event;
			AllocRef const& transientAlloc;
		};
		// Returns true if the cursor was occluded
		[[nodiscard]] virtual bool CursorMove(
			CursorMoveParams const& params,
			bool occluded) { return false; }

		struct CursorPressParams {
			Context& ctx;
			TextManager& textManager;
			EventWindowInfo const& window;
			Rect windowRect;
			Rect safeAreaRect;
			RectCollection const& rectCollection;
			AllocRef const& transientAlloc;
			Math::Vec2Int cursorPos;
			CursorPressEvent const& event;
		};
		struct Press_Return
		{
			bool eventConsumed = false;
			bool destroyLayer = false;
		};
		[[nodiscard]] virtual Press_Return CursorPress(CursorPressParams const& params, bool eventConsumed) {
			return {};
		}

		struct TouchPressParams
		{
			Context& ctx;
			TextManager& textManager;
			EventWindowInfo const& window;
			Rect const& windowRect;
			Rect const& safeAreaRect;
			RectCollection const& rectCollection;
			AllocRef const& transientAlloc;
			TouchPressEvent const& event;
		};
		[[nodiscard]] virtual Press_Return TouchPress2(TouchPressParams const& params, bool eventConsumed) {
			return {};
		}

		struct TouchMoveParams
		{
			Context& ctx;
			TextManager& textManager;
			EventWindowInfo const& window;
			Rect const& windowRect;
			Rect const& safeAreaRect;
			RectCollection const& rectCollection;
			AllocRef const& transientAlloc;
			TouchMoveEvent const& event;
		};
		[[nodiscard]] virtual bool TouchMove2(TouchMoveParams const& params, bool occluded) {
			return false;
		}

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