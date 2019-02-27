#include "Renderer.hpp"
#include "OpenGL.hpp"
#include "Vulkan.hpp"

#include "../Scene.hpp"
#include "../Components/SpriteRenderer.hpp"
#include "../Components/MeshRenderer.hpp"
#include "../Components/Camera.hpp"

#include "Math/LinearTransform3D.hpp"

#include <functional>

namespace Engine
{
	namespace Renderer
	{
		namespace Core
		{
			struct Data
			{
				API activeAPI;

				std::unordered_map<AssetID, size_t> meshReferences;
				std::unordered_map<AssetID, size_t> spriteReferences;

				std::vector<AssetID> loadMeshQueue;
				std::vector<AssetID> unloadMeshQueue;
				std::vector<AssetID> loadSpriteQueue;
				std::vector<AssetID> unloadSpriteQueue;

				std::vector<std::unique_ptr<Viewport>> viewports;

				std::function<void(void*&)> Initialize = nullptr;
				std::function<void(void*&)> Terminate = nullptr;
				std::function<void(void)> Draw = nullptr;
				std::function<void(void)> PrepareRendering = nullptr;

				void* apiData = nullptr;
			};

			static std::unique_ptr<Data> data;

			class PrivateAccessor
			{
			public:
				static void PrepareRenderingEarly();
				static void PrepareRenderingLate();
				static void AfterDrawComplete();
			};
		}
	}
}


Engine::Renderer::Viewport& Engine::Renderer::NewViewport(Utility::ImgDim dimensions, void* surfaceHandle)
{
	Core::data->viewports.emplace_back(std::make_unique<Viewport>(dimensions, surfaceHandle));
	return *Core::data->viewports.back();
}

const std::vector<std::unique_ptr<Engine::Renderer::Viewport>>& Engine::Renderer::GetViewports() { return Core::data->viewports; }

size_t Engine::Renderer::GetViewportCount() { return Core::data->viewports.size(); }

Engine::Renderer::Viewport& Engine::Renderer::GetViewport(size_t index) { return *Core::data->viewports[index]; }

const std::vector<Engine::Renderer::AssetID>& Engine::Renderer::Core::GetMeshLoadQueue() { return data->loadMeshQueue; }

const std::vector<Engine::Renderer::AssetID>& Engine::Renderer::Core::GetMeshUnloadQueue() { return data->unloadMeshQueue; }

const std::vector<Engine::Renderer::AssetID>& Engine::Renderer::Core::GetSpriteLoadQueue() { return data->loadSpriteQueue; }

const std::vector<Engine::Renderer::AssetID>& Engine::Renderer::Core::GetSpriteUnloadQueue() { return data->unloadSpriteQueue; }

size_t Engine::Renderer::GetMeshReferenceCount(AssetID id)
{
	auto iterator = Core::data->meshReferences.find(id);
	if (iterator != Core::data->meshReferences.end())
		return iterator->second;
	else
		return 0;
}

void Engine::Renderer::AddMeshReference(AssetID id)
{ 
	auto newIterator = Core::data->meshReferences.find(id);
	if (newIterator != Core::data->meshReferences.end())
		newIterator->second++;
	else
	{
		Core::data->meshReferences.insert({ id, 1 });
		Core::data->loadMeshQueue.push_back(id);
	}
}

void Engine::Renderer::RemoveMeshReference(AssetID id)
{
	auto oldIterator = Core::data->meshReferences.find(id);
	assert(oldIterator != Core::data->meshReferences.end());
	oldIterator->second--;
	if (oldIterator->second == 0)
	{
		Core::data->unloadMeshQueue.push_back(id);
		Core::data->meshReferences.erase(oldIterator);
	}
}

size_t Engine::Renderer::GetSpriteReferenceCount(AssetID id)
{
	auto iterator = Core::data->spriteReferences.find(id);
	if (iterator != Core::data->spriteReferences.end())
		return iterator->second;
	else
		return 0;
}

