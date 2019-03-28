#pragma once

#include "Renderer.hpp"

#include <vector>
#include <functional>
#include <any>
#include <unordered_map>

namespace Engine
{
	namespace Renderer
	{
		namespace Core
		{
			struct Data
			{
				RenderGraph renderGraph;
				RenderGraphTransform renderGraphTransform;
				CameraInfo cameraInfo;

				API activeAPI = API::None;

				std::unordered_map<MeshID, size_t> meshReferences;
				std::unordered_map<SpriteID, size_t> spriteReferences;

				std::vector<MeshID> loadMeshQueue;
				std::vector<MeshID> unloadMeshQueue;
				std::vector<SpriteID> loadSpriteQueue;
				std::vector<SpriteID> unloadSpriteQueue;

				std::vector<std::unique_ptr<Viewport>> viewports;

				std::function<void(void)> Draw;
				std::function<void(const std::vector<SpriteID>&, const std::vector<MeshID>&)> PrepareRenderingEarly;
				std::function<void(void)> PrepareRenderingLate;

				AssetLoadCreateInfo assetLoadData;
				DebugCreateInfo debugData;


				std::any apiData = nullptr;
			};

			const Data& GetData();

			std::any& GetAPIData();
		}
	}
}