#include "Editor.hpp"
#include "EditorImpl.hpp"

#include "ComponentWidgets.hpp"
#include "ViewportWidget.hpp"

#include <DEngine/Gui/StdWidgets/Slider.hpp>

#include <format>

import DEngine.Math.Common;
import DEngine.Std.Box;

using namespace DEngine;

namespace DEngine::Editor {
	// Defined further down; the Menu's Theme entry spawns it into the dock area.
	Std::Box<Gui::Widget> BuildThemeControllerPanel();

	Std::Box<Gui::Widget> CreateNavigationBar(EditorImpl& editorImpl) {
		auto* stackLayout = new Gui::StackLayout(Gui::StackLayout::Direction::Horizontal);
		stackLayout->getThemeFn = Editor::WidgetThemeAdapters::stackLayout;

		auto* menuButton = new Gui::MenuButton;
		stackLayout->AddWidget(Std::BoxAdopt(menuButton));
		menuButton->getThemeFn = Editor::WidgetThemeAdapters::menuButton;
		menuButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
		menuButton->title = "Menu";
		{
			// Create button for Entities window
			Gui::MenuButton::Line line = {};
			line.title = "Entities";
			line.toggled = true;
			line.togglable = true;
			line.callback = [](
				Gui::MenuButton::Line& line,
				Gui::Widget::WidgetEvent_DeferredJobQueue& jobQueue)
			{
				if (line.toggled) {
					auto job = [](Std::AnyRef customData) {
						auto* appDataPtr = customData.Get<Editor::EditorGuiAppData>();
						DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
						auto& editorCtx = appDataPtr->EditorCtx();

						editorCtx.GetImplData().dockArea->AddWindow(
							"Entities",
							Settings::GetColor(Settings::Color::Window_Entities),
							Std::BoxAdopt(new EntityIdList(editorCtx.GetImplData())));
					};
					jobQueue.PushPostEventJob(Std::Move(job));
				}
				line.toggled = true;
			};
			//menuButton->submenu.lines[(int)FileMenuEnum::Entities] = Std::Move(line);
			menuButton->submenu.lines.emplace_back(Std::Move(line));
		}
		{
			// Create button for Components window
			Gui::MenuButton::Line line = {};
			line.title = "Components";
			line.toggled = true;
			line.togglable = true;
			line.callback = [](
				Gui::MenuButton::Line& line,
				Gui::Widget::WidgetEvent_DeferredJobQueue& jobQueue)
			{
				if (line.toggled) {
					auto job = [](Std::AnyRef customData) {
						auto* appDataPtr = customData.Get<Editor::EditorGuiAppData>();
						DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
						auto& editorCtx = appDataPtr->EditorCtx();
						editorCtx.GetImplData().dockArea->AddWindow(
							"Components",
							Settings::GetColor(Settings::Color::Window_Components),
							Std::BoxAdopt(new ComponentList(editorCtx.GetImplData())));
					};
					jobQueue.PushPostEventJob(Std::Move(job));
				}
				line.toggled = true;
			};
			//menuButton->submenu.lines[(int)FileMenuEnum::Components] = Std::Move(line);
			menuButton->submenu.lines.emplace_back(Std::Move(line));
		}
		{
			// Create button for Theme window
			Gui::MenuButton::Line line = {};
			line.title = "Theme";
			line.toggled = true;
			line.togglable = true;
			line.callback = [](
				Gui::MenuButton::Line& line,
				Gui::Widget::WidgetEvent_DeferredJobQueue& jobQueue)
			{
				if (line.toggled) {
					auto job = [](Std::AnyRef customData) {
						auto* appDataPtr = customData.Get<Editor::EditorGuiAppData>();
						DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
						auto& editorCtx = appDataPtr->EditorCtx();
						editorCtx.GetImplData().dockArea->AddWindow(
							"Theme",
							Settings::GetColor(Settings::Color::Window_Theme),
							Editor::BuildThemeControllerPanel());
					};
					jobQueue.PushPostEventJob(Std::Move(job));
				}
				line.toggled = true;
			};
			menuButton->submenu.lines.emplace_back(Std::Move(line));
		}
		{
			// Create button for adding viewport
			Gui::MenuButton::Line line{};
			line.title = "New viewport";
			line.toggled = false;
			line.togglable = false;
			line.callback = [](Gui::MenuButton::Line& line, Gui::Widget::WidgetEvent_DeferredJobQueue& jobQueue) {
				auto job = [](Std::AnyRef customData) {
					auto* appDataPtr = customData.Get<Editor::EditorGuiAppData>();
					DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
					auto& editorCtx = appDataPtr->EditorCtx();
					editorCtx.GetImplData().dockArea->AddWindow(
						"Viewport",
						Settings::GetColor(Settings::Color::Window_Viewport),
						Std::BoxAdopt(new ViewportWidget(editorCtx.GetImplData())));
				};
				jobQueue.PushPostEventJob(Std::Move(job));
			};
			//menuButton->submenu.lines[(int)FileMenuEnum::NewViewport] = Std::Move(line);
			menuButton->submenu.lines.emplace_back(Std::Move(line));
		}

		// Delta time counter at the top
		auto deltaTimeText = new Gui::Text;
		stackLayout->AddWidget(Std::BoxAdopt(deltaTimeText));
		editorImpl.test_fpsText = deltaTimeText;
		deltaTimeText->getThemeFn = Editor::WidgetThemeAdapters::text;
		deltaTimeText->expandX = false;
		deltaTimeText->getTextFn = [](Gui::Text::GetTextParams const& in) {
			auto* appDataPtr = in.appData.Get<Editor::EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto const& deltaTime = appDataPtr->EditorCtx().GetImplData().deltaTime;
			int fpsCount = 0.f;
			if (deltaTime <= 0.0001f) {
				fpsCount = 0;
			} else {
				fpsCount = (int)Std::Round(1.f / deltaTime);
			}
			std::format_to(in.pusher.BackInserter(), "{:.3f}ms / {} FPS", deltaTime * 1000, fpsCount);
		};

		auto playButton = new Gui::Button;
		stackLayout->AddWidget(Std::Box{playButton});
		playButton->colors.normal = Settings::GetColor(Settings::Color::Button_Normal);
		playButton->colors.toggled = Settings::GetColor(Settings::Color::Button_Active);
		playButton->text = "Play";
		playButton->getThemeFn = Editor::WidgetThemeAdapters::button;
		playButton->type = Gui::Button::Type::Toggle;
		playButton->activateFn = [](Gui::Button::ActivateParams const& in) {
			auto *appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& editorCtx = appDataPtr->EditorCtx();
			if (in.btn.GetToggled()) {
				editorCtx.BeginSimulatingScene();
			} else {
				editorCtx.StopSimulatingScene();
			}
		};

		auto gizmoBtnGroup = new Gui::ButtonGroup;
		editorImpl.gizmoTypeBtnGroup = gizmoBtnGroup;
		stackLayout->AddWidget(Std::BoxAdopt(gizmoBtnGroup));
		gizmoBtnGroup->colors.inactiveColor = Settings::GetColor(Settings::Color::Button_Normal);
		gizmoBtnGroup->colors.activeColor = Settings::GetColor(Settings::Color::Button_Active);
		gizmoBtnGroup->getThemeFn = Editor::WidgetThemeAdapters::buttonGroup;
		gizmoBtnGroup->AddButton("Translate");
		gizmoBtnGroup->AddButton("Rotate");
		gizmoBtnGroup->AddButton("Scale");

		return Std::BoxAdopt(stackLayout);
	}

	Std::Box<Gui::Widget> BuildThemeControllerPanel() {
		auto outerScrollArea = Std::BoxAdopt(new Gui::ScrollArea);
		outerScrollArea->getThemeFn = Editor::WidgetThemeAdapters::scrollAreaDefault;
		outerScrollArea->scrollbarInactiveColor = Settings::GetColor(Settings::Color::Scrollbar_Normal);

		auto* vertStack = new Gui::StackLayout(Gui::StackLayout::Dir::Vertical);
		outerScrollArea->child = Std::BoxAdopt(vertStack);
		vertStack->getThemeFn = Editor::WidgetThemeAdapters::stackLayoutDefault;

		// Set up font scale controls
		{
			auto* horiStack = new Gui::StackLayout(Gui::StackLayout::Dir::Horizontal);
			horiStack->getThemeFn = Editor::WidgetThemeAdapters::stackLayoutDefault;
			vertStack->AddWidget(Std::BoxAdopt(horiStack));
			{
				auto* label = new Gui::Text;
				horiStack->AddWidget(Std::BoxAdopt(label));
				label->getThemeFn = Editor::WidgetThemeAdapters::text;
				label->expandX = false;
				label->getTextFn = [](Gui::Text::GetTextParams const& in) {
					std::format_to(in.pusher.BackInserter(), "Content scale {:.2f}", in.windowInfo.contentScale);
				};
			}

			{
				auto *decrementBtn = new Gui::Button;
				decrementBtn->text = "-";
				decrementBtn->getThemeFn = Editor::WidgetThemeAdapters::button;
				decrementBtn->activateFn = [](Gui::Button::ActivateParams const& in) {
					auto* appData = in.appData.Get<Editor::EditorGuiAppData>();
					DENGINE_IMPL_ASSERT(appData != nullptr);
					auto& multiplier =
						appData->EditorCtx().GetImplData().guiCtx->windowModifiers.contentScaleMultiplier;
					if (multiplier > 0.5f) {
						multiplier -= 0.1f;
					}
				};
				horiStack->AddWidget(Std::BoxAdopt(decrementBtn));
			}
			{
				auto *incrementBtn = new Gui::Button;
				incrementBtn->text = "+";
				incrementBtn->getThemeFn = Editor::WidgetThemeAdapters::button;
				incrementBtn->activateFn = [](Gui::Button::ActivateParams const& in) {
					auto* appData = in.appData.Get<Editor::EditorGuiAppData>();
					DENGINE_IMPL_ASSERT(appData != nullptr);
					auto& multiplier = appData->EditorCtx().GetImplData().guiCtx->windowModifiers.contentScaleMultiplier;
					multiplier += 0.1f;
				};
				horiStack->AddWidget(Std::BoxAdopt(incrementBtn));
			}
		}

		// Set up font scale controls
		{
			auto* horiStack = new Gui::StackLayout(Gui::StackLayout::Dir::Horizontal);
			horiStack->getThemeFn = Editor::WidgetThemeAdapters::stackLayoutDefault;
			vertStack->AddWidget(Std::BoxAdopt(horiStack));
			{
				auto* label = new Gui::Text;
				horiStack->AddWidget(Std::BoxAdopt(label));
				label->getThemeFn = Editor::WidgetThemeAdapters::text;
				label->expandX = false;
				label->getTextFn = [](Gui::Text::GetTextParams const& in) {
					std::format_to(in.pusher.BackInserter(), "Font {:.2f}", in.windowInfo.fontScale);
				};
			}

			{
				auto *decrementBtn = new Gui::Button();
				decrementBtn->text = "-";
				decrementBtn->getThemeFn = Editor::WidgetThemeAdapters::button;
				decrementBtn->activateFn = [](Gui::Button::ActivateParams const& in) {
					auto* appData = in.appData.Get<Editor::EditorGuiAppData>();
					DENGINE_IMPL_ASSERT(appData != nullptr);
					auto& multiplier = appData->EditorCtx().GetImplData().guiCtx->windowModifiers.fontScaleMultiplier;
					if (multiplier > 0.5f) {
						multiplier -= 0.1f;
					}
				};
				horiStack->AddWidget(Std::Box{decrementBtn});
			}
			{
				auto *incrementBtn = new Gui::Button;
				incrementBtn->text = "+";
				incrementBtn->getThemeFn = Editor::WidgetThemeAdapters::button;
				incrementBtn->activateFn = [](Gui::Button::ActivateParams const& in) {
					auto* appData = in.appData.Get<Editor::EditorGuiAppData>();
					DENGINE_IMPL_ASSERT(appData != nullptr);
					auto& fontScaleMultiplier = appData->EditorCtx().GetImplData().guiCtx->windowModifiers.fontScaleMultiplier;
					fontScaleMultiplier += 0.1f;
				};
				horiStack->AddWidget(Std::Box{incrementBtn});
			}
		}

		{
			auto* horiStack = new Gui::StackLayout(Gui::StackLayout::Dir::Horizontal);
			horiStack->getThemeFn = Editor::WidgetThemeAdapters::stackLayoutDefault;
			vertStack->AddWidget(Std::BoxAdopt(horiStack));
			{
				auto* label = new Gui::Text;
				horiStack->AddWidget(Std::BoxAdopt(label));
				label->getThemeFn = Editor::WidgetThemeAdapters::text;
				label->text = "Min";
				label->expandX = false;
				label->getTextFn = [](Gui::Text::GetTextParams const& in) {
					std::format_to(in.pusher.BackInserter(), "Min {:.1f}", in.minimumHeightCm);
				};
			}

			constexpr auto minValue = 0.3f;
			constexpr auto maxValue = 1.5f;
			auto* slider = new Gui::Slider;
			horiStack->AddWidget(Std::BoxAdopt(slider));
			slider->valueChangedFn = [](Gui::Slider::ValueChangedFnParamsT const& in) {
				auto* appData = in.appData.Get<Editor::EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appData != nullptr);
				appData->EditorCtx().GetImplData().guiCtx->windowModifiers.minimumHeightCm =
					Math::Lerp(minValue, maxValue, in.newValue);
			};
		}

		// Set up text margin controls
		{
			auto* horiStack = new Gui::StackLayout(Gui::StackLayout::Dir::Horizontal);
			horiStack->getThemeFn = Editor::WidgetThemeAdapters::stackLayoutDefault;
			vertStack->AddWidget(Std::BoxAdopt(horiStack));

			auto* label = new Gui::Text;
			horiStack->AddWidget(Std::BoxAdopt(label));
			label->getThemeFn = Editor::WidgetThemeAdapters::text;
			label->expandX = false;
			label->getTextFn = [](Gui::Text::GetTextParams const& in) {
				auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& theme = appDataPtr->EditorCtx().GetTheme();
				std::format_to(
					in.pusher.BackInserter(),
					"Text margin: {:.1f}",
					theme.textMargin.GetFloat());
			};

			constexpr auto minValue = 0.f;
			constexpr auto maxValue = 3.f;
			auto* slider = new Gui::Slider;
			horiStack->AddWidget(Std::BoxAdopt(slider));
			slider->valueChangedFn = [](Gui::Slider::ValueChangedFnParamsT const& in) {
				auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& theme = appDataPtr->EditorCtx().GetTheme();
				theme.textMargin = Gui::Distance::Mm(
					Math::Lerp(minValue, maxValue, in.newValue),
					true);
			};
		}

		// Add line for controlling corner radius
		{
			auto* horiStack = new Gui::StackLayout(Gui::StackLayout::Dir::Horizontal);
			horiStack->getThemeFn = Editor::WidgetThemeAdapters::stackLayoutDefault;
			vertStack->AddWidget(Std::BoxAdopt(horiStack));

			auto* label = new Gui::Text;
			horiStack->AddWidget(Std::BoxAdopt(label));
			label->getThemeFn = Editor::WidgetThemeAdapters::text;
			label->expandX = false;
			label->getTextFn = [](Gui::Text::GetTextParams const& in) {
				auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& theme = appDataPtr->EditorCtx().GetTheme();
				std::format_to(
					in.pusher.BackInserter(),
					"Corner radius: {:.1f}",
					theme.cornerRadius.GetFloat());
			};

			constexpr auto minValue = 0.f;
			constexpr auto maxValue = 3.f;
			auto* slider = new Gui::Slider;
			horiStack->AddWidget(Std::BoxAdopt(slider));
			slider->valueChangedFn = [](Gui::Slider::ValueChangedFnParamsT const& in) {
				auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& theme = appDataPtr->EditorCtx().GetTheme();
				theme.cornerRadius = Gui::Distance::Mm(
					Math::Lerp(minValue, maxValue, in.newValue),
					true);
			};
		}

		// Add line for controlling spacing
		{
			auto* horiStack = new Gui::StackLayout(Gui::StackLayout::Dir::Horizontal);
			horiStack->getThemeFn = Editor::WidgetThemeAdapters::stackLayoutDefault;
			vertStack->AddWidget(Std::BoxAdopt(horiStack));

			auto* label = new Gui::Text;
			horiStack->AddWidget(Std::BoxAdopt(label));
			label->getThemeFn = Editor::WidgetThemeAdapters::text;
			label->expandX = false;
			label->getTextFn = [](Gui::Text::GetTextParams const& in) {
				auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& theme = appDataPtr->EditorCtx().GetTheme();
				std::format_to(
					in.pusher.BackInserter(),
					"Spacing: {:.1f}",
					theme.spacing.GetFloat());
			};

			constexpr auto minValue = 0.f;
			constexpr auto maxValue = 3.f;
			auto* slider = new Gui::Slider;
			horiStack->AddWidget(Std::BoxAdopt(slider));
			slider->valueChangedFn = [](Gui::Slider::ValueChangedFnParamsT const& in) {
				auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
				DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
				auto& theme = appDataPtr->EditorCtx().GetTheme();
				theme.spacing = Gui::Distance::Mm(
					Math::Lerp(minValue, maxValue, in.newValue),
					true);
			};
		}

		return outerScrollArea;
	}

	Std::Box<Gui::Widget> SetupUiRoot(EditorImpl& editorImpl) {
		auto* outmostLayout = new Gui::StackLayout(Gui::StackLayout::Dir::Vertical);

		outmostLayout->AddWidget(CreateNavigationBar(editorImpl));

		auto* dockArea = new Gui::DockArea;
		dockArea->getThemeFn = Editor::WidgetThemeAdapters::dockArea;
		editorImpl.dockArea = dockArea;
		outmostLayout->AddWidget(Std::BoxAdopt(dockArea));

		dockArea->AddWindow(
			"Entities",
			Settings::GetColor(Settings::Color::Window_Entities),
			Std::BoxAdopt(new EntityIdList(editorImpl)));
		dockArea->AddWindow(
			"Components",
			Settings::GetColor(Settings::Color::Window_Components),
			Std::BoxAdopt(new ComponentList(editorImpl)));
		dockArea->AddWindow(
			"Theme",
			Settings::GetColor(Settings::Color::Window_Theme),
			Editor::BuildThemeControllerPanel());
		dockArea->AddWindow(
			"Viewport",
			Settings::GetColor(Settings::Color::Window_Viewport),
			Std::BoxAdopt(new ViewportWidget(editorImpl)));

		return Std::BoxAdopt(outmostLayout);
	}
}