void Engine::Renderer::AddSpriteReference(AssetID id)
{
	auto newIterator = Core::data->spriteReferences.find(id);
	if (newIterator != Core::data->spriteReferences.end())
		newIterator->second++;
	else
	{
		Core::data->spriteReferences.insert({ id, 1 });
		Core::data->loadSpriteQueue.push_back(id);
	}

	AddMeshReference(static_cast<AssetID>(Asset::Mesh::SpritePlane));
}

void Engine::Renderer::RemoveSpriteReference(AssetID id)
{ 
	auto oldIterator = Core::data->spriteReferences.find(id);
	assert(oldIterator != Core::data->spriteReferences.end());
	oldIterator->second--;
	if (oldIterator->second == 0)
	{
		Core::data->unloadSpriteQueue.push_back(id);
		Core::data->spriteReferences.erase(oldIterator);
	}

	RemoveMeshReference(static_cast<AssetID>(Asset::Mesh::SpritePlane));
}

Engine::Renderer::API Engine::Renderer::GetActiveAPI() { return Core::data->activeAPI; }

void* Engine::Renderer::Core::GetAPIData() { return data->apiData; }

bool Engine::Renderer::Core::Initialize(API api, Utility::ImgDim dimensions, void* surfaceHandle)
{
	data = std::make_unique<Data>();
	data->activeAPI = api;

	NewViewport(dimensions, surfaceHandle);

	switch (data->activeAPI)
	{
	case API::Vulkan:
		data->Initialize = Vulkan::Initialize;
		data->Terminate = Vulkan::Terminate;
		data->Draw = Vulkan::Draw;
		data->PrepareRendering = Vulkan::PrepareRendering;
		break;
	case API::OpenGL:
		data->Initialize = OpenGL::Initialize;
		data->Terminate = OpenGL::Terminate;
		data->Draw = OpenGL::Draw;
		data->PrepareRendering = OpenGL::PrepareRendering;
		break;
	}

	data->Initialize(data->apiData);

	return true;
}

void Engine::Renderer::Core::Terminate()
{
	data->Terminate(data->apiData);

	data = nullptr;
}

void Engine::Renderer::Core::PrivateAccessor::PrepareRenderingEarly()
{
	auto& viewports = data->viewports;
	for (auto& item : viewports)
	{
		Viewport& viewport = *item;
		if (viewport.GetScene() == nullptr)
			continue;

		const SceneType& scene = *viewport.GetScene();

		if (scene.RenderSceneValid() == false)
			viewport.renderInfoValidated = false;

		const auto& spritesOpt = scene.GetComponents<Engine::SpriteRenderer>();
		if (spritesOpt)
		{
			const auto& sprites = *spritesOpt;
			viewport.renderInfo.sprites.resize(sprites.size());
			viewport.renderInfo.spriteTransforms.resize(sprites.size());
			for (size_t i = 0; i < sprites.size(); i++)
				viewport.renderInfo.sprites[i] = static_cast<AssetID>(*sprites[i]);
		}
		

		const auto& meshesOpt = scene.GetComponents<Engine::MeshRenderer>();
		if (meshesOpt)
		{
			const auto& meshes = *meshesOpt;
			viewport.renderInfo.meshes.resize(meshes.size());
			viewport.renderInfo.meshTransforms.resize(meshes.size());
			for (size_t i = 0; i < meshes.size(); i++)
				viewport.renderInfo.meshes[i] = static_cast<AssetID>(meshes[i]->GetMeshID());
		}
	}
}

