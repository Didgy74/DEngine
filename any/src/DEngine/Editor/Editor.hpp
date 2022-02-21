#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>

#include <DEngine/Math/Vector.hpp>

#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Application.hpp>
#include <DEngine/Scene.hpp>

namespace DEngine::Editor
{
	namespace Settings
	{
		enum class Color
		{
			Background,

			Button_Normal,
			Button_Active,

			Scrollbar_Normal,

			Window_Components,
			Window_Viewport,
			Window_DefaultViewportBackground,
			Window_Entities,

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

		[[nodiscard]] inline Std::Array<Math::Vec4, (int)Color::COUNT> BuildColorArray() noexcept
		{
			Std::Array<Math::Vec4, (int)Color::COUNT> returnVal = {};
			// Just give a default dumb color to everything, so we can tell if something isn't set.
			for (auto& item : returnVal)
				returnVal = { 1.f, 0.0f, 0.0f, 1.f };

			// Just to make for less typing.
			auto Ref = [&returnVal](Color in) -> Math::Vec4& { return returnVal[(int)in]; };
			auto Clamp = [](Math::Vec4 const& in) -> Math::Vec4
				{
					Math::Vec4 returnVal = {};
					for (auto i = 0; i < 4; i++)
						returnVal[i] = Math::Clamp(in[i], 0.f, 1.f);
					return returnVal;
				};

			Ref(Color::Background) = FromHexadecimal(0x1A1A1A);

			Ref(Color::Button_Normal) = FromHexadecimal(0x4B4B4B);
			Ref(Color::Button_Active) = Clamp(
				(Ref(Color::Button_Normal).AsVec3() + Math::Vec3{ 0.25f, 0.25f, 0.25f })
				.AsVec4(1.f));

			Ref(Color::Scrollbar_Normal) = FromHexadecimal(0x595959);

			Ref(Color::Window_Viewport) = FromHexadecimal(0x8C2C23);
			Ref(Color::Window_DefaultViewportBackground) = FromHexadecimal(0x211E1E);
			Ref(Color::Window_Components) = FromHexadecimal(0x2A608C);
			Ref(Color::Window_Entities) = FromHexadecimal(0x407580);

			return returnVal;
		}

		extern Std::Array<Math::Vec4, (int)Color::COUNT> colorArray;

		[[nodiscard]] inline Math::Vec4 GetColor(Color in) noexcept
		{
			return colorArray[(int)in];
		}

		constexpr auto inputFieldPrecision = 3;
		constexpr auto defaultTextMargin = 10;
	}

	struct DrawInfo
	{
		std::vector<u32> indices;
		std::vector<Gfx::GuiVertex> vertices;
		std::vector<Gfx::GuiDrawCmd> drawCmds;
		std::vector<Gfx::NativeWindowUpdate> windowUpdates;
		std::vector<Gfx::ViewportUpdate> viewportUpdates;
		std::vector<Math::Vec3> lineVertices;
		std::vector<Gfx::LineDrawCmd> lineDrawCmds;
	};

	class EditorImpl;

	class Context
	{
	public:
		Context(Context const&) = delete;
		Context(Context&&) noexcept;
		Context& operator=(Context const&) = delete;
		Context& operator=(Context&&) noexcept;
		virtual ~Context();

		[[nodiscard]] constexpr EditorImpl const& GetImplData() const noexcept { return *m_implData; }
		[[nodiscard]] constexpr EditorImpl& GetImplData() noexcept { return *m_implData; }

		void ProcessEvents();

		[[nodiscard]] DrawInfo GetDrawInfo() const;

		[[nodiscard]] bool IsSimulating() const;
		[[nodiscard]] Scene& GetActiveScene();


		struct CreateInfo
		{
			App::Context& appCtx;
			Gfx::Context& gfxCtx;
			Scene& scene;
			App::WindowID mainWindow;
			Math::Vec2Int windowPos;
			App::Extent windowExtent;
			Math::Vec2UInt windowSafeAreaOffset;
			App::Extent windowSafeAreaExtent;
		};
		[[nodiscard]] static Context Create(
			CreateInfo const& createInfo);


	protected:
		Context() = default;

		EditorImpl* m_implData = nullptr;
	};

	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoArrowMesh3D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTranslateArrowMesh2D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTorusMesh2D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoScaleArrowMesh2D();
}