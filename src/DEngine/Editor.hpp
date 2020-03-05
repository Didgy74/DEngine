#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Array.hpp"
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
		ProjectionMode projectionMode = ProjectionMode::Perspective;

		u8 id = 255;
	};

	struct EditorData
	{
		struct Viewport
		{
			bool initialized = false;
			u8 id = 0;
			bool visible = false;
			void* imguiTextureID = nullptr;
			u32 width = 0;
			u32 height = 0;
			u32 renderWidth = 0;
			u32 renderHeight = 0;
			bool currentlyResizing = false;

			u8 cameraID = 0;

			Camera camera{};
		};

		std::vector<Viewport> viewports{};

		std::vector<Camera> cameras{};
	};

	void RenderImGuiStuff(EditorData& editorData, Gfx::Data& gfx);
}