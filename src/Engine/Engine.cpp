#include "Engine.hpp"

#include "Physics2D.hpp"

#include "Application.hpp"
#include "Renderer/Renderer.hpp"
#include "Components/SpriteRenderer.hpp"
#include "Components/MeshRenderer.hpp"
#include "Time/Time.hpp"
#include "Input/InputRaw.hpp"
#include "Scene.hpp"
#include "SceneObject.hpp"

#include "Components/Camera.hpp"
#include "Components/RigidBody2D.hpp"

#include <iostream>

std::vector<std::unique_ptr<Engine::Scene>> Engine::Core::scenes;

namespace Engine
{
	void BuildRenderGraph(const Scene& scene, Renderer::RenderGraph& graph, Renderer::RenderGraphTransform& transforms);
}

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
	auto& sprite1 = sceneObject1.AddComponent<MeshRenderer>().first.get();
	sprite1.SetMesh(Asset::Mesh::Cube);

	auto& objCamera = scene1.NewSceneObject();
	auto& camera = objCamera.AddComponent<Camera>().first.get();

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

		//std::cout << scene1.GetTimeData().GetFPS() << std::endl;



		BuildRenderGraph(scene1, graph, graphTransform);

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

void Engine::BuildRenderGraph(const Scene& scene, Renderer::RenderGraph& graph, Renderer::RenderGraphTransform& transforms)
{
	auto spriteComponentsPtr = scene.GetAllComponents<SpriteRenderer>();
	if (spriteComponentsPtr == nullptr)
	{
		graph.sprites.clear();
		transforms.sprites.clear();
	}
	else
	{
		const auto& spriteComponents = *spriteComponentsPtr;
		graph.sprites.resize(spriteComponents.size());
		transforms.sprites.resize(spriteComponents.size());
		for (size_t i = 0; i < spriteComponents.size(); i++)
		{
			graph.sprites[i] = static_cast<Renderer::SpriteID>(spriteComponents[i].GetSprite());
			transforms.sprites[i] = spriteComponents[i].GetModel(Space::World);
		}
	}

	auto meshComponentsPtr = scene.GetAllComponents<MeshRenderer>();
	if (meshComponentsPtr == nullptr)
	{
		graph.meshes.clear();
		transforms.meshes.clear();
	}
	else
	{
		const auto& meshComponents = *meshComponentsPtr;
		graph.meshes.resize(meshComponents.size());
		transforms.meshes.resize(meshComponents.size());
		for (size_t i = 0; i < meshComponents.size(); i++)
		{
			graph.meshes[i] = static_cast<Renderer::MeshID>(meshComponents[i].GetMesh());
			transforms.meshes[i] = meshComponents[i].GetModel(Space::World);
		}
	}
}