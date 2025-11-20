#pragma once


#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Platform/Platform.hpp>
#include <DEngine/Scene.hpp>

#include <DEngine/GuiTextEngineImpl.hpp>

#include <DEngine/Gui/StdWidgets/Button.hpp>
#include <DEngine/Gui/StdWidgets/ButtonGroup.hpp>
#include <DEngine/Gui/StdWidgets/CollapsingHeader.hpp>
#include <DEngine/Gui/StdWidgets/DockArea.hpp>
#include <DEngine/Gui/StdWidgets/Dropdown.hpp>
#include <DEngine/Gui/StdWidgets/Grid.hpp>
#include <DEngine/Gui/StdWidgets/LineIntEdit.hpp>
#include <DEngine/Gui/StdWidgets/LineFloatEdit.hpp>
#include <DEngine/Gui/StdWidgets/LineList.hpp>
#include <DEngine/Gui/StdWidgets/MenuButton.hpp>
#include <DEngine/Gui/StdWidgets/ScrollArea.hpp>
#include <DEngine/Gui/StdWidgets/StackLayout.hpp>
#include <DEngine/Gui/StdWidgets/Text.hpp>

namespace DEngine::Editor {
    enum class GizmoType : u8 { Translate, Rotate, Scale, COUNT };

	namespace Settings {
		enum class Color {
			Background,

			Button_Normal,
			Button_Active,

			Scrollbar_Normal,

			Window_Components,
			Window_Viewport,
			Window_DefaultViewportBackground,
			Window_Entities,
			Window_Theme,

			COUNT,
		};

		[[nodiscard]] constexpr Math::Vec4 FromHexadecimal(i32 hex) noexcept
		{
			Math::Vec3 out = {};

			auto const tempR = (hex & (255 << 16)) >> 16;
			out.x = (f32)tempR / 255.f;

			auto const tempG = (hex & (255 << 8)) >> 8;
			out.y = (f32)tempG / 255.f;

			auto const tempB = hex & 255;
			out.z = (f32)tempB / 255.f;

			return out.AsVec4(1.f);
		}

		[[nodiscard]] inline Std::Array<Math::Vec4, (int)Color::COUNT> BuildColorArray() noexcept {
			Std::Array<Math::Vec4, (int)Color::COUNT> returnVal = {};
			// Just give a default dumb color to everything, so we can tell if something isn't set.
			for (auto& item : returnVal)
				returnVal = { 1.f, 0.0f, 0.0f, 1.f };

			// Just to make for less typing.
			auto Ref = [&returnVal](Color in) -> Math::Vec4& { return returnVal[(int)in]; };
			auto Clamp = [](Math::Vec4 const& in) -> Math::Vec4 {
				Math::Vec4 returnVal = {};
				for (auto i = 0; i < 4; i++)
					returnVal[i] = Std::Clamp(in[i], 0.f, 1.f);
				return returnVal;
			};

			Ref(Color::Background) = FromHexadecimal(0x1A1A1A);

			//Ref(Color::Button_Normal) = FromHexadecimal(0x505050);
			Ref(Color::Button_Normal) = { 0.4, 0.4, 0.4, 1 };
			Ref(Color::Button_Active) = Clamp(
				(Ref(Color::Button_Normal).AsVec3() + Math::Vec3{ 0.2f, 0.2f, 0.2f })
				.AsVec4(1.f));

			Ref(Color::Scrollbar_Normal) = { 1, 1, 1, 0.2 };

			Ref(Color::Window_Viewport) = FromHexadecimal(0x8C2C23);
			Ref(Color::Window_DefaultViewportBackground) = FromHexadecimal(0x211E1E);
			Ref(Color::Window_Components) = FromHexadecimal(0x2A608C);
			Ref(Color::Window_Entities) = FromHexadecimal(0x407580);
			Ref(Color::Window_Theme) = FromHexadecimal(0x7A7252);

			return returnVal;
		}

		extern Std::Array<Math::Vec4, (int)Color::COUNT> colorArray;

		[[nodiscard]] inline Math::Vec4 GetColor(Color in) noexcept {
			return colorArray[(int)in];
		}

		constexpr auto inputFieldPrecision = 3;
	}

