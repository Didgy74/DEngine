#pragma once

#include <DEngine/Gui/RectCollection.hpp>
#include <DEngine/Gui/Widget.hpp>

import DEngine.Gui.Utility;
import DEngine.Gui.WindowInsets;

namespace DEngine::Gui
{
	class Context;
	class EventWindowInfo;

	// Note! This is deprecated and will be removed in the future. It should not be used. Any functionality currently
	// only existing here should be moved into Widget as a more flexible API. Several of the components of the Layer
	// class should be directly implemented by application-specific code.
	class Layer
	{
	public:
		Layer() = default;
		Layer(Layer const&) = delete;
		Layer(Layer&&) = delete;
		inline virtual ~Layer() = 0;

		struct BuildSizeHints_Params {
			TextEngine& textManager;
			EventWindowInfo const& window;
			Rect windowRect;
			Rect safeAreaRect;
			AllocRef transientAlloc;
			RectCollection::SizeHintPusher& pusher;
			Std::ConstAnyRef const& customData;

			[[nodiscard]] auto FontScale() const noexcept { return window.fontScale; }
			[[nodiscard]] auto MinimumSizeCm() const noexcept { return window.minimumHeightCm; }
		};
		virtual void BuildSizeHints(BuildSizeHints_Params const& params) const {}

		struct BuildRects_Params {
			EventWindowInfo const& window;
			Extent const& windowExtent;
			WindowInsets const& windowInsets;
			TextEngine& textManager;
			AllocRef transientAlloc;
			RectCollection::RectPusher& pusher;
		};
		virtual void BuildRects(BuildRects_Params const& params) const {}

		struct Render_Params {
			TextEngine& textManager;
			AllocRef transientAlloc;
			EventWindowInfo const& window;
			Extent const& windowExtent;
			WindowInsets const& windowInsets;
			RectCollection const& rectCollection;
			DrawEngine& drawInfo;
		};
		virtual void Render(Render_Params const& params) const {}

		struct CursorMoveParams {
			Context& ctx;
			TextEngine& textManager;
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
			TextEngine& textManager;
			EventWindowInfo const& window;
			Rect windowRect;
			Rect safeAreaRect; // TODO: Should be swapped out for window-insets structure
			RectCollection const& rectCollection;
			AllocRef const& transientAlloc;
			Math::Vec2Int cursorPos;
			CursorPressEvent const& event;
			Std::AnyRef const& customData;
		};
		struct Press_Return
		{
			bool eventConsumed = false;
			bool destroyLayer = false;
		};
		[[nodiscard]] virtual Press_Return CursorPress(CursorPressParams const& params, bool eventConsumed) {
			return {};
		}

		struct TouchPressParams {
			Context& ctx;
			TextEngine& textManager;
			WindowID windowId;
			EventWindowInfo const& window;
			Rect const& windowRect;
			Rect const& safeAreaRect;
			RectCollection const& rectCollection;
			AllocRef const& transientAlloc;
			TouchPressEvent const& event;
			Std::AnyRef const& customData;

			[[nodiscard]] WindowID GetWindowID() const { return windowId; }
		};
		[[nodiscard]] virtual Press_Return TouchPress2(TouchPressParams const& params, bool eventConsumed) {
			return {};
		}

		struct TouchMoveParams {
			Context& ctx;
			TextEngine& textManager;
			WindowID windowId;
			EventWindowInfo const& window;
			Rect const& windowRect;
			Rect const& safeAreaRect;
			RectCollection const& rectCollection;
			AllocRef const& transientAlloc;
			TouchMoveEvent const& event;

			[[nodiscard]] WindowID GetWindowId() const { return windowId; }
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
			DrawEngine& drawInfo) const {}

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