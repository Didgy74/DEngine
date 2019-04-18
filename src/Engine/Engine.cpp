#include "Engine.hpp"

#include "Application.hpp"
#include "DEngine/Time/Time.hpp"
#include "DEngine/Input/InputRaw.hpp"
#include "DRenderer/Renderer.hpp"

#include "DEngine/Scene.hpp"
#include "DEngine/SceneObject.hpp"

#include "DEngine/Components/Camera.hpp"
#include "DEngine/Components/FreeLook.hpp"
#include "DEngine/Components/SpriteRenderer.hpp"
#include "DEngine/Components/MeshRenderer.hpp"
#include "DEngine/Components/PointLight.hpp"

#include "SinusMovement.hpp"

#include "Systems/RenderSystem.hpp"

#include "AssManRendererConnect.hpp"

#include "LoadGLTFScene.hpp"

#include <iostream>

std::vector<std::unique_ptr<Engine::Scene>> Engine::Core::scenes;

void rendererDebugCallback(std::string_view message)
{
	std::cout << "Renderer log: " << message << std::endl;
}

namespace Engine
{
	void InitializeRenderer()
	{
		// Initialize renderer
		Renderer::InitInfo rendererInitInfo;

		rendererInitInfo.preferredAPI = Renderer::API::OpenGL;

		// Debug info
		rendererInitInfo.debugInitInfo.useDebugging = true;
		rendererInitInfo.debugInitInfo.errorMessageCallback = rendererDebugCallback;

		rendererInitInfo.surfaceDimensions = Application::GetWindowSize();
		rendererInitInfo.surfaceHandle = Application::Core::GetMainWindowHandle();

		rendererInitInfo.assetLoadCreateInfo.meshLoader = &AssMan::GetRendererMeshDoc;
		rendererInitInfo.assetLoadCreateInfo.assetLoadEnd = &AssMan::Renderer_AssetLoadEndEvent;
		rendererInitInfo.assetLoadCreateInfo.textureLoader = &LoadTexture;

		rendererInitInfo.openGLInitInfo.glSwapBuffers = &Application::Core::GL_SwapWindow;
		Renderer::Core::Initialize(rendererInitInfo);
	}
}

void Engine::Core::Run()
{
	AssetManager::Core::Initialize();
	Application::Core::Initialize(Application::API3D::OpenGL);
	Time::Core::Initialize();
	Input::Core::Initialize();
	InitializeRenderer();


	// Create scene and make viewport 0 point to scene 0.
	Scene& scene1 = Engine::NewScene();
	Renderer::GetViewport(0).SetSceneRef(&scene1);

	auto& objCamera = scene1.NewSceneObject();
	objCamera.localPosition.z = -5.f;
	auto& camera = objCamera.AddComponent<Components::Camera>().second.get();
	camera.zFar = 10000.f;
	objCamera.AddComponent<Components::FreeLook>();

	auto& lightObj = scene1.NewSceneObject();
	lightObj.localPosition = { 0.f, 1.f, 0.f };
	Components::PointLight& light1 = lightObj.AddComponent<Components::PointLight>().second.get();
	light1.color = { 1.f, 1.f, 1.f };
	light1.intensity = 2.f;
	lightObj.AddComponent<Assignment02::SinusMovement>();

	auto& obj4 = scene1.NewSceneObject();
	obj4.localPosition = { 0.f, -7.5f, -10.f };
	//Components::PointLight& light3 = obj4.AddComponent<Components::PointLight>().second.get();


	LoadGLTFScene(scene1, "Data/Sponza2/Sponza.gltf");

	const auto& meshCompVector = scene1.GetAllComponents<Components::MeshRenderer>();

	Renderer::RenderGraph graph;
	Renderer::RenderGraphTransform graphTransform;


	scene1.Scripts_SceneStart();
	while (Application::Core::UpdateEvents(), Application::IsRunning())
	{
		scene1.ScriptTick();

		RenderSystem::BuildRenderGraph(scene1, graph);
		Renderer::Core::PrepareRenderingEarly(graph);

		Renderer::Core::SetCameraInfo(GetRendererCameraInfo(camera));
		RenderSystem::BuildRenderGraphTransform(scene1, graphTransform);
		Renderer::Core::PrepareRenderingLate(graphTransform);

		Renderer::Core::Draw();

		Input::Core::ClearValues();
		Time::Core::TickEnd(scene1.GetTimeData());
	}

	scenes.clear();

	Time::Core::Terminate();
	Input::Core::Terminate();
	Renderer::Core::Terminate();
	Application::Core::Terminate();
	AssetManager::Core::Terminate();
}

void Engine::Core::Destroy(SceneObject& sceneObject)
{
	sceneObject.GetScene().RemoveSceneObject(sceneObject);
}

Engine::Scene& Engine::NewScene() 
{
	Core::scenes.push_back(std::make_unique<Engine::Scene>(Core::scenes.size()));
	return *Core::scenes.back();
}

std::vector<std::unique_ptr<Engine::Scene>>& Engine::GetScenes() { return Core::scenes; }

