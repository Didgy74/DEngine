#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>

#include <DEngine/Math/Vector.hpp>

#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/Application.hpp>
#include <DEngine/Scene.hpp>

namespace DEngine::Editor
{
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

		[[nodiscard]] EditorImpl& ImplData() const { return *implData; }

		void ProcessEvents();

		[[nodiscard]] DrawInfo GetDrawInfo() const;

		[[nodiscard]] bool CurrentlySimulating() const;
		[[nodiscard]] Scene& GetActiveScene() const;

		[[nodiscard]] static Context Create(
			App::WindowID mainWindow,
			Scene* scene,
			Gfx::Context* gfxCtx);


	protected:
		Context() = default;

		EditorImpl* implData = nullptr;
	};

	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoArrowMesh3D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTranslateArrowMesh2D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoTorusMesh2D();
	[[nodiscard]] std::vector<Math::Vec3> BuildGizmoScaleArrowMesh2D();
}