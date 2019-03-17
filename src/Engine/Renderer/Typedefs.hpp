#pragma once

#include <cstdint>

namespace Engine
{
	namespace Renderer
	{
		namespace Core
		{
			using AssetIntegerType = uint32_t;
		}

		enum class SpriteID : Core::AssetIntegerType {};

		enum class MeshID : Core::AssetIntegerType {};

		enum class API;
		class Viewport;
		class SceneData;
		struct RenderGraph;
		struct RenderGraphTransform;
		struct CameraInfo;
		struct PointLight;
	}
}
