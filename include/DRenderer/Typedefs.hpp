#pragma once

#include <cstdint>
#include <functional>

namespace DRenderer
{
	class MeshDocument;
	using MeshDoc = MeshDocument;

	enum class API;
}

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
		
		class Viewport;

		struct MeshID;
		struct SpriteID;
		struct RenderGraph;
		struct RenderGraphTransform;

		struct CameraInfo;
		struct PointLight;
	}
}
