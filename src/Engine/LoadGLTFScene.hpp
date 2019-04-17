#pragma once

#include <string>
#include <filesystem>

namespace Engine
{
	class Scene;

	bool LoadGLTFScene(Scene& scene, std::filesystem::path path);
}