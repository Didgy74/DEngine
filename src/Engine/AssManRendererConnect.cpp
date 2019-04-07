#include "AssManRendererConnect.hpp"

#include "AssetManager/AssetManager.hpp"

namespace Engine
{
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
}