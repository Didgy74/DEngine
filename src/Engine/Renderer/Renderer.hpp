#pragma once

#include "Setup.hpp"
#include "Typedefs.hpp"
#include "OpenGLCreateInfo.hpp"

#include "../Utility/ImgDim.hpp"
#include "../Utility/Color.hpp"

#include "DMath/Vector/Vector.hpp"
#include "DMath/UnitQuaternion.hpp"

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

		namespace Core
		{
			bool Initialize(CreateInfo&& createInfo);
			void PrepareRenderingEarly(RenderGraph& renderGraphInput);
			void PrepareRenderingLate(RenderGraphTransform& sceneData);
			void Draw();
			void Terminate();

			std::any& GetAPIData();

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

	struct Renderer::CreateInfo
	{
		API preferredAPI = API::None;
		Utility::ImgDim surfaceDimensions{};
		void* surfaceHandle = nullptr;
		OpenGL::CreateInfo openGLCreateInfo{};
	};

	struct Renderer::PointLight
	{
		float intensity;
	};

	struct Renderer::RenderGraph
	{
		std::vector<SpriteID> sprites;
		std::vector<MeshID> meshes;
		std::vector<PointLight> pointLights;
	};

	struct Renderer::RenderGraphTransform
	{
		std::vector<Math::Matrix<4, 4, float>> sprites;
		std::vector<Math::Matrix<4, 4, float>> meshes;
		std::vector<Math::Vector<3, float>> pointLights;
	};

	class Renderer::SceneData
	{
	public:
	    SceneData();

        size_t GetSceneID() const;
	private:
		size_t sceneID;
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


