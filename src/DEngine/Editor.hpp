#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/Pair.hpp"
#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/Math/Vector/Vector.hpp"
#include "DEngine/Math/UnitQuaternion.hpp"

#include <vector>

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

		Math::Vec3D position{};
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

		uSize cameraID = invalidCamID;
		static constexpr uSize invalidCamID = static_cast<uSize>(-1);
		Camera camera{};
	};

	struct EditorData
	{
		std::vector<Cont::Pair<uSize, Viewport>> viewports{};

		std::vector<Cont::Pair<uSize,Camera>> cameras{};
	};

	void RenderImGuiStuff(EditorData& editorData, Gfx::Data& gfx);
}