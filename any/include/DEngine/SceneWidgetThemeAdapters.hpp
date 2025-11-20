#pragma once

#include "Scene.hpp"

#include <DEngine/Gui/StdWidgets/Button.hpp>
#include <DEngine/Gui/StdWidgets/ButtonGroup.hpp>
#include <DEngine/Gui/StdWidgets/CollapsingHeader.hpp>
#include <DEngine/Gui/StdWidgets/StackLayout.hpp>
#include <DEngine/Gui/StdWidgets/Text.hpp>

namespace DEngine::SceneWidgetThemeAdapters {
	namespace impl {
		inline Scene::GuiTheme const& ThemeGrabber(Std::ConstAnyRef const& in) {
			auto const* scenePtr = in.Get<Scene>();
			DENGINE_IMPL_ASSERT(scenePtr != nullptr);
			return scenePtr->GetGuiTheme();
		}
	}

	constexpr auto button = [](Gui::Button::GetThemeParamsT const& in) {
		auto const& theme = impl::ThemeGrabber(in.appData);
		Gui::Button::Theme out = {};
		out.cornerRadius = theme.cornerRadius;
		out.margin = theme.textMargin;
		return out;
	};

	constexpr auto buttonGroup = [](Gui::ButtonGroup::GetThemeParamsT const& in) {
		auto const& theme = impl::ThemeGrabber(in.appData);
		Gui::ButtonGroup::Theme out = {};
		out.cornerRadius = theme.cornerRadius;
		out.margin = theme.textMargin;
		return out;
	};

	constexpr auto collapsingHeader = [](Gui::CollapsingHeader::GetThemeParamsT const& in) {
		auto const& theme = impl::ThemeGrabber(in.appData);
		Gui::CollapsingHeader::Theme out = {};
		out.contentMargin = theme.spacing;
		out.cornerRadius = theme.cornerRadius;
		out.textMargin = theme.textMargin;
		out.contentShadowHeight = theme.textMargin;
		return out;
	};

	constexpr auto dropdown = [](Gui::Dropdown::GetThemeParamsT const& in) {
		auto const& theme = impl::ThemeGrabber(in.appData);
		Gui::Dropdown::Theme out = {};
		out.margin = theme.textMargin;
		out.cornerRadius = theme.cornerRadius;
		return out;
	};

	constexpr auto stackLayout = [](Gui::StackLayout::GetThemeParamsT const& in) {
		auto const& theme = impl::ThemeGrabber(in.appData);
		Gui::StackLayout::Theme out = {};
		out.spacing = theme.spacing;
		return out;
	};

	constexpr auto text = [](Gui::Text::GetThemeParamsT const& in) {
		auto const& theme = impl::ThemeGrabber(in.appData);
		Gui::Text::Theme out = {};
		out.textMargin = theme.textMargin;
		return out;
	};
}