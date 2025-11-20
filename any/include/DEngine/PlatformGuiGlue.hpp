#pragma once

#include <DEngine/Gui/Context/Context.hpp>
#include <DEngine/Platform/Platform.hpp>

#include <DEngine/Std/Containers/FnScratchList.hpp>

namespace DEngine::PlatformGuiGlue {
	[[nodiscard]] inline Platform::SoftInputFilter ToPlatformSoftInputFilter(Gui::TextInputType input) {
		switch (input) {
			case Gui::TextInputType::NoFilter: return Platform::SoftInputFilter::Text;
			case Gui::TextInputType::SignedFloat:
			case Gui::TextInputType::UnsignedFloat: return Platform::SoftInputFilter::Float;
			case Gui::TextInputType::SignedInteger: return Platform::SoftInputFilter::Integer;
			case Gui::TextInputType::UnsignedInteger: return Platform::SoftInputFilter::UnsignedInteger;
			default: return Platform::SoftInputFilter::Text;
		}
		return {};
	}

	[[nodiscard]] inline Gui::WindowInsetSource ToGuiWindowInsetSource(Platform::WindowInsetSource input)
	{
		switch (input) {
			case Platform::WindowInsetSource::SystemBars: return Gui::WindowInsetSource::SystemBar;
			case Platform::WindowInsetSource::TextInputSystem: return Gui::WindowInsetSource::TextInputSystem;
			default: return Gui::WindowInsetSource::COUNT;
		}
	}

	[[nodiscard]] inline Gui::WindowInsetValues ToGuiWindowInsetValues(Platform::WindowInsets input)
	{
		return {
			.left = input.left,
			.right = input.right,
			.top = input.top,
			.bottom = input.bottom, };
	}

	[[nodiscard]] inline Gui::WindowInsets ToGuiWindowInsetsCollection(
		Std::Array<Platform::WindowInsets, (int)Platform::WindowInsetSource::COUNT> input)
	{
		Gui::WindowInsets insetsOut = {};
		for (int i = 0; i < input.Size(); i++) {
			auto inputInsetSource = ToGuiWindowInsetSource((Platform::WindowInsetSource)i);
			insetsOut.m_insets[(int)inputInsetSource] = ToGuiWindowInsetValues(input[i]);
		}

		return insetsOut;
	}

    [[nodiscard]] inline auto MakeGuiWindowAdoptInfo(
        Platform::Context::NewWindow_ReturnT const& platformWindowInfo,
        Math::Vec4 clearColor,
        Std::Box<Gui::Widget>&& widget)
    {
        Gui::Context::AdoptWindowInfo adoptInfo = {};
        adoptInfo.id = (Gui::WindowID)platformWindowInfo.windowId;
        adoptInfo.contentScale = platformWindowInfo.contentScale;
        adoptInfo.dpiX = platformWindowInfo.dpiX;
        adoptInfo.clearColor = clearColor;
        adoptInfo.rect = {
            platformWindowInfo.position,
            { platformWindowInfo.extent.width, platformWindowInfo.extent.height }};
		adoptInfo.insets = ToGuiWindowInsetsCollection(platformWindowInfo.insets);

        adoptInfo.touchScrollSlopPx = platformWindowInfo.touchScrollSlopPx;
        adoptInfo.scrollBarWidthPx = platformWindowInfo.scrollBarWidthPx;

        adoptInfo.widget = Std::Move(widget);

        return adoptInfo;
    }

	[[nodiscard]] inline auto MakePlatformAccessibilityElement(
		u64 widgetId,
		Gui::Widget::AccessibilityInfoElement const& guiAccesssItem)
	{
		Platform::AccessibilityUpdateElement returnValue = {};
    	returnValue.widgetId = widgetId;
    	returnValue.posX = guiAccesssItem.rect.position.x;
    	returnValue.posY = guiAccesssItem.rect.position.y;
    	returnValue.width = guiAccesssItem.rect.extent.width;
    	returnValue.height = guiAccesssItem.rect.extent.height;
    	returnValue.isClickable = guiAccesssItem.isClickable;
    	returnValue.textStart = guiAccesssItem.textStart;
    	returnValue.textCount = guiAccesssItem.textCount;
    	return returnValue;
    }

	struct BasicPlatformToGuiEventForwarder : Platform::EventForwarder {
	private:
		Std::FnScratchList<Gui::Context&, Gui::TextEngine&> fnList;
	public:
		void Consume(Gui::Context& context, Gui::TextEngine& engine) {
			fnList.Consume(context, engine);
		}
		bool emulateCursorAsTouch = false;

		void WindowResize(
			Platform::Context& appCtx,
			Platform::WindowID window,
			Platform::Extent extent,
			Std::Array<Platform::WindowInsets, (int)Platform::WindowInsetSource::COUNT> windowInsets) override;

		virtual void ButtonEvent(
			Platform::Context& appCtx,
			Platform::WindowID windowId,
			Platform::Button button,
			bool state) override;
		void CursorMove(
			Platform::Context& appCtx,
			Platform::WindowID window,
			Math::Vec2Int position,
			Math::Vec2Int positionDelta,
			Platform::CursorData cursorData) override;
		void CursorPress(
			Platform::Context& appCtx,
			Platform::WindowID windowId,
			Platform::CursorButton button,
			bool pressed,
			Platform::CursorData cursorData) override;
    	void TouchEvent(
    		Platform::WindowID windowId,
    		u8 id,
    		Platform::TouchEventType type,
    		Math::Vec2 position) override;
		void TextInputEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			u64 start,
			u64 count,
			Std::Span<u32 const> newString) override;

		void TextSelectionEvent(
			Platform::Context& ctx,
			Platform::WindowID windowId,
			u64 start,
			u64 count) override;
	};
}