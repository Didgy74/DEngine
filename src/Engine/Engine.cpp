#include "Engine.hpp"

#include "Application.hpp"
#include "Renderer/Renderer.hpp"
#include "Components/SpriteRenderer.hpp"
#include "Components/MeshRenderer.hpp"
#include "Components/PointLight.hpp"
#include "Time/Time.hpp"
#include "Input/InputRaw.hpp"
#include "Scene.hpp"
#include "SceneObject.hpp"

#include "Components/Camera.hpp"

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
	Physics2D::Core::Initialize();

	InitializeRenderer();


	// Create scene and make viewport 0 point to scene 0.
	Scene& scene1 = Engine::NewScene();
	Renderer::GetViewport(0).SetSceneRef(&scene1);




	auto& sceneObject1 = scene1.NewSceneObject();
	auto& mesh1 = sceneObject1.AddComponent<Components::MeshRenderer>().first.get();
	mesh1.SetMesh(AssMan::Mesh::Helmet);

	auto& meshTest = sceneObject1.AddComponent<Components::MeshRenderer>().first.get();
	meshTest.SetMesh(AssMan::Mesh::Helmet);
	meshTest.positionOffset.x = -3.f;

	auto& mesh2 = sceneObject1.AddComponent<Components::MeshRenderer>().first.get();
	mesh2.SetMesh(AssMan::Mesh::Cube);
	mesh2.positionOffset.x = 2.f;

	auto& objCamera = scene1.NewSceneObject();
	auto& camera = objCamera.AddComponent<Components::Camera>().first.get();
	camera.positionOffset.z = 5.f;

	auto& lightObj = scene1.NewSceneObject();
	lightObj.transform.localPosition = { 2.5f, 2.5f, 2.5f };
	Components::PointLight& light1 = lightObj.AddComponent<Components::PointLight>().first.get();
	light1.color = { 1.f, 0.5f, 0.f };
	auto& mesh3 = lightObj.AddComponent<Components::MeshRenderer>().first.get();
	mesh3.SetMesh(AssMan::Mesh::Cube);
	mesh3.scale = { 0.1f, 0.1f, 0.1f };

	auto& obj4 = scene1.NewSceneObject();
	obj4.transform.localPosition = { 0.f, -7.5f, -10.f };
	Components::PointLight& light3 = obj4.AddComponent<Components::PointLight>().first.get();
	auto& mesh4 = obj4.AddComponent<Components::MeshRenderer>().first.get();
	mesh4.SetMesh(AssMan::Mesh::Cube);
	mesh4.scale = { 0.1f, 0.1f, 0.1f };


	Renderer::RenderGraph graph;
	Renderer::RenderGraphTransform graphTransform;


	scene1.Scripts_SceneStart();
	while (Application::Core::UpdateEvents(), Application::IsRunning())
	{
		// Handles origin movement for camera
		const float speed = 2.5f;
		auto cross = Math::Vector3D::Cross(camera.forward, camera.up);
		if (Input::Raw::GetValue(Input::Raw::Button::A))
			camera.positionOffset -= cross * speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::D))
			camera.positionOffset += cross * speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::W))
			camera.positionOffset += camera.forward *  speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::S))
			camera.positionOffset -= camera.forward * speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::Space))
			camera.positionOffset += Math::Vector3D::Up() * speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::LeftCtrl))
			camera.positionOffset += Math::Vector3D::Down() * speed * scene1.GetTimeData().GetDeltaTime();
		camera.LookAt({ -1.f, 0, 0 });

		if (Input::Raw::GetValue(Input::Raw::Button::Up))
			lightObj.transform.localPosition.z -= speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::Down))
			lightObj.transform.localPosition.z += speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::Left))
			lightObj.transform.localPosition.x -= speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::Right))
			lightObj.transform.localPosition.x += speed * scene1.GetTimeData().GetDeltaTime();

		if (Input::Raw::GetEventType(Input::Raw::Button::C) == Input::EventType::Pressed)
		{
			scene1.Clear();
		}


		scene1.ScriptTick();


		RenderSystem::BuildRenderGraph(scene1, graph);
		Renderer::Core::PrepareRenderingEarly(graph);

		Renderer::Core::SetCameraInfo(camera.GetRendererCameraInfo());
		RenderSystem::BuildRenderGraphTransform(scene1, graphTransform);
		Renderer::Core::PrepareRenderingLate(graphTransform);


		Renderer::Core::Draw();


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

