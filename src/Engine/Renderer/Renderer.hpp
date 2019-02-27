#pragma once

#include "Setup.hpp"
#include "Typedefs.hpp"

#include "../Utility/ImgDim.hpp"
#include "../Utility/Color.hpp"

#include "Math/Vector/Vector.hpp"
#include "Math/UnitQuaternion.hpp"

#include <memory>
#include <vector>
#include <map>

namespace Engine
{
	namespace Renderer
	{
		using SceneType = Setup::SceneType;
		enum class API;
		enum class Mode;
		class SceneInfo;
		class Viewport;
		struct Transform;
		struct CameraInfo;

		Viewport& NewViewport(Utility::ImgDim dimensions, void* surfaceHandle);
		const std::vector<std::unique_ptr<Viewport>>& GetViewports();
		size_t GetViewportCount();
		Viewport& GetViewport(size_t index);

		size_t GetMeshReferenceCount(AssetID id);
		void AddMeshReference(AssetID id);
		void RemoveMeshReference(AssetID id);

		size_t GetSpriteReferenceCount(AssetID id);
		void AddSpriteReference(AssetID id);
		void RemoveSpriteReference(AssetID id);

		API GetActiveAPI();

		namespace Core
		{
			bool Initialize(API api, Utility::ImgDim dimensions, void* surfaceHandle);
			void PrepareRendering();
			void Draw();
			void Terminate();

			void* GetAPIData();

			const std::vector<AssetID>& GetMeshLoadQueue();
			const std::vector<AssetID>& GetMeshUnloadQueue();
			const std::vector<AssetID>& GetSpriteLoadQueue();
			const std::vector<AssetID>& GetSpriteUnloadQueue();

			class PrivateAccessor;
		};
	}

	enum class Renderer::API
	{
		None,
		OpenGL,
		Vulkan
	};

	enum class Renderer::Mode
	{
		Shaded,
		Wireframe,
		Flat
	};

	struct Renderer::CameraInfo
	{
		enum class ProjectMode
		{
			Projection,
			Orthogonal
		};

		Math::Matrix4x4 transform;
		ProjectMode projectMode;
		float fovY;
	};

	class Renderer::SceneInfo
	{
	public:
		CameraInfo cameraInfo;

		std::vector<AssetID> meshes;
		std::vector<Math::Matrix4x4> meshTransforms;

		std::vector<AssetID> sprites;
		std::vector<Math::Matrix4x4> spriteTransforms;

		std::vector<Math::Vector2D> particle2Ds;
	};

	class Renderer::Viewport
	{
	public:
		const SceneType* GetScene() const;
		SceneType* GetScene();
		void SetScene(SceneType* scene);
		Utility::ImgDim GetDimensions() const;
		size_t GetCameraIndex() const;
		void* GetSurfaceHandle();
		Mode GetRenderMode() const;
		void SetRenderMode(Mode newRenderMode);
		const SceneInfo& GetRenderInfo() const;
		bool IsRenderInfoValidated() const;
		Math::Matrix4x4 GetCameraModel() const;
		Utility::Color GetWireframeColor() const;

		Viewport(Utility::ImgDim dimensions, void* surfaceHandle);

	private:
		SceneType* sceneRef;
		size_t cameraIndex;
		Utility::ImgDim dimensions;
		void* surfaceHandle;
		Mode renderMode;
		SceneInfo renderInfo;
		bool renderInfoValidated;
		Utility::Color wireframeColor;

		friend class Core::PrivateAccessor;
	};
}


