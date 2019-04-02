#pragma once

#include "Typedefs.hpp"
#include "MeshDocument.hpp"
#include "TextureDocument.hpp"

#include <string>
#include <string_view>
#include <optional>

#include "../Renderer/MeshDocument.hpp"
#include "../Renderer/TextureDocument.hpp"

namespace Engine
{
	namespace AssetManager
	{
		constexpr std::string_view textureFolderPath = "Data/Textures/";
		constexpr std::string_view meshFolderPath = "Data/Meshes/";

		std::string GetMeshPath(size_t i);
		std::string GetTexturePath(size_t i);

		std::optional<Renderer::MeshDocument> LoadMesh(size_t i);
		std::optional<Renderer::TextureDocument> LoadTexture(size_t i);
	}

	namespace AssMan = AssetManager;
}