#pragma once

#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Button.hpp>

#include <DEngine/Std/Containers/Box.hpp>

#include <functional>
#include <string_view>

namespace DEngine::Gui
{
	class CollapsingHeader : public Widget
	{
	public:
		static constexpr Math::Vec4 fieldBackgroundColor = { 1.f, 1.f, 1.f, 0.25f };

		CollapsingHeader(bool collapsed = true);

		using CollapseFn = void(bool collapsed);
		std::function<CollapseFn> collapseCallback;
		[[nodiscard]] StackLayout& GetChildStackLayout() noexcept { return *childStackLayoutPtr; }
		[[nodiscard]] StackLayout const& GetChildStackLayout() const noexcept { return *childStackLayoutPtr; }
		[[nodiscard]] bool IsCollapsed() const noexcept;
		void SetCollapsed(bool value);
		void SetHeaderText(std::string_view text);



		[[nodiscard]] virtual SizeHint GetSizeHint(
			Context const& ctx) const override;

		virtual void Render(
			Context const& ctx,
			Extent framebufferExtent,
			Rect widgetRect,
			Rect visibleRect,
			DrawInfo& drawInfo) const override;

		virtual void CharEnterEvent(
			Context& ctx) override;

		virtual void CharEvent(
			Context& ctx,
			u32 utfValue) override;

		virtual void CharRemoveEvent(
			Context& ctx) override;
		
		virtual void InputConnectionLost() override;

		virtual void CursorMove(
			Context& ctx,
			WindowID windowId,
			Rect widgetRect,
			Rect visibleRect,
			CursorMoveEvent event) override;

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

	private:
		StackLayout mainStackLayout = StackLayout(StackLayout::Direction::Vertical);
		Std::Box<StackLayout> childStackLayoutBox;
		StackLayout* childStackLayoutPtr = nullptr;
	};
}