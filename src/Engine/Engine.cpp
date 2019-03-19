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

#include "Systems/RenderSystem.hpp"

#include "Math/LinearTransform3D.hpp"

#include "Components/ScriptBase.hpp"

#include <iostream>

class ScriptTest : public Engine::Components::ScriptBase
{
	using ParentType = Engine::Components::ScriptBase;

public:
	explicit ScriptTest(Engine::SceneObject& owningObject) :
		ParentType(owningObject)
	{

	}

protected:
	

	void Start() override
	{
		ParentType::Start();
	}

	void Tick() override
	{
		ParentType::Tick();
		std::cout << "We ticked!" << std::endl;
	}
};

class ScriptTest2 : public Engine::Components::ScriptBase
{
	using ParentType = Engine::Components::ScriptBase;

public:
	explicit ScriptTest2(Engine::SceneObject& owningObject) :
			ParentType(owningObject)
	{

	}

protected:


	void Start() override
	{
		ParentType::Start();
	}

	void Tick() override
	{
		ParentType::Tick();
		std::cout << "We ticked222222!" << std::endl;
	}
};

std::vector<std::unique_ptr<Engine::Scene>> Engine::Core::scenes;

void Engine::Core::Run()
{
	Application::Core::Initialize(Application::API3D::OpenGL);
	Time::Core::Initialize();
	Input::Core::Initialize();
	Physics2D::Core::Initialize();

	Renderer::CreateInfo rendererCreateInfo;
	rendererCreateInfo.preferredAPI = Renderer::API::OpenGL;
	rendererCreateInfo.surfaceDimensions = Application::GetWindowSize();
	rendererCreateInfo.surfaceHandle = Application::Core::GetMainWindowHandle();
	rendererCreateInfo.openGLCreateInfo.glSwapBuffers = Application::Core::GL_SwapWindow;
	Renderer::Core::Initialize(std::move(rendererCreateInfo));

	// Create scene and make viewport 0 point to scene 0.
	Scene& scene1 = Engine::NewScene();
	Renderer::GetViewport(0).SetSceneRef(&scene1);




	auto& sceneObject1 = scene1.NewSceneObject();
	auto& sprite1 = sceneObject1.AddComponent<Components::MeshRenderer>().first.get();
	sprite1.SetMesh(Asset::Mesh::Helmet);

	auto& mesh2 = sceneObject1.AddComponent<Components::MeshRenderer>().first.get();
	mesh2.SetMesh(Asset::Mesh::Cube);
	mesh2.position.x = 2.5f;

	auto& objCamera = scene1.NewSceneObject();
	auto& camera = objCamera.AddComponent<Components::Camera>().first.get();

	camera.position.z = 5.f;

	auto& obj3 = scene1.NewSceneObject();
	ScriptTest& scriptTest = obj3.AddComponent<ScriptTest>();
	ScriptTest2& scriptTest2 = obj3.AddComponent<ScriptTest2>();


	Renderer::RenderGraph graph;
	Renderer::RenderGraphTransform graphTransform;


	scene1.ScriptStart();
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
		if (Input::Raw::GetValue(Input::Raw::Button::LeftCtrl))
			camera.position += Math::Vector3D::Down() * speed * scene1.GetTimeData().GetDeltaTime();
		camera.LookAt({ 0, 0, 0 });


		scene1.ScriptTick();


		RenderSystem::BuildRenderGraph(scene1, graph, graphTransform);



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

