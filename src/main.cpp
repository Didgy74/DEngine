
#include <DEngine/Scene.hpp>
#include "DEngine/Application/detail_Application.hpp"
#include <DEngine/Time.hpp>
#include "DEngine/Editor/Editor.hpp"

#include "DEngine/Gfx/Gfx.hpp"
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <iostream>
#include <vector>
#include <string>

void DEngine::Move::Update(Entity entity, Scene& scene, f32 deltaTime) const
{
	Math::Vec2 addAcceleration{};

	f32 amount = 300.f * deltaTime;

	if (App::ButtonValue(App::Button::Up))
		addAcceleration.y += 1.f;
	if (App::ButtonValue(App::Button::Down))
		addAcceleration.y -= 1.f;
	if (App::ButtonValue(App::Button::Right))
		addAcceleration.x += 1.f;
	if (App::ButtonValue(App::Button::Left))
		addAcceleration.x -= 1.f;

	if (addAcceleration.MagnitudeSqrd() != 0)
	{
		addAcceleration.Normalize();
		addAcceleration *= amount;
	}

	if (App::ButtonEvent(App::Button::Space) == App::KeyEventType::Pressed)
		addAcceleration.y += 400.f;

	if (addAcceleration.MagnitudeSqrd() != 0)
	{
		Box2DBody* body = scene.GetComponent<Box2DBody>(entity);
		if (body != nullptr)
		{
			body->ptr->ApplyForceToCenter({ addAcceleration.x, addAcceleration.y }, true);
		}
	}
}

namespace DEngine::impl
{
	class GfxLogger : public Gfx::LogInterface
	{
	public:
		virtual void Log(Gfx::LogInterface::Level level, const char* msg) override
		{
			App::Log(msg);
		}

		virtual ~GfxLogger() override
		{

		}
	};

	class GfxTexAssetInterfacer : public Gfx::TextureAssetInterface
	{
		virtual char const* get(Gfx::TextureID id) const override
		{
			if ((u64)id == 0)
				return "data/01.ktx";
			else if ((u64)id == 1)
				return "data/2.png";
			else
				return "data/Circle.png";
		}
	};

	struct GfxWsiConnection : public Gfx::WsiInterface
	{
		App::WindowID appWindowID{};

		// Return type is VkResult
		//
		// Argument #1: VkInstance - The Vulkan instance handle
		// Argument #2: VkAllocationCallbacks const* - Allocation callbacks for surface creation.
		// Argument #3: VkSurfaceKHR* - The output surface handle
		virtual i32 CreateVkSurface(uSize vkInstance, void const* allocCallbacks, u64& outSurface) override
		{
			auto resultOpt = App::CreateVkSurface(appWindowID, vkInstance, nullptr);
			if (resultOpt.HasValue())
			{
				outSurface = resultOpt.Value();
				return 0; // 0 is VK_RESULT_SUCCESS
			}
			else
				return -1;
		}
	};

	Gfx::Context CreateGfxContext(
		Gfx::WsiInterface& wsiConnection,
		Gfx::TextureAssetInterface const& textureAssetConnection,
		Gfx::LogInterface& logger,
		Std::Span<char const*> requiredVkInstanceExtensions)
	{
		Gfx::InitInfo rendererInitInfo{};
		rendererInitInfo.initialWindowConnection = &wsiConnection;
		rendererInitInfo.texAssetInterface = &textureAssetConnection;
		rendererInitInfo.optional_logger = &logger;
		rendererInitInfo.requiredVkInstanceExtensions = requiredVkInstanceExtensions;
		rendererInitInfo.gizmoArrowMesh = Editor::BuildGizmoArrowMesh();
		Std::Opt<Gfx::Context> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
		if (!rendererDataOpt.HasValue())
		{
			std::cout << "Could not initialize renderer." << std::endl;
			std::abort();
		}
		return static_cast<Gfx::Context&&>(rendererDataOpt.Value());
	}

