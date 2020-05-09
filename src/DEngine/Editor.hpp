#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/Pair.hpp"
#include "DEngine/Gfx/Gfx.hpp"
#include "DEngine/Application.hpp"

#include "DEngine/Math/Vector.hpp"
#include "DEngine/Math/UnitQuaternion.hpp"

#include "ImGui/imgui.h"

#include <vector>
#include <chrono>
#include <string>

namespace DEngine
{
	class Scene;
}

namespace DEngine::Editor
{
	struct Camera
	{
	public:
		enum class ProjectionMode
		{
			Perspective,
			Orthgraphic
		};

		static constexpr f32 defaultFovY = 50.f;
		static constexpr f32 defaultOrtographicWidth = 10.f;
		static constexpr f32 defaultZNear = 0.1f;
		static constexpr f32 defaultZFar = 10000.f;
		static constexpr ProjectionMode defaultProjectionMode = ProjectionMode::Perspective;

		Math::Vec3 position{};
		Math::UnitQuat rotation{};
		f32 fov = defaultFovY;
		f32 orthographicWidth = defaultOrtographicWidth;
		f32 zNear = defaultZNear;
		f32 zFar = defaultZFar;
		ProjectionMode projectionMode = defaultProjectionMode;
	};

	struct Viewport
	{
		bool initialized = false;
		bool visible = false;
		bool paused = false;
		// Currently in free look
		bool currentlyControlling = false;
		u32 width = 0;
		u32 height = 0;
		u32 renderWidth = 0;
		u32 renderHeight = 0;
		bool currentlyResizing = false;

		Gfx::ViewportRef gfxViewportRef{};

		static constexpr uSize invalidCamID = static_cast<uSize>(-1);
		uSize cameraID = invalidCamID;
		Camera camera{};
	};

	struct EditorData
	{
		std::string log;

		u64 currentlySelectedEntity = static_cast<u64>(-1);

		std::vector<Std::Pair<uSize, Viewport>> viewports{};
		static constexpr uSize invalidViewportID = static_cast<uSize>(-1);

		std::vector<Std::Pair<uSize,Camera>> cameras{};

		ImGuiID dockSpaceID = 0;

		ImVec2 mainMenuBarSize{};

		std::chrono::steady_clock::time_point deltaTimePoint{};
		std::string displayedDeltaTime{};
		float deltaTimeRefreshTime = 0.25f;

		float viewportFullscreenHoldTime = 1.f;
		bool insideFullscreenViewport = false;
		uSize fullscreenViewportID = invalidViewportID;
		App::TouchInput::IDType fullscreen_posTouchID = App::TouchInput::invalidID;
		App::TouchInput::IDType fullscreen_rotTouchID = App::TouchInput::invalidID;
	};

	EditorData Initialize();
	void RenderImGuiStuff(EditorData& editorData, Scene& scene, Gfx::Data& gfx);
}