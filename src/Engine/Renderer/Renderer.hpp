#pragma once

#include "Setup.hpp"
#include "Typedefs.hpp"
#include "OpenGLCreateInfo.hpp"

#include "../Utility/ImgDim.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/Matrix/Matrix.hpp"

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
			bool Initialize(const InitInfo& createInfo);
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
		using TextureLoaderPFN = std::optional<TextureDocument>(*)(size_t);
		TextureLoaderPFN textureLoader = nullptr;
	};

	struct Renderer::InitInfo
	{
		API preferredAPI = API::None;
		Utility::ImgDim surfaceDimensions{};
		void* surfaceHandle = nullptr;

		AssetLoadCreateInfo assetLoadCreateInfo;
		DebugCreateInfo debugInitInfo;

		OpenGL::InitInfo openGLInitInfo;
	};

	struct Renderer::RenderGraph
	{
		std::vector<SpriteID> sprites;
		std::vector<MeshID> meshes;
		
		Math::Vector3D ambientLight;
		std::vector<Math::Vector<3, float>> pointLights;
	};

	struct Renderer::RenderGraphTransform
	{
		std::vector<Math::Matrix<4, 4, float>> sprites;
		std::vector<Math::Matrix<4, 4, float>> meshes;
		std::vector<Math::Vector<3, float>> pointLights;
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


