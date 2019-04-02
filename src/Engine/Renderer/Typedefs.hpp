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

		enum class SpriteID : Core::AssetIntegerType {};

		enum class MeshID : Core::AssetIntegerType {};

		struct DebugCreateInfo;
		struct AssetLoadCreateInfo;
		struct InitInfo;

		class MeshDocument;
		class TextureDocument;


		enum class API;
		class Viewport;
		struct RenderGraph;
		struct RenderGraphTransform;
		struct CameraInfo;
		struct PointLight;
	}
}
