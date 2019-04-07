#pragma once

#include "AssetManager/MeshDocument.hpp"

#include "Renderer/MeshDocument.hpp"

namespace Engine
{
	std::optional<Renderer::MeshDocument> LoadMesh(size_t i);
}