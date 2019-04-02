#include "AssetManager.hpp"
#include "MeshDocument.hpp"
#include "TextureDocument.hpp"

#include <cassert>
#include <map>
#include <type_traits>
#include <iostream>
#include <string_view>

struct AssetManagerInfo
{
	std::string_view name;
	std::string_view relativePath;
};

static const std::map<Engine::AssetManager::Sprite, AssetManagerInfo> textureInfos
{
	{ Engine::AssetManager::Sprite::None, {"None" , ""} },
	{ Engine::AssetManager::Sprite::Default, {"Default", "defaultTexture.png"} },
	{ Engine::AssetManager::Sprite::Test, {"Test", "test.png"} },
	{ Engine::AssetManager::Sprite::Circle, {"Circle", "circle.png"} },
};

static const std::map<size_t, AssetManagerInfo> meshInfos
{
	{ size_t(Engine::AssetManager::Mesh::None), {"None", ""} },
	{ size_t(Engine::AssetManager::Mesh::Cube), {"Cube", "Cube/Cube.gltf"} },
	{ size_t(Engine::AssetManager::Mesh::SpritePlane), {"SpritePlane", "SpritePlane/SpritePlane.gltf"} },
	{ size_t(Engine::AssetManager::Mesh::Helmet), {"Helmet", "Helmet/Helmet.gltf"} },
};

namespace Engine
{
	namespace AssetManager
	{
		std::string GetMeshPath(size_t i)
		{
			auto iterator = meshInfos.find(i);
			if (iterator == meshInfos.end())
				return {};
			else
				return std::string(meshFolderPath) + std::string(iterator->second.relativePath);
		}

		std::optional<Renderer::MeshDocument> AssetManager::LoadMesh(size_t i)
		{
			auto path = GetMeshPath(i);
			if (path == "")
				return {};

			auto AssetManagerMeshDocOpt = LoadMeshDocument(path);
			if (AssetManagerMeshDocOpt.has_value() == false)
				return {};

			auto oldInfo = MeshDocument::ToCreateInfo(std::move(AssetManagerMeshDocOpt.value()));

			Renderer::MeshDocument::CreateInfo newInfo;
			newInfo.byteArray = std::move(oldInfo.byteArray);
			newInfo.vertexCount = std::move(oldInfo.vertexCount);
			newInfo.indexCount = std::move(oldInfo.indexCount);
			newInfo.indexType = oldInfo.indexType == MeshDocument::IndexType::UInt16 ? Renderer::MeshDocument::IndexType::UInt16 : Renderer::MeshDocument::IndexType::UInt32;

			newInfo.posByteOffset = std::move(oldInfo.posByteOffset);
			newInfo.uvByteOffset = std::move(oldInfo.uvByteOffset);
			newInfo.normalByteOffset = std::move(oldInfo.normalByteOffset);
			newInfo.tangentByteOffset = std::move(oldInfo.tangentByteOffset);
			newInfo.indexByteOffset = std::move(oldInfo.indexByteOffset);

			return { Renderer::MeshDocument(std::move(newInfo)) };
		}
	}
	
}

#include "MeshDocument.inl"
#include "TextureDocument.inl"