#pragma once

#include "Setup.hpp"
#include "Typedefs.hpp"
#include "OpenGLCreateInfo.hpp"
#include "VulkanInitInfo.hpp"

#include "Utility/ImgDim.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/Matrix/Matrix.hpp"

#include "DTex/DTex.hpp"

#include <memory>
#include <vector>
#include <map>
#include <any>
#include <array>

namespace Engine
{
	namespace Renderer
	{
		using SceneType = Setup::SceneType;
		
		Viewport& NewViewport(Utility::ImgDim dimensions, void* surfaceHandle);
		size_t GetViewportCount();
		Viewport& GetViewport(size_t index);

		API GetActiveAPI();

		bool IsCompatible(const RenderGraph& renderGraph, const RenderGraphTransform& transforms);

		// This call is thread-safe if ErrorMessageCallback supplied to InitInfo is.
		void LogDebugMessage(std::string_view message);

		namespace Core
		{
			bool Initialize(InitInfo createInfo);
			void PrepareRenderingEarly(RenderGraph& renderGraphInput);
			void PrepareRenderingLate(RenderGraphTransform& sceneData);
			void Draw();
			void Terminate();

			const RenderGraph& GetRenderGraph();
			const RenderGraphTransform& GetRenderGraphTransform();
			const CameraInfo& GetCameraInfo();
			void SetCameraInfo(const CameraInfo& input);
		};
	}

	enum class Renderer::API
	{
		None,
		OpenGL,
		Vulkan
	};

	struct Renderer::DebugCreateInfo
	{
		bool useDebugging = false;
		ErrorMessageCallbackPFN errorMessageCallback = nullptr;
	};

	struct Renderer::AssetLoadCreateInfo
	{
		using MeshLoaderPFN = std::optional<MeshDocument>(*)(size_t);
		MeshLoaderPFN meshLoader = nullptr;

		using TextureLoaderPFN = std::optional<DTex::TexDoc>(*)(size_t);
		TextureLoaderPFN textureLoader = nullptr;

		using AssetLoadEndPFN = void(*)();
		AssetLoadEndPFN assetLoadEnd = nullptr;
	};

	struct Renderer::InitInfo
	{
		API preferredAPI = API::None;
		Utility::ImgDim surfaceDimensions{};
		void* surfaceHandle = nullptr;

		AssetLoadCreateInfo assetLoadCreateInfo;
		DebugCreateInfo debugInitInfo;

		OpenGL::InitInfo openGLInitInfo;
		DRenderer::Vulkan::InitInfo vulkanInitInfo;
	};

	struct Renderer::MeshID
	{
		size_t meshID;
		size_t diffuseID;
	};

	struct Renderer::SpriteID
	{
		size_t spriteID;
	};

	struct Renderer::RenderGraph
	{
		std::vector<SpriteID> sprites;
		std::vector<MeshID> meshes;
		
		std::vector<std::array<float, 3>> pointLightIntensities;
		std::array<float, 3> ambientLight{};
	};

	struct Renderer::RenderGraphTransform
	{
		std::vector<std::array<float, 16>> sprites;
		std::vector<std::array<float, 16>> meshes;

		std::vector<std::array<float, 3>> pointLights;
	};

	struct Renderer::CameraInfo
	{
		enum class ProjectionMode
		{
			Perspective,
			Orthographic
		};

		Math::Matrix4x4 transform;
		Math::Vector3D worldSpacePos;
		ProjectionMode projectMode;
		float fovY;
		float zNear;
		float zFar;
		float orthoWidth;

		Math::Matrix4x4 GetModel(float aspectRatio) const;
	};

	class Renderer::Viewport
	{
	public:
		Viewport(Utility::ImgDim dimensions, void* surfaceHandle);

		void SetSceneRef(const SceneType* scene);
		Utility::ImgDim GetDimensions() const;
		size_t GetCameraIndex() const;
		void* GetSurfaceHandle();

	private:
		const SceneType* sceneRef;
		size_t cameraIndex;
		Utility::ImgDim dimensions;
		void* surfaceHandle;
	};
}


