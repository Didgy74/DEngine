#pragma once

#include <DEngine/Gui/Events.hpp>
#include <DEngine/Gui/Utility.hpp>

namespace DEngine::Gui
{
	class Context;
	class DrawInfo;

	class Layer
	{
	public:
		Layer() = default;
		Layer(Layer const&) = delete;
		Layer(Layer&&) = delete;
		inline virtual ~Layer() = 0;

		virtual void Render(
			Context const& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			DrawInfo& drawInfo) const {}

		[[nodiscard]] virtual bool CursorMove(
			Context& ctx,
			Rect const& windowRect,
			Rect const& usableRect,
			CursorMoveEvent const& event,
			bool occluded) { return false; }

		struct Press_Return
		{
			bool eventConsumed = false;
			bool destroy = false;
		};
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