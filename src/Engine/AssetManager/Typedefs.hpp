#pragma once

#include <cstdint>

namespace Engine
{
	namespace AssetManager
	{
		enum class Sprite : uint32_t;
		enum class Mesh : uint32_t;

		class MeshDocument;
		class TextureDocument;
	}

	namespace AssMan = AssetManager;
}

enum class Engine::AssetManager::Sprite : uint32_t
{
	None,
	Default,
	Test,
	Circle,
#ifdef ASSETMANAGER_TEXTURE_COUNT
	COUNT
#endif
};

enum class Engine::AssetManager::Mesh : uint32_t
{
	None,
	Plane,
	Cube,
	SpritePlane,
	Helmet,
#ifdef ASSETMANAGER_MESH_COUNT
	COUNT
#endif
};