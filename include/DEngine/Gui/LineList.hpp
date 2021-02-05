#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <DEngine/Std/Containers/Opt.hpp>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace DEngine::Gui
{
	class LineList : public Widget
	{
	public:
		static constexpr Math::Vec4 highlightColor = { 1.f, 1.f, 1.f, 0.5f };
		static constexpr Math::Vec4 alternatingLineColor = { 0.f, 0.f, 0.f, 0.1f };
		static constexpr u32 horizontalPadding = 20;

		void RemoveLine(uSize index);

		bool canSelect = false;
		Std::Opt<uSize> selectedLine;
		std::vector<std::string> lines;

		using Callback = std::function<void(LineList&)>;
		Callback selectedLineChangedCallback = nullptr;

		[[nodiscard]] virtual SizeHint GetSizeHint(Context const& ctx) const override;
	
		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CursorClick(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Math::Vec2Int cursorPos,
			CursorClickEvent event) override;

		virtual void TouchEvent(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			Gui::TouchEvent event) override;
	};
}