	void RunPhysicsStep(
		Scene& scene);
	
	void SubmitRendering(
		Gfx::Context& gfxData,
		Editor::Context& editorData,
		Scene& scene);
}

#include <DEngine/Std/Containers/Variant.hpp>

int DENGINE_APP_MAIN_ENTRYPOINT(int argc, char** argv)
{
	using namespace DEngine;

	Std::NameThisThread("MainThread");

	Time::Initialize();
	App::detail::Initialize();

	App::WindowID mainWindow = App::CreateWindow(
		"Main window",
		{ 1000, 750 });

	auto gfxWsiConnection = new impl::GfxWsiConnection;
	gfxWsiConnection->appWindowID = mainWindow;

	// Initialize the renderer
	auto requiredInstanceExtensions = App::RequiredVulkanInstanceExtensions();
	impl::GfxLogger gfxLogger{};
	impl::GfxTexAssetInterfacer gfxTexAssetInterfacer{};
	Gfx::Context gfxCtx = impl::CreateGfxContext(
		*gfxWsiConnection,
		gfxTexAssetInterfacer,
		gfxLogger,
		requiredInstanceExtensions);

	Scene myScene;

	{
		Entity a = myScene.NewEntity();

		Transform transform{};
		transform.position.x = 0.f;
		transform.position.y = 0.f;
		//transform.rotation = 0.707f;
		//transform.scale = { 1.f, 1.f };
		myScene.AddComponent(a, transform);

		Gfx::TextureID textureId{ 0 };
		myScene.AddComponent(a, textureId);

		b2BodyDef bodyDef{};
		bodyDef.type = b2BodyType::b2_dynamicBody;
		bodyDef.fixedRotation = true;
		b2Body* physicsBody = myScene.physicsWorld->CreateBody(&bodyDef);
		DEngine::Box2DBody body{};
		body.ptr = physicsBody;
		myScene.AddComponent(a, body);
		b2MassData massData{};
		massData.mass = 1.f;
		physicsBody->SetMassData(&massData);
		b2PolygonShape boxShape{};
		boxShape.SetAsBox(0.5f, 0.5f);
		f32 density = 1.;
		physicsBody->CreateFixture(&boxShape, density);
	}

	{
		Entity a = myScene.NewEntity();

		Transform transform{};
		transform.position.x = 2.f;
		transform.position.y = 0.f;
		//transform.scale = { 1.f, 1.f };
		myScene.AddComponent(a, transform);

		Gfx::TextureID textureId{ 2 };
		myScene.AddComponent(a, textureId);
	}

	{
		Entity a = myScene.NewEntity();

		Transform transform{};
		transform.position.x = 0.f;
		transform.position.y = -5.f;
		//transform.scale = { 25.f, 1.f };
		myScene.AddComponent(a, transform);

		Gfx::TextureID textureId{ 0 };
		myScene.AddComponent(a, textureId);

		b2BodyDef bodyDef{};
		bodyDef.type = b2BodyType::b2_staticBody;
		b2Body* physicsBody = myScene.physicsWorld->CreateBody(&bodyDef);
		DEngine::Box2DBody body{};
		body.ptr = physicsBody;
		myScene.AddComponent(a, body);
		b2MassData massData{};
		massData.mass = 1.f;
		physicsBody->SetMassData(&massData);
		b2PolygonShape boxShape{};
		boxShape.SetAsBox(12.5f, 0.5f);
		f32 density = 1.;
		physicsBody->CreateFixture(&boxShape, density);
	}

	{
		Entity a = myScene.NewEntity();

		Transform transform{};
		transform.position.x = -2.f;
		transform.position.y = 0.f;
		//transform.scale = { 1.f, 1.f };
		myScene.AddComponent(a, transform);

		Gfx::TextureID textureId{ 0 };
		myScene.AddComponent(a, textureId);

		b2BodyDef bodyDef{};
		bodyDef.type = b2BodyType::b2_dynamicBody;
		b2Body* physicsBody = myScene.physicsWorld->CreateBody(&bodyDef);
		DEngine::Box2DBody body{};
		body.ptr = physicsBody;
		myScene.AddComponent(a, body);
		b2MassData massData{};
		massData.mass = 1.f;
		physicsBody->SetMassData(&massData);
		b2PolygonShape boxShape{};
		boxShape.SetAsBox(0.5f, 0.5f);
		f32 density = 1.f;
		physicsBody->CreateFixture(&boxShape, density);
	}

	Move move{};
	myScene.AddComponent( Entity(), move);

	Editor::Context editorCtx = Editor::Context::Create(
		mainWindow,
		&myScene,
		&gfxCtx);

	while (true)
	{
		Time::TickStart();
		App::detail::ProcessEvents();
		if (App::GetWindowCount() == 0)
			break;

		editorCtx.ProcessEvents();

		if (myScene.play)
		{
			//Physics::Update(myScene, Time::Delta());

			for (auto const& [entity, moveComponent] : myScene.GetComponentVector<Move>())
				moveComponent.Update(entity, myScene, Time::Delta());

			impl::RunPhysicsStep(myScene);
		}

		impl::SubmitRendering(
			gfxCtx, 
			editorCtx, 
			myScene);
	}

	return 0;
}

