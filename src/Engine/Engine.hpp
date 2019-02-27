#pragma once

#include <vector>
#include <memory>


namespace Engine
{
	class Scene;
	class SceneObject;

	Scene& NewScene();
	std::vector<std::unique_ptr<Scene>>& GetScenes();

	class Core
	{
	public:
		static void Run();
		static void Destroy(SceneObject& sceneObject);
		static void TickEnd(Scene& scene);

		static std::vector<std::unique_ptr<Engine::Scene>> scenes;
	};
}

