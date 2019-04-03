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

			bool IsValid(const InitInfo& createInfo, ErrorMessageCallbackPFN callback);
		}
	}

	bool Renderer::Core::IsValid(const InitInfo& createInfo, ErrorMessageCallbackPFN callback)
	{
		const auto& DebugMessage = [callback](const char* message)
		{
			if (callback)
				callback(message);
		};

		if (createInfo.surfaceHandle == nullptr)
		{
			DebugMessage("Error. InitInfo needs a window/surface handle.");
			return false;
		}

		if (createInfo.surfaceDimensions[0] == 0 || createInfo.surfaceDimensions[1] == 1)
		{
			DebugMessage("Error. InitInfo::surfaceDimensions can't have any dimension(s) with value 0.");
			return false;
		}

		// Asset load stuff
		if (createInfo.assetLoadCreateInfo.meshLoader == nullptr)
		{
			DebugMessage("Error. MeshLoader pointer in initialization cannot be nullptr.");
			return false;
		}

		// Debug stuff
		if (createInfo.debugInitInfo.useDebugging == true)
		{
			const DebugCreateInfo& debugInfo = createInfo.debugInitInfo;
			if (debugInfo.errorMessageCallback == nullptr)
			{
				DebugMessage("Error. DebugCreateInfo::errorMessageCallback cannot be nullptr when DebugCreateInfo::enableDebug is true.");
				return false;
			}
		}

		switch (createInfo.preferredAPI)
		{
		case API::OpenGL:
			if (IsValid(createInfo.openGLInitInfo, createInfo.debugInitInfo.errorMessageCallback) == true)
				break;
			else
				return false;
		default:
			DebugMessage("Error. InitInfo::preferredAPI can't be set to 'API::None'");
			return false;
		}

		return true;
	}

	bool Renderer::OpenGL::IsValid(const InitInfo& initInfo, ErrorMessageCallbackPFN callback)
	{
		const auto& DebugMessage = [callback](const char* message)
		{
			if (callback)
				callback(message);
		};

		if (initInfo.glSwapBuffers == nullptr)
		{
			DebugMessage("Error. Renderer::OpenGL::InitInfo must point to a valid function.");
			return false;
		}

		return true;
	}

	const Renderer::Core::Data& Engine::Renderer::Core::GetData()
	{
		return *data;
	}

	Renderer::Viewport& Engine::Renderer::NewViewport(Utility::ImgDim dimensions, void* surfaceHandle)
	{
		Core::data->viewports.emplace_back(std::make_unique<Viewport>(dimensions, surfaceHandle));
		return *Core::data->viewports.back();
	}

	size_t Renderer::GetViewportCount() { return Core::data->viewports.size(); }

	Renderer::Viewport& Renderer::GetViewport(size_t index) { return *Core::data->viewports[index]; }

	Renderer::API Renderer::GetActiveAPI()
	{
		if (Core::data)
			return Core::data->activeAPI;
		else
			return API::None;
	}

	std::any& Renderer::Core::GetAPIData() { return data->apiData; }

	bool Renderer::Core::Initialize(const InitInfo& createInfo)
	{
		if constexpr (Setup::enableDebugging)
		{
			if (createInfo.debugInitInfo.useDebugging)
			{
				bool valid = IsValid(createInfo, createInfo.debugInitInfo.errorMessageCallback);
				if (!valid)
					LogDebugMessage("Error. InitInfo is not valid.");
				assert(valid);
			}
		}

		Core::data = std::make_unique<Data>();
		Data& data = *Core::data;

		// Debug info
		if constexpr (Setup::enableDebugging)
		{
			data.debugData = createInfo.debugInitInfo;
		}

		data.activeAPI = createInfo.preferredAPI;

		data.assetLoadData = createInfo.assetLoadCreateInfo;

		NewViewport(createInfo.surfaceDimensions, createInfo.surfaceHandle);

		switch (data.activeAPI)
		{
		case API::OpenGL:
			OpenGL::Initialize(data.apiData, createInfo.openGLInitInfo);
			data.Draw = OpenGL::Draw;
			data.PrepareRenderingEarly = OpenGL::PrepareRenderingEarly;
			data.PrepareRenderingLate = OpenGL::PrepareRenderingLate;
			break;
		default:
			break;
		}

		return true;
	}

	void Renderer::Core::Terminate()
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

	void Renderer::Core::PrepareRenderingEarly(RenderGraph& renderGraphInput)
	{
		Data& data = *Core::data;

		auto& renderGraph = data.renderGraph;

		UpdateAssetReferences(data, renderGraph, &renderGraphInput);

		std::swap(renderGraph, renderGraphInput);

		if (!data.loadSpriteQueue.empty() || !data.loadMeshQueue.empty())
			LogDebugMessage("Loading sprite/mesh resource(s)...");
		data.PrepareRenderingEarly(data.loadSpriteQueue, data.loadMeshQueue);

		data.loadSpriteQueue.clear();
		data.unloadSpriteQueue.clear();
		data.loadMeshQueue.clear();
		data.unloadMeshQueue.clear();
	}

	void Renderer::Core::PrepareRenderingLate(RenderGraphTransform &input)
	{
		Data& data = *Core::data;

		const auto& renderGraph = data.renderGraph;
		auto& renderGraphTransform = data.renderGraphTransform;

		if constexpr (Setup::enableDebugging)
		{
			if (data.debugData.useDebugging)
			{
				bool compatible = IsCompatible(renderGraph, input);
				if (!compatible)
				{
					LogDebugMessage("Error. Newly submitted RenderGraphTransform is not compatible with previously submitted RenderGraph.");
					assert(false);
				}
			}
		}

		std::swap(renderGraphTransform, input);

		data.PrepareRenderingLate();
	}

	void Renderer::Core::Draw()
	{
		data->Draw();
	}

	const Renderer::RenderGraph &Renderer::Core::GetRenderGraph()
	{
		return data->renderGraph;
	}

	const Renderer::RenderGraphTransform &Renderer::Core::GetRenderGraphTransform()
	{
		return data->renderGraphTransform;
	}

	const Renderer::CameraInfo &Renderer::Core::GetCameraInfo()
	{
		return data->cameraInfo;
	}

	void Renderer::Core::SetCameraInfo(const CameraInfo &input)
	{
		data->cameraInfo = input;
	}

	Renderer::Viewport::Viewport(Utility::ImgDim dimensions, void* surfaceHandle) :
		sceneRef(nullptr),
		cameraIndex(0),
		dimensions(dimensions),
		surfaceHandle(surfaceHandle)
	{
	}

	Utility::ImgDim Renderer::Viewport::GetDimensions() const { return dimensions; }

	size_t Renderer::Viewport::GetCameraIndex() const { return cameraIndex; }

	void* Renderer::Viewport::GetSurfaceHandle() { return surfaceHandle; }

	void Renderer::Viewport::SetSceneRef(const Renderer::SceneType* scene)
	{
		sceneRef = scene;
	}

	bool Renderer::IsCompatible(const RenderGraph &renderGraph, const RenderGraphTransform &transforms)
	{
		if (renderGraph.sprites.size() != transforms.sprites.size())
			return false;

		if (renderGraph.meshes.size() != transforms.meshes.size())
			return false;

		if (renderGraph.pointLights.size() != transforms.pointLights.size())
			return false;

		return true;
	}

	void Renderer::LogDebugMessage(std::string_view message)
	{
		if constexpr (Setup::enableDebugging)
		{
			const Core::Data& data = *Core::data;
			if (data.debugData.useDebugging)
				data.debugData.errorMessageCallback(message);
		}
	}

	void Renderer::Core::UpdateAssetReferences(Data& data, const RenderGraph& oldRG, const RenderGraph* newRG)
	{
		// Add new references
		if (newRG)
		{
			// Sprites
			for (const auto& item : newRG->sprites)
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

	Math::Matrix4x4 Renderer::CameraInfo::GetModel(float aspectRatio) const
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
}

#include "MeshDocument.inl"
#include "TextureDocument.inl"