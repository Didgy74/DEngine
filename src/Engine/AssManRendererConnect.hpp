#pragma once

#include "DEngine/Components/Camera.hpp"

#include "DRenderer/MeshDocument.hpp"
#include "DRenderer/Renderer.hpp"

#include "DTex/DTex.hpp"

namespace Engine
{
	Renderer::CameraInfo GetRendererCameraInfo(const Components::Camera& in);

	std::optional<DTex::TexDoc> LoadTexture(size_t i);
}