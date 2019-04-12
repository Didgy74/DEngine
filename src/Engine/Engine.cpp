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

#include "Systems/RenderSystem.hpp"

#include "AssManRendererConnect.hpp"

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

		rendererInitInfo.assetLoadCreateInfo.meshLoader = &LoadMesh;
		rendererInitInfo.assetLoadCreateInfo.textureLoader = &LoadTexture;

		rendererInitInfo.openGLInitInfo.glSwapBuffers = &Application::Core::GL_SwapWindow;
		Renderer::Core::Initialize(rendererInitInfo);
	}
}

void Engine::Core::Run()
{
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
	objCamera.AddComponent<Components::FreeLook>();

	auto& sceneObject1 = scene1.NewSceneObject();
	auto& mesh1 = sceneObject1.AddComponent<Components::MeshRenderer>().second.get();
	mesh1.SetMesh(AssMan::Mesh::Helmet);

	auto& meshTest = sceneObject1.AddComponent<Components::MeshRenderer>().second.get();
	meshTest.SetMesh(AssMan::Mesh::Helmet);
	meshTest.positionOffset.x = -3.f;

	auto& mesh2 = sceneObject1.AddComponent<Components::MeshRenderer>().second.get();
	mesh2.SetMesh(AssMan::Mesh::Cube);
	mesh2.positionOffset.x = 2.f;

	

	auto& lightObj = scene1.NewSceneObject();
	lightObj.localPosition = { 2.5f, 2.5f, 2.5f };
	Components::PointLight& light1 = lightObj.AddComponent<Components::PointLight>().second.get();
	light1.color = { 1.f, 1.f, 1.f };
	auto& mesh3 = lightObj.AddComponent<Components::MeshRenderer>().second.get();
	mesh3.SetMesh(AssMan::Mesh::Cube);
	mesh3.scale = { 0.1f, 0.1f, 0.1f };

	auto& obj4 = scene1.NewSceneObject();
	obj4.localPosition = { 0.f, -7.5f, -10.f };
	Components::PointLight& light3 = obj4.AddComponent<Components::PointLight>().second.get();
	auto& mesh4 = obj4.AddComponent<Components::MeshRenderer>().second.get();
	mesh4.SetMesh(AssMan::Mesh::Cube);
	mesh4.scale = { 0.1f, 0.1f, 0.1f };


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

