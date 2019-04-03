#pragma once

#include "AssetManager/MeshDocument.hpp"
#include "AssetManager/TextureDocument.hpp"

#include "Renderer/MeshDocument.hpp"
#include "Renderer/TextureDocument.hpp"

namespace Engine
{
	std::optional<Renderer::MeshDocument> LoadMesh(size_t i);

	std::optional<Renderer::TextureDocument> LoadTexture(size_t i);
}