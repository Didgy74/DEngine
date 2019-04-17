#pragma once

#include <cstdint>
#include <functional>

namespace Engine
{
	namespace Renderer
	{
		namespace Core
		{
			using AssetIntegerType = uint32_t;
		}

		using ErrorMessageCallbackPFN = std::function<void(std::string_view)>;

		struct DebugCreateInfo;
		struct AssetLoadCreateInfo;
		struct InitInfo;

		class MeshDocument;

		enum class API;
		class Viewport;

		struct MeshID;
		struct SpriteID;
		struct RenderGraph;
		struct RenderGraphTransform;

		struct CameraInfo;
		struct PointLight;
	}
}