	struct DrawInfo {
		std::vector<u32> indices;
		std::vector<Gfx::GuiVertex> vertices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;
		std::vector<Gfx::ViewportUpdate> viewportUpdates;
		std::vector<Math::Vec3> lineVertices;
		std::vector<Gfx::LineDrawCmd> lineDrawCmds;
		std::vector<u32> utfValues;
		std::vector<Gfx::GlyphRect> textGlyphRects;
	};

	struct Theme {
		static constexpr auto defaultTextMargin = Gui::Distance::Mm(1.5, true);
		static constexpr auto defaultCornerRadius = Gui::Distance::Mm(2.5, true);
		static constexpr auto defaultSpacing = Gui::Distance::Mm(0.5, true);
		static constexpr auto defaultScrollbarPadding = defaultSpacing * 2;

		Gui::Distance textMargin = defaultTextMargin;
		Gui::Distance cornerRadius = defaultCornerRadius;
		Gui::Distance spacing = defaultSpacing;
		Gui::Distance scrollbarPadding = defaultScrollbarPadding;
	};

	class EditorImpl;

	class Context {
	public:
		Context(Context const&) = delete;
		Context(Context&&) noexcept;
		Context& operator=(Context const&) = delete;
		Context& operator=(Context&&) noexcept;
		virtual ~Context();

		[[nodiscard]] constexpr EditorImpl const& GetImplData() const noexcept {
			DENGINE_IMPL_ASSERT(m_implData != nullptr);
			return *m_implData;
		}
		[[nodiscard]] constexpr EditorImpl& GetImplData() noexcept {
			DENGINE_IMPL_ASSERT(m_implData != nullptr);
			return *m_implData;
		}

		[[nodiscard]] Theme const& GetTheme() const;
		[[nodiscard]] Theme& GetTheme();

		void ProcessEvents(float deltaTime);

		GuiTextEngineImpl& GetTextEngineImpl();

		[[nodiscard]] DrawInfo GetDrawInfo() const;

		[[nodiscard]] bool IsSimulating() const;
		[[nodiscard]] Scene& GetActiveScene();

        void BeginSimulatingScene();
        void StopSimulatingScene();

        [[nodiscard]] Std::Opt<Entity> const& GetSelectedEntity() const;
        void SelectEntity(Entity id);
        void UnselectEntity();
        [[nodiscard]] GizmoType GetCurrentGizmoType() const;

		struct CreateInfo {
			Platform::Context& appCtx;
			Gfx::Context& gfxCtx;
			Scene& scene;

			Platform::Context::NewWindow_ReturnT newWindowInfo;
		};
		[[nodiscard]] static Context Create(
			CreateInfo const& createInfo);

	protected:
		Context() = default;

		EditorImpl* m_implData = nullptr;

		// Returns true if any events happened
        bool FlushQueuedEventsToGui();
	};

	struct EditorGuiAppData_Render_Temp {
		struct ViewportWidgetRenderData {
			// On-screen size in pixels the gizmo should target for this viewport, derived from
			// the window's minimum interactable height during the render pass (where the
			// EventWindowInfo is available). Consumed later by BuildViewportUpdate, which still
			// computes the depth-dependent scale fresh every frame so perspective stays correct.
			f32 gizmoTargetSizePx = 0;
		};

		std::vector<Std::Pair<Gfx::ViewportID, ViewportWidgetRenderData>> viewports;
	};

	struct EditorGuiAppData {
		[[nodiscard]] Editor::Context& EditorCtx() {
			DENGINE_IMPL_ASSERT(m_editorCtx != nullptr);
			return *m_editorCtx;
		}
		[[nodiscard]] Editor::Context const& EditorCtx() const {
			DENGINE_IMPL_ASSERT(m_editorCtx != nullptr);
			return *m_editorCtx;
		}
		[[nodiscard]] EditorGuiAppData_Render_Temp* RenderResultData() const { return m_renderResultData; }

		EditorGuiAppData() = delete;
		EditorGuiAppData(EditorGuiAppData const&) = delete;
		EditorGuiAppData(EditorGuiAppData&&) = delete;
		EditorGuiAppData& operator=(EditorGuiAppData const&) = delete;
		EditorGuiAppData& operator=(EditorGuiAppData&&) = delete;

