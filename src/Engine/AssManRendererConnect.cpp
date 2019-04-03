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

	std::optional<Renderer::TextureDocument> LoadTexture(size_t i)
	{
		auto path = AssMan::GetTexturePath(i);
		if (path == "")
			return {};

		auto assManTexOpt = AssMan::LoadTextureDocument(path);
		if (assManTexOpt.has_value() == false)
			return {};

		auto oldInfo = AssMan::TextureDocument::ToCreateInfo(std::move(assManTexOpt.value()));

		Renderer::TextureDocument::CreateInfo newInfo{};

		newInfo.byteArray = std::move(oldInfo.byteArray);

		newInfo.baseInternalFormat = Renderer::TextureDocument::Format(oldInfo.baseInternalFormat);

		newInfo.type = Renderer::TextureDocument::Type(oldInfo.type);

		newInfo.numDimensions = oldInfo.numDimensions;

		newInfo.dimensions = oldInfo.dimensions;

		newInfo.numLayers = oldInfo.numLayers;

		newInfo.numLevels = oldInfo.numLevels;

		return std::optional<Renderer::TextureDocument>{ Renderer::TextureDocument(std::move(newInfo)) };
	}
}