void Engine::Renderer::Core::PrivateAccessor::PrepareRenderingLate()
{
	auto& viewports = data->viewports;
	for (auto& item : viewports)
	{
		Viewport& viewport = *item;
		if (viewport.GetScene() == nullptr)
			continue;
		const SceneType& scene = *viewport.GetScene();

		// Sprites
		const auto& spritesOpt = scene.GetComponents<Engine::SpriteRenderer>();
		if (spritesOpt)
		{
			const auto& sprites = *spritesOpt;
			for (size_t i = 0; i < sprites.size(); i++)
				viewport.renderInfo.spriteTransforms[i] = sprites[i]->GetModel(Engine::Space::World);
		}
		


		// Meshes
		const auto& meshOpt = scene.GetComponents<Engine::MeshRenderer>();
		if (meshOpt)
		{
			const auto& meshes = *meshOpt;
			//for (size_t i = 0; i < meshes.size(); i++)
				//viewport.renderInfo.meshTransforms[i] = meshes[i].
		}

		const auto& camerasOpt = scene.GetComponents<Engine::Camera>();
		assert(camerasOpt);

		const auto& cameras = *camerasOpt;
		viewport.renderInfo.cameraInfo = static_cast<Renderer::CameraInfo>(*cameras[viewport.GetCameraIndex()]);
	}
}

void Engine::Renderer::Core::PrivateAccessor::AfterDrawComplete()
{
	data->loadMeshQueue.clear();
	data->unloadMeshQueue.clear();
	data->loadSpriteQueue.clear();
	data->unloadSpriteQueue.clear();

	for (auto& item : data->viewports)
	{
		Viewport& viewport = *item;
		viewport.renderInfoValidated = true;
	}
}

void Engine::Renderer::Core::PrepareRendering()
{
	Core::PrivateAccessor::PrepareRenderingEarly();
	
	data->PrepareRendering();
}

void Engine::Renderer::Core::Draw()
{
	Core::PrivateAccessor::PrepareRenderingLate();

	data->Draw();

	Core::PrivateAccessor::AfterDrawComplete();
}

const Engine::Renderer::SceneInfo& Engine::Renderer::Viewport::GetRenderInfo() const { return renderInfo; }

bool Engine::Renderer::Viewport::IsRenderInfoValidated() const { return renderInfoValidated; }

Math::Matrix4x4 Engine::Renderer::Viewport::GetCameraModel() const
{
	const CameraInfo& cameraInfo = renderInfo.cameraInfo;
	switch (GetActiveAPI())
	{
	case API::Vulkan:
		return Math::LinearTransform3D::PerspectiveRH_ZO(cameraInfo.fovY, GetDimensions().AspectRatio(), 0.1f, 100.f) * cameraInfo.transform;
	case API::OpenGL:
		return Math::LinearTransform3D::PerspectiveRH_NO(cameraInfo.fovY, GetDimensions().AspectRatio(), 0.1f, 100.f) * cameraInfo.transform;
	default:
		return {};
	}
}

Utility::Color Engine::Renderer::Viewport::GetWireframeColor() const { return wireframeColor; }

Engine::Renderer::Viewport::Viewport(Utility::ImgDim dimensions, void* surfaceHandle) :
	sceneRef(nullptr),
	cameraIndex(0),
	dimensions(dimensions),
	surfaceHandle(surfaceHandle),
	renderMode(Mode::Shaded),
	renderInfoValidated(false),
	wireframeColor(Utility::Color::White())
{
	
}

const Engine::Renderer::SceneType* Engine::Renderer::Viewport::GetScene() const { return sceneRef; }

Engine::Renderer::SceneType* Engine::Renderer::Viewport::GetScene() { return sceneRef; }

void Engine::Renderer::Viewport::SetScene(SceneType* scene)
{
	sceneRef = scene;
}

Utility::ImgDim Engine::Renderer::Viewport::GetDimensions() const { return dimensions; }

size_t Engine::Renderer::Viewport::GetCameraIndex() const { return cameraIndex; }

void* Engine::Renderer::Viewport::GetSurfaceHandle() { return surfaceHandle; }

Engine::Renderer::Mode Engine::Renderer::Viewport::GetRenderMode() const { return renderMode; }

void Engine::Renderer::Viewport::SetRenderMode(Mode newRenderMode)
{
	if (newRenderMode == GetRenderMode())
		return;

	renderInfoValidated = false;
}