#include "AssManRendererConnect.hpp"

#include "DEngine/AssetManager/AssetManager.hpp"

#include <cassert>

namespace Engine
{
	Renderer::CameraInfo GetRendererCameraInfo(const Components::Camera& in)
	{
		Renderer::CameraInfo cameraInfo{};

		cameraInfo.fovY = in.fov;
		cameraInfo.orthoWidth = in.orthographicWidth;
		cameraInfo.zNear = in.zNear;
		cameraInfo.zFar = in.zFar;

		cameraInfo.transform = in.GetViewModel(Space::World);

		switch (in.projectionMode)
		{
		case Components::Camera::ProjectionMode::Perspective:
			cameraInfo.projectMode = Renderer::CameraInfo::ProjectionMode::Perspective;
			break;
		case Components::Camera::ProjectionMode::Orthgraphic:
			cameraInfo.projectMode = Renderer::CameraInfo::ProjectionMode::Orthographic;
			break;
		default:
			assert(false && "Invalid Camera Projection Mode.");
		}

		return cameraInfo;
	}

	std::optional<Renderer::MeshDocument> LoadMesh(size_t i)
	{
		auto path = AssMan::GetMeshPath(i);
		if (path == "")
			return {};

		auto assManMeshOpt = AssMan::LoadMeshDocument(path);
		if (assManMeshOpt.has_value() == false)
			return {};

		auto oldInfo = AssMan::MeshDocument::ToCreateInfo(std::move(assManMeshOpt.value()));

		Renderer::MeshDocument::CreateInfo newInfo{};
		newInfo.byteArray = std::move(oldInfo.byteArray);
		newInfo.vertexCount = oldInfo.vertexCount;
		newInfo.indexCount = oldInfo.indexCount;
		newInfo.indexType = oldInfo.indexType == AssMan::MeshDocument::IndexType::UInt16 ? Renderer::MeshDocument::IndexType::UInt16 : Renderer::MeshDocument::IndexType::UInt32;

		newInfo.posByteOffset = oldInfo.posByteOffset;
		newInfo.uvByteOffset = oldInfo.uvByteOffset;
		newInfo.normalByteOffset = oldInfo.normalByteOffset;
		newInfo.tangentByteOffset = oldInfo.tangentByteOffset;
		newInfo.indexByteOffset = oldInfo.indexByteOffset;

		return std::optional<Renderer::MeshDocument>{ Renderer::MeshDocument(std::move(newInfo)) };
	}

	std::optional<DTex::TexDoc> LoadTexture(size_t i)
	{
		auto path = AssMan::GetTexturePath(i);
		if (path == "")
			return {};

		auto loadResult = DTex::LoadFromFile(path);
		if (loadResult.GetResultInfo() != DTex::ResultInfo::Success)
			return {};

		return std::optional<DTex::TexDoc>{ std::move(loadResult.GetValue()) };
	}
}
