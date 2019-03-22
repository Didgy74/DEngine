#include "Renderer.hpp"
#include "OpenGL.hpp"

#include "../Scene.hpp"
#include "../Components/SpriteRenderer.hpp"
#include "../Components/MeshRenderer.hpp"
#include "../Components/Camera.hpp"

#include "DMath/LinearTransform3D.hpp"

#include <functional>
#include <iostream>

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

			    size_t sceneIDCounter = 0;

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

				std::any apiData = nullptr;
			};

			static std::unique_ptr<Data> data;

			void UpdateAssetReferences(Data& data, const RenderGraph& oldRG, const RenderGraph* newRG);
		}
	}
}

Engine::Renderer::Viewport& Engine::Renderer::NewViewport(Utility::ImgDim dimensions, void* surfaceHandle)
{
	Core::data->viewports.emplace_back(std::make_unique<Viewport>(dimensions, surfaceHandle));
	return *Core::data->viewports.back();
}

size_t Engine::Renderer::GetViewportCount() { return Core::data->viewports.size(); }

Engine::Renderer::Viewport& Engine::Renderer::GetViewport(size_t index) { return *Core::data->viewports[index]; }

Engine::Renderer::API Engine::Renderer::GetActiveAPI() { return Core::data->activeAPI; }

std::any& Engine::Renderer::Core::GetAPIData() { return data->apiData; }

bool Engine::Renderer::Core::Initialize(CreateInfo&& createInfo)
{
	data = std::make_unique<Data>();
	data->activeAPI = createInfo.preferredAPI;

	NewViewport(createInfo.surfaceDimensions, createInfo.surfaceHandle);

	switch (data->activeAPI)
	{
	case API::OpenGL:
		OpenGL::Initialize(data->apiData, std::move(createInfo.openGLCreateInfo));
		data->Draw = OpenGL::Draw;
		data->PrepareRenderingEarly = OpenGL::PrepareRenderingEarly;
		data->PrepareRenderingLate = OpenGL::PrepareRenderingLate;
		break;
	default:
		break;
	}

	return true;
}

void Engine::Renderer::Core::Terminate()
{
	switch (GetActiveAPI())
	{
	case API::OpenGL:
		OpenGL::Terminate(data->apiData);
		break;
	default:
		break;
	}

	data = nullptr;
}

void Engine::Renderer::Core::PrepareRenderingEarly(RenderGraph& renderGraphInput)
{
    auto& renderGraph = data->renderGraph;

	UpdateAssetReferences(*data, renderGraph, &renderGraphInput);

	std::swap(renderGraph, renderGraphInput);

	if (!data->loadSpriteQueue.empty() || !data->loadMeshQueue.empty())
		std::cout << "Loading sprite/mesh resource(s)..." << std::endl;
	data->PrepareRenderingEarly(data->loadSpriteQueue, data->loadMeshQueue);

	data->loadSpriteQueue.clear();
	data->unloadSpriteQueue.clear();
	data->loadMeshQueue.clear();
	data->unloadMeshQueue.clear();
}

void Engine::Renderer::Core::PrepareRenderingLate(RenderGraphTransform &input)
{
	const auto& renderGraph = data->renderGraph;
	auto& renderGraphTransform = data->renderGraphTransform;

	assert(IsCompatible(renderGraph, input));

	std::swap(renderGraphTransform, input);
}

void Engine::Renderer::Core::Draw()
{
	data->Draw();
}

const Engine::Renderer::RenderGraph &Engine::Renderer::Core::GetRenderGraph()
{
	return data->renderGraph;
}

const Engine::Renderer::RenderGraphTransform &Engine::Renderer::Core::GetRenderGraphTransform()
{
	return data->renderGraphTransform;
}

const Engine::Renderer::CameraInfo &Engine::Renderer::Core::GetCameraInfo()
{
	return data->cameraInfo;
}

void Engine::Renderer::Core::SetCameraInfo(const CameraInfo &input)
{
	data->cameraInfo = input;
}

Engine::Renderer::SceneData::SceneData()
{
    sceneID = Core::data->sceneIDCounter;
    Core::data->sceneIDCounter++;
}

Engine::Renderer::Viewport::Viewport(Utility::ImgDim dimensions, void* surfaceHandle) :
	sceneRef(nullptr),
	cameraIndex(0),
	dimensions(dimensions),
	surfaceHandle(surfaceHandle)
{
}

Utility::ImgDim Engine::Renderer::Viewport::GetDimensions() const { return dimensions; }

size_t Engine::Renderer::Viewport::GetCameraIndex() const { return cameraIndex; }

void* Engine::Renderer::Viewport::GetSurfaceHandle() { return surfaceHandle; }

void Engine::Renderer::Viewport::SetSceneRef(const Engine::Renderer::SceneType* scene)
{
	sceneRef = scene;
}

bool Engine::Renderer::IsCompatible(const RenderGraph &renderGraph, const RenderGraphTransform &transforms)
{
	if (renderGraph.sprites.size() != transforms.sprites.size())
		return false;

	if (renderGraph.meshes.size() != transforms.meshes.size())
		return false;

	return true;
}

void Engine::Renderer::Core::UpdateAssetReferences(Data& data, const RenderGraph& oldRG, const RenderGraph* newRG)
{
	// Add new references
	if (newRG)
	{
		// Sprites
		for(const auto& item : newRG->sprites)
		{
			auto& referenceCount = data.spriteReferences[item];
			referenceCount++;
			if (referenceCount == 1)
				data.loadSpriteQueue.emplace_back(item);
		}

		// Meshes
		for (const auto& item : newRG->meshes)
		{
			auto& referenceCount = data.meshReferences[item];
			referenceCount++;
			if (referenceCount == 1)
				data.loadMeshQueue.emplace_back(item);
		}
	}

	// Remove existing references
	for (const auto& item : oldRG.sprites)
	{
		auto iterator = data.spriteReferences.find(item);
		iterator->second--;
		if (iterator->second <= 0)
		{
			data.unloadSpriteQueue.emplace_back(item);
			data.spriteReferences.erase(iterator);
		}
	}

	for (const auto& item : oldRG.meshes)
	{
		auto iterator = data.meshReferences.find(item);
		iterator->second--;
		if (iterator->second <= 0)
		{
			data.unloadMeshQueue.emplace_back(item);
			data.meshReferences.erase(iterator);
		}
	}
}

Math::Matrix4x4 Engine::Renderer::CameraInfo::GetModel(float aspectRatio) const
{
	if (projectMode == ProjectMode::Perspective)
	{
		switch (GetActiveAPI())
		{
			case API::OpenGL:
				return Math::LinTran3D::Perspective<float>(Math::API3D::OpenGL, fovY, aspectRatio, zNear, zFar) *  transform;
			case API::Vulkan:
				return Math::LinTran3D::Perspective<float>(Math::API3D::Vulkan, fovY, aspectRatio, zNear, zFar) * transform;
			default:
				assert(false);
				return {};
		}
	}
	else // Orthogonal
	{
		assert(false);
		return {};
	}
}