#include "AssetManager.hpp"
#include "MeshDocument.hpp"

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

static const std::map<size_t, AssetManagerInfo> textureInfos
{
	{ size_t(Engine::AssetManager::Sprite::Test), {"Test", "test3.ktx"} },
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

		std::string GetTexturePath(size_t i)
		{
			auto iterator = textureInfos.find(i);
			if (iterator == textureInfos.end())
				return {};
			else
				return std::string(textureFolderPath) + std::string(iterator->second.relativePath);
		}
	}
}

#include "MeshDocument.inl"