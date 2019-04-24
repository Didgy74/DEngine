#pragma once

#include "DRenderer/Renderer.hpp"

#include <unordered_map>
#include <vector>

namespace DRenderer::Core
{
	struct APIDataPointer
	{
		void* data = nullptr;
		void(*deleterPfn)(void*&) = nullptr;
	};

	struct Data
	{
		Engine::Renderer::RenderGraph renderGraph;
		Engine::Renderer::RenderGraphTransform renderGraphTransform;
		Engine::Renderer::CameraInfo cameraInfo;

		Engine::Renderer::API activeAPI = Engine::Renderer::API::None;

		std::unordered_map<size_t, size_t> meshReferences;
		std::unordered_map<size_t, size_t> textureReferences;

		std::vector<size_t> loadMeshQueue;
		std::vector<size_t> unloadMeshQueue;
		std::vector<size_t> loadTextureQueue;
		std::vector<size_t> unloadTextureQueue;

		std::vector<std::unique_ptr<Engine::Renderer::Viewport>> viewports;

		std::function<void(void)> Draw;
		std::function<void(const std::vector<size_t>&, const std::vector<size_t>&)> PrepareRenderingEarly;
		std::function<void(void)> PrepareRenderingLate;

		Engine::Renderer::AssetLoadCreateInfo assetLoadData{};
		Engine::Renderer::DebugCreateInfo debugData{};

		APIDataPointer apiData{};
	};

	const Data& GetData();

	void* GetAPIData();

	// This call is thread-safe if ErrorMessageCallback supplied to InitInfo is.
	void LogDebugMessage(std::string_view message);
}