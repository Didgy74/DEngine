#pragma once

#include <string>
#include <filesystem>

namespace Engine
{
	class SceneObject;

	bool LoadGLTFScene(SceneObject& scene, std::filesystem::path path);
}