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
		void AddLine(std::string_view const& text);
		std::string_view GetLine(uSize index) const;
		Std::Opt<uSize> GetSelectedLine() const;
		[[nodiscard]] uSize LineCount() const;
		void RemoveLine(uSize index);
		void SetCanSelect(bool);

		using Callback = std::function<void(LineList&)>;
		Callback selectedLineChangedCallback = nullptr;


		[[nodiscard]] virtual Gui::SizeHint GetSizeHint(Context const& ctx) const override;
	

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


	protected:
		bool canSelect = false;
		Std::Opt<uSize> selectedLine;
		std::vector<std::string> lines;
	};
}