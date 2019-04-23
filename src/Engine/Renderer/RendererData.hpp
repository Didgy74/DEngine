#pragma once

#include "DRenderer/Renderer.hpp"

#include <unordered_map>
#include <vector>

namespace DRenderer
{
	namespace Core
	{
		struct APIDataPointer
		{
			void* data = nullptr;
			void(*deleterPfn)(void*&) = nullptr;
		};
	}
}

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

				std::unordered_map<size_t, size_t> meshReferences;
				std::unordered_map<size_t, size_t> textureReferences;

				std::vector<size_t> loadMeshQueue;
				std::vector<size_t> unloadMeshQueue;
				std::vector<size_t> loadTextureQueue;
				std::vector<size_t> unloadTextureQueue;

				std::vector<std::unique_ptr<Viewport>> viewports;

				std::function<void(void)> Draw;
				std::function<void(const std::vector<size_t>&, const std::vector<size_t>&)> PrepareRenderingEarly;
				std::function<void(void)> PrepareRenderingLate;

				AssetLoadCreateInfo assetLoadData{};
				DebugCreateInfo debugData{};

				DRenderer::Core::APIDataPointer apiData{};
			};

			const Data& GetData();

			void* GetAPIData();
		}
	}
}