void DEngine::impl::RunPhysicsStep(
	Scene& scene)
{
	// First copy our positions into every physics body
	for (auto const& physicsBodyPair : scene.GetComponentVector<Box2DBody>())
	{
		Entity a = physicsBodyPair.a;

		Transform const* transformPtr = scene.GetComponent<Transform>(a);
		DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
		Transform const& transform = *transformPtr;
		physicsBodyPair.b.ptr->SetTransform({ transform.position.x, transform.position.y }, transform.rotation);
	}
	scene.physicsWorld->Step(Time::Delta(), 8, 8);
	// Then copy the stuff back
	for (auto const& physicsBodyPair : scene.GetComponentVector<Box2DBody>())
	{
		Entity a = physicsBodyPair.a;
		Transform* transformPtr = scene.GetComponent<Transform>(a);
		DENGINE_DETAIL_ASSERT(transformPtr != nullptr);
		Transform& transform = *transformPtr;
		auto physicsBodyTransform = physicsBodyPair.b.ptr->GetTransform();
		transform.position = { physicsBodyTransform.p.x, physicsBodyTransform.p.y };
		transform.rotation = physicsBodyTransform.q.GetAngle();
	}
}

void DEngine::impl::SubmitRendering(
	Gfx::Context& gfxData,
	Editor::Context& editorCtx,
	Scene& scene)
{
	Gfx::DrawParams params{};

	for (auto const& item : scene.GetComponentVector<Gfx::TextureID>())
	{
		auto& entity = item.a;

		// First check if this entity has a position
		Transform* transformPtr = scene.GetComponent<Transform>(entity);
		if (transformPtr == nullptr)
			continue;
		Transform& transform = *transformPtr;

		params.textureIDs.push_back(item.b);

		Math::Mat4 transformMat = Math::LinTran3D::Translate(transform.position) *
			Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Z, transform.rotation);
		params.transforms.push_back(transformMat);
	}

	auto editorDrawData = editorCtx.GetDrawInfo();
	params.guiVertices = editorDrawData.vertices;
	params.guiIndices = editorDrawData.indices;
	params.guiDrawCmds = editorDrawData.drawCmds;
	params.nativeWindowUpdates = editorDrawData.windowUpdates;
	params.viewportUpdates = editorDrawData.viewportUpdates;
	params.lineVertices = editorDrawData.lineVertices;
	params.lineDrawCmds = editorDrawData.lineDrawCmds;



	gfxData.Draw(params);
}