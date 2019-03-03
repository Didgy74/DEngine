#include "Engine.hpp"

#include "Physics2D.hpp"

#include "Application.hpp"
#include "Renderer/Renderer.hpp"
#include "Components/SpriteRenderer.hpp"
#include "Time.hpp"
#include "Input/InputRaw.hpp"

#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"

#include <chrono>

#include <iostream>

std::vector<std::unique_ptr<Engine::Scene>> Engine::Core::scenes;

void Engine::Core::Run()
{
	Application::Core::Initialize(Application::API3D::OpenGL);
	Time::Core::Initialize();
	Input::Core::Initialize();
	Physics2D::Core::Initialize();
	Renderer::Core::Initialize(Renderer::API::OpenGL, Application::GetViewportSize(), Application::Core::GetSDLWindowHandle());

	// Create scene and make viewport 0 point to scene 0.
	Scene& scene1 = Engine::NewScene();
	Renderer::GetViewport(0).SetSceneRef(&scene1);



	auto& sceneObject1 = scene1.NewSceneObject();
	auto& sprite1 = sceneObject1.AddComponent<SpriteRenderer>();
	sprite1.SetSprite(Asset::Sprite::Default);
	auto& rb1 = sceneObject1.AddComponent<RigidBody2D>();

	auto& objCamera = scene1.NewSceneObject();
	auto& camera = objCamera.AddComponent<Camera>();
	camera.position.z = 5.f;


	Renderer::RenderGraph graph;
	Renderer::RenderGraphTransform graphTransform;


	while (Application::Core::UpdateEvents(), Application::IsRunning())
	{
		// Handles origin movement for camera
		const float speed = 2.5f;
		auto cross = Math::Vector3D::Cross(camera.forward, camera.up);
		if (Input::Raw::GetValue(Input::Raw::Button::A))
			camera.position -= cross * speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::D))
			camera.position += cross * speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::W))
			camera.position += camera.forward *  speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::S))
			camera.position -= camera.forward * speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::Space))
			camera.position += Math::Vector3D::Up() * speed * scene1.GetTimeData().GetDeltaTime();
		if (Input::Raw::GetValue(Input::Raw::Button::Ctrl))
			camera.position += Math::Vector3D::Down() * speed * scene1.GetTimeData().GetDeltaTime();
		camera.LookAt({ 0, 0, 0 });

		std::cout << scene1.GetTimeData().GetFPS() << std::endl;





		if (scene1.GetTimeData().GetTickCount() == 0 || scene1.GetTimeData().GetTickCount() == 1)
		{
			graph.sprites.push_back(static_cast<Renderer::SpriteID>(Asset::Sprite::Default));
			graphTransform.sprites.push_back(sprite1.GetModel(Space::World));
		}

		Renderer::Core::SetCameraInfo(camera.GetCameraInfo());

		Renderer::Core::PrepareRenderingEarly(graph);

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
