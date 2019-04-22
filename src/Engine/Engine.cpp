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
#include "InputRotate.hpp"

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
		// Whether we want to enable debugging in the rendering system
		// And the error callback method we want to use.
		rendererInitInfo.debugInitInfo.useDebugging = true;
		rendererInitInfo.debugInitInfo.errorMessageCallback = rendererDebugCallback;

		// Dimensions and handle to a surface we have generated to render onto.
		rendererInitInfo.surfaceDimensions = Application::GetWindowSize();
		rendererInitInfo.surfaceHandle = Application::Core::GetMainWindowHandle();

		// Set function pointer to our decoupled asset loaders.
		rendererInitInfo.assetLoadCreateInfo.meshLoader = &AssMan::GetRendererMeshDoc;
		rendererInitInfo.assetLoadCreateInfo.assetLoadEnd = &AssMan::Renderer_AssetLoadEndEvent;
		rendererInitInfo.assetLoadCreateInfo.textureLoader = &LoadTexture;

		// Function pointer to the Application systems GL_SwapBuffer function.
		// This one goes to GLFW3's swap buffer function.
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

	// Add our cube model to the asset manager
	AssMan::MeshInfo meshInfo;
	meshInfo.path = "data/Meshes/Cube/Cube.gltf";
	size_t cubeMesh = AssMan::AddMesh(std::move(meshInfo));

	// Create camera Object
	// SceneObjects are guaranteed to be in the same place in memory always. No need for soft-references.
	auto& objCamera = scene1.NewSceneObject();
	objCamera.localPosition.z = -5.f;
	// Add a component to our SceneObject (similiar to Unity's GameObject)
	// This function returns a soft-reference to the component (.first) and a hard-reference (.second.get()).
	// We use hard-references, but components may move in memory over the lifetime of the program
	// so soft-references are usually preferred.
	auto& camera = objCamera.AddComponent<Components::Camera>().second.get();
	camera.rotation = camera.rotation;
	objCamera.AddComponent<Components::FreeLook>();

	// Light object 1
	auto& lightObj = scene1.NewSceneObject();
	lightObj.localPosition = { 0.f, 1.f, 0.f };
	lightObj.localScale = { 0.1f, 0.1f, 0.1f };
	Components::PointLight& light1 = lightObj.AddComponent<Components::PointLight>().second.get();
	light1.color = { 1.f, 0.5f, 0.5f };
	light1.intensity = 0.5f;
	auto& sinus1 = lightObj.AddComponent<Assignment02::SinusMovement>();
	auto& mesh1 = lightObj.AddComponent<Components::MeshRenderer>().second.get();
	mesh1.mesh = cubeMesh;
	mesh1.texture = 0;


	// Light object 2
	auto& lightObj2 = scene1.NewSceneObject();
	lightObj2.localPosition = { 0.f, 5.f, 3.f };
	lightObj2.localScale = { 0.1f, 0.1f, 0.1f };
	auto& mesh2 = lightObj2.AddComponent<Components::MeshRenderer>().second.get();
	mesh2.mesh = cubeMesh;
	mesh2.texture = 0;
	auto& sinus2 = lightObj2.AddComponent<Assignment02::SinusMovement>();
	sinus2.timeMultiplier = 15.f;
	sinus2.movementMultiplier = -sinus2.movementMultiplier;
	Components::PointLight& light2 = lightObj2.AddComponent<Components::PointLight>().second.get();
	light2.color = { 0.5f, 1.f, 0.5f };
	light2.intensity = 0.5f;


	// Light object 3
	auto& lightObj3 = scene1.NewSceneObject();
	lightObj3.localPosition = { 0.f, 5.f, -3.5f };
	lightObj3.localScale = { 0.1f, 0.1f, 0.1f };
	auto& mesh3 = lightObj3.AddComponent<Components::MeshRenderer>().second.get();
	mesh3.mesh = cubeMesh;
	mesh3.texture = 0;
	auto& sinus3 = lightObj3.AddComponent<Assignment02::SinusMovement>();
	sinus3.timeMultiplier = 15.f;
	Components::PointLight& light3 = lightObj3.AddComponent<Components::PointLight>().second.get();
	light3.color = { 0.5f, 0.5f, 1.f };
	light3.intensity = 0.5f;

	// Load the GLTF scene, with a reference to the SceneObject we want to add it to
	auto& rootObj = scene1.NewSceneObject();
	rootObj.AddComponent<Assignment02::InputRotate>();
	LoadGLTFScene(rootObj, "data/Sponza/Sponza.gltf");

	lightObj.SetParent(&rootObj);
	lightObj2.SetParent(&rootObj);
	lightObj3.SetParent(&rootObj);

	// Not optimal solution. These are the RenderGraph struct we can work with. 
	// Ideally they should belong to Rendering system
	Renderer::RenderGraph graph;
	Renderer::RenderGraphTransform graphTransform;


	scene1.Scripts_SceneStart();
	// Checks for any window events, like input etc and updates the Input system.
	while (Application::Core::UpdateEvents(), Application::IsRunning())
	{
		// Calls all the custom script components' Tick function.
		scene1.ScriptTick();

		// Build the RenderGraph we want to submit to the rendering system
		RenderSystem::BuildRenderGraph(scene1, graph);
		// Submit built RenderGraph
		Renderer::Core::PrepareRenderingEarly(graph);

		// Update camera-info in the renderer by using the Camera-component we have made.
		Renderer::Core::SetCameraInfo(GetRendererCameraInfo(camera));
		// Build the positional data for the RenderGraph.
		RenderSystem::BuildRenderGraphTransform(scene1, graphTransform);
		// Submit the positional data for the RenderGraph
		Renderer::Core::PrepareRenderingLate(graphTransform);

		// Draw the most recently submitted RenderGraph
		Renderer::Core::Draw();

		// Various End-of-Tick events
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

