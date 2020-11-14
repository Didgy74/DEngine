#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>

#include "DEngine/Math/Vector.hpp"

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

		EditorImpl& ImplData() const { return *implData; }

		void ProcessEvents();

		DrawInfo GetDrawInfo() const;

		static Context Create(
			App::WindowID mainWindow,
			Scene* scene,
			Gfx::Context* gfxCtx);


	private:
		Context() = default;

		EditorImpl* implData = nullptr;
	};
}