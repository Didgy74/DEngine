#include "Renderer.hpp"
#include "RendererData.hpp"

#include "OpenGL.hpp"

#include "DMath/LinearTransform3D.hpp"

#include <functional>

namespace Engine
{
	namespace Renderer
	{
		namespace Core
		{
			static std::unique_ptr<Data> data;

			void UpdateAssetReferences(Data& data, const RenderGraph& oldRG, const RenderGraph* newRG);
		}
	}
}

const Engine::Renderer::Core::Data& Engine::Renderer::Core::GetData()
{
	return *data;
}

Engine::Renderer::Viewport& Engine::Renderer::NewViewport(Utility::ImgDim dimensions, void* surfaceHandle)
{
	Core::data->viewports.emplace_back(std::make_unique<Viewport>(dimensions, surfaceHandle));
	return *Core::data->viewports.back();
}

size_t Engine::Renderer::GetViewportCount() { return Core::data->viewports.size(); }

Engine::Renderer::Viewport& Engine::Renderer::GetViewport(size_t index) { return *Core::data->viewports[index]; }

Engine::Renderer::API Engine::Renderer::GetActiveAPI()
{
	if (Core::data)
		return Core::data->activeAPI;
	else
		return API::None;
}

std::any& Engine::Renderer::Core::GetAPIData() { return data->apiData; }

bool Engine::Renderer::Core::Initialize(CreateInfo&& createInfo)
{
	Core::data = std::make_unique<Data>();
	Data& data = *Core::data;

	data.activeAPI = createInfo.preferredAPI;

	data.debugData = std::move(createInfo.debugCreateInfo);

	NewViewport(createInfo.surfaceDimensions, createInfo.surfaceHandle);

	switch (data.activeAPI)
	{
	case API::OpenGL:
		OpenGL::Initialize(data.apiData, std::move(createInfo.openGLCreateInfo));
		data.Draw = OpenGL::Draw;
		data.PrepareRenderingEarly = OpenGL::PrepareRenderingEarly;
		data.PrepareRenderingLate = OpenGL::PrepareRenderingLate;
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
	Data& data = *Core::data;

    auto& renderGraph = data.renderGraph;

	UpdateAssetReferences(data, renderGraph, &renderGraphInput);

	std::swap(renderGraph, renderGraphInput);

	if (!data.loadSpriteQueue.empty() || !data.loadMeshQueue.empty())
		data.debugData.errorMessageCallback("Loading sprite/mesh resource(s)...");
	data.PrepareRenderingEarly(data.loadSpriteQueue, data.loadMeshQueue);

	data.loadSpriteQueue.clear();
	data.unloadSpriteQueue.clear();
	data.loadMeshQueue.clear();
	data.unloadMeshQueue.clear();
}

void Engine::Renderer::Core::PrepareRenderingLate(RenderGraphTransform &input)
{
	const auto& renderGraph = data->renderGraph;
	auto& renderGraphTransform = data->renderGraphTransform;

	assert(IsCompatible(renderGraph, input));

	std::swap(renderGraphTransform, input);

	data->PrepareRenderingLate();
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

	if (renderGraph.pointLights.size() != transforms.pointLights.size())
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
	using namespace Math::LinTran3D;
	if (projectMode == ProjectionMode::Perspective)
	{
		switch (GetActiveAPI())
		{
			case API::OpenGL:
				return Perspective<float>(Math::API3D::OpenGL, fovY, aspectRatio, zNear, zFar) * transform;
			case API::Vulkan:
				return Perspective<float>(Math::API3D::Vulkan, fovY, aspectRatio, zNear, zFar) * transform;
			default:
				assert(false);
				return {};
		}
	}
	else if (projectMode == ProjectionMode::Orthographic)
	{
		const float& right = orthoWidth / 2;
		const float& left = -right;
		const float& top = orthoWidth / aspectRatio / 2;
		const float& bottom = -top;
		switch (GetActiveAPI())
		{
			case API::OpenGL:
				return Orthographic<float>(Math::API3D::OpenGL, left, right, bottom, top, zNear, zFar);
			case API::Vulkan:
				return Orthographic<float>(Math::API3D::Vulkan, left, right, bottom, top, zNear, zFar);
			default:
				assert(false);
				return {};
		}
	}

	// Function should NOT reach here.
	assert(false);
	return {};
}