		EditorGuiAppData(Editor::Context& ctx) : m_editorCtx(&ctx) {}
		EditorGuiAppData(Editor::Context& ctx, EditorGuiAppData_Render_Temp& renderResultData)
			: m_editorCtx { &ctx },
			m_renderResultData { &renderResultData } {}

	private:
		Editor::Context* m_editorCtx = nullptr;
		EditorGuiAppData_Render_Temp* m_renderResultData = nullptr;
	};

	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoArrowMesh3D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTranslateArrowMesh2D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTorusMesh2D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoScaleArrowMesh2D();

	namespace WidgetThemeAdapters {
		constexpr auto button = [](Gui::Button::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();

			Gui::Button::Theme out;
			out.margin = appTheme.textMargin;
			out.cornerRadius = appTheme.cornerRadius;
			return out;
		};

		constexpr auto buttonGroup = [](Gui::ButtonGroup::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::ButtonGroup::Theme out;
			out.margin = appTheme.textMargin;
			out.cornerRadius = appTheme.cornerRadius;
			return out;
		};

		constexpr auto collapsingHeader = [](Gui::CollapsingHeader::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();

			Gui::CollapsingHeader::Theme out;
			out.textMargin = appTheme.textMargin;
			out.cornerRadius = appTheme.cornerRadius;
			out.contentMargin = appTheme.spacing;
			out.contentShadowHeight = appTheme.cornerRadius;
			return out;
		};

		constexpr auto dockArea = [](Gui::DockArea::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::DockArea::Theme out;
			out.tabTextMargin = appTheme.textMargin;
			out.tabCornerRadius = appTheme.cornerRadius;
			out.layerCornerRadius = appTheme.cornerRadius * 2;
			out.layerShadowFalloff = out.layerCornerRadius;
			return out;
		};

		constexpr auto dropdown = [](Gui::Dropdown::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::Dropdown::Theme out;
			out.margin = appTheme.textMargin;
			out.cornerRadius = appTheme.cornerRadius;
			return out;
		};

		constexpr auto grid = [](Gui::Grid::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::Grid::Theme out;
			out.spacing = appTheme.spacing;
			return out;
		};

		constexpr auto lineIntEdit = [](Gui::LineIntEdit::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::LineIntEdit::Theme out;
			out.margin = appTheme.textMargin;
			out.cornerRadius = appTheme.cornerRadius;
			return out;
		};

		constexpr auto lineFloatEdit = [](Gui::LineFloatEdit::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::LineFloatEdit::Theme out;
			out.margin = appTheme.textMargin;
			out.cornerRadius = appTheme.cornerRadius;
			return out;
		};

		constexpr auto lineList = [](Gui::LineList::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::LineList::Theme out;
			out.textMargin = appTheme.textMargin;
			return out;
		};

		constexpr auto menuButton = [](Gui::MenuButton::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::MenuButton::Theme out;
			out.textMargin = appTheme.textMargin;
			out.cornerRadius = appTheme.cornerRadius;
			return out;
		};

		constexpr auto scrollArea = [](Gui::ScrollArea::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::ScrollArea::Theme out;
			out.childPadding = appTheme.spacing;
			out.scrollbarPadding = appTheme.spacing;
			return out;
		};

		constexpr auto scrollAreaDefault = [](Gui::ScrollArea::GetThemeParamsT const&) {
			Gui::ScrollArea::Theme out;
			out.childPadding = Editor::Theme::defaultSpacing;
			out.scrollbarPadding = Editor::Theme::defaultSpacing;
			return out;
		};

		constexpr auto stackLayout = [](Gui::StackLayout::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::StackLayout::Theme out;
			out.spacing = appTheme.spacing;
			return out;
		};

		constexpr auto stackLayoutDefault = [](Gui::StackLayout::GetThemeParamsT const&) {
			Gui::StackLayout::Theme out;
			out.spacing = Editor::Theme::defaultSpacing;
			return out;
		};

		constexpr auto text = [](Gui::Text::GetThemeParamsT const& in) {
			auto* appDataPtr = in.appData.Get<EditorGuiAppData>();
			DENGINE_IMPL_ASSERT(appDataPtr != nullptr);
			auto& appTheme = appDataPtr->EditorCtx().GetTheme();
			Gui::Text::Theme out;
			out.textMargin = appTheme.textMargin;
			return out;
		};
	}
}