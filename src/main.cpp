
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

#include <box2d/box2d.h>

#include <iostream>
#include <vector>
#include <string>

#include <DEngine/Std/Trait.hpp>
#include <DEngine/Std/Containers/Variant.hpp>

class GfxLogger : public DEngine::Gfx::LogInterface
{
public:
	virtual void log(DEngine::Gfx::LogInterface::Level level, const char* msg) override 
	{
		DEngine::App::Log(msg);
	}

	virtual ~GfxLogger() override
	{

	}
};

class GfxTexAssetInterfacer : public DEngine::Gfx::TextureAssetInterface
{
	virtual char const* get(DEngine::Gfx::TextureID id) const override
	{
		if ((DEngine::u64)id == 0)
			return "data/01.ktx";
		else if ((DEngine::u64)id == 1)
			return "data/2.png";
		else
			return "data/Circle.png";
	}
};

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
		auto rbIt = Std::FindIf(
			scene.rigidbodies.begin(),
			scene.rigidbodies.end(),
			[entity](auto const& val) -> bool { return entity == val.a; });

		if (rbIt != scene.rigidbodies.end())
		{
			Physics::Rigidbody2D& rb = rbIt->b;
			rb.acceleration += addAcceleration;
		}

		auto physicsBodyIt = Std::FindIf(
			scene.b2Bodies.begin(),
			scene.b2Bodies.end(),
			[entity](auto const& val) -> bool { return entity == val.a; });
		if (physicsBodyIt != scene.b2Bodies.end())
		{
			auto& physicsBody = *physicsBodyIt;
			physicsBody.b.ptr->ApplyForceToCenter({ addAcceleration.x, addAcceleration.y }, true);
		}
	}
}

namespace DEngine::detail
{
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

	void SubmitRendering(
		Gfx::Context& gfxData,
		Editor::Context& editorData,
		Scene& scene);
}

int DENGINE_APP_MAIN_ENTRYPOINT(int argc, char** argv)
{
	using namespace DEngine;

	DEngine::Std::NameThisThread("MainThread");

	Time::Initialize();
	App::detail::Initialize();

	App::WindowID mainWindow = App::CreateWindow(
		"Main window",
		{ 1000, 750 });

	auto gfxWsiConnection = new detail::GfxWsiConnection;
	gfxWsiConnection->appWindowID = mainWindow;

	// Initialize the renderer
	auto requiredInstanceExtensions = App::RequiredVulkanInstanceExtensions();
	GfxLogger gfxLogger{};
	GfxTexAssetInterfacer gfxTexAssetInterfacer{};
	Gfx::InitInfo rendererInitInfo{};
	rendererInitInfo.initialWindowConnection = gfxWsiConnection;
	rendererInitInfo.texAssetInterface = &gfxTexAssetInterfacer;
	rendererInitInfo.optional_logger = &gfxLogger;
	rendererInitInfo.requiredVkInstanceExtensions = requiredInstanceExtensions.ToSpan();
	rendererInitInfo.gizmoArrowMesh = Editor::BuildGizmoArrowMesh();
	Std::Opt<Gfx::Context> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
	if (!rendererDataOpt.HasValue())
	{
		std::cout << "Could not initialize renderer." << std::endl;
		std::abort();
	}
	Gfx::Context gfxCtx = Std::Move(rendererDataOpt.Value());

	Scene myScene;

	Std::Box<b2World> physicsWorld{ new b2World({ 0.f, -10.f }) };

	{
		Entity a = myScene.NewEntity();

		Transform transform{};
		transform.position.x = 0.f;
		transform.position.y = 0.f;
		//transform.rotation = 0.707f;
		transform.scale = { 1.f, 1.f };
		myScene.transforms.push_back({ a, transform });

		Physics::Rigidbody2D rb{};
		myScene.rigidbodies.push_back({ a, rb });

		Gfx::TextureID textureId{ 0 };
		myScene.textureIDs.push_back({ a, textureId });

		Physics::BoxCollider2D collider{};
		myScene.boxColliders.push_back({ a, collider });

		b2BodyDef bodyDef{};
		bodyDef.type = b2BodyType::b2_dynamicBody;
		bodyDef.fixedRotation = true;
		b2Body* physicsBody = physicsWorld->CreateBody(&bodyDef);
		DEngine::Box2DBody body{};
		body.ptr = physicsBody;
		myScene.b2Bodies.push_back({ a, body });
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
		transform.scale = { 1.f, 1.f };
		myScene.transforms.push_back({ a, transform });

		//Physics::Rigidbody2D rb{};
		//myScene.rigidbodies.push_back({ a, rb });

		Gfx::TextureID textureId{ 2 };
		myScene.textureIDs.push_back({ a, textureId });

		//Physics::CircleCollider2D collider{};
		//myScene.circleColliders.push_back({ a, collider });
	}

	{
		Entity a = myScene.NewEntity();

		Transform transform{};
		transform.position.x = 0.f;
		transform.position.y = -5.f;
		transform.scale = { 25.f, 1.f };
		myScene.transforms.push_back({ a, transform });

		//Physics::Rigidbody2D rb{};
		//myScene.rigidbodies.push_back({ a, rb });

		Gfx::TextureID textureId{ 0 };
		myScene.textureIDs.push_back({ a, textureId });

		//Physics::BoxCollider2D collider{};
		//myScene.boxColliders.push_back({ a, collider });

		b2BodyDef bodyDef{};
		bodyDef.type = b2BodyType::b2_staticBody;
		b2Body* physicsBody = physicsWorld->CreateBody(&bodyDef);
		DEngine::Box2DBody body{};
		body.ptr = physicsBody;
		myScene.b2Bodies.push_back({ a, body });
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
		transform.scale = { 1.f, 1.f };
		myScene.transforms.push_back({ a, transform });

		Physics::Rigidbody2D rb{};
		rb.inverseMass = 1 / 100.f;
		myScene.rigidbodies.push_back({ a, rb });

		Gfx::TextureID textureId{ 0 };
		myScene.textureIDs.push_back({ a, textureId });

		Physics::BoxCollider2D collider{};
		myScene.boxColliders.push_back({ a, collider });

		b2BodyDef bodyDef{};
		bodyDef.type = b2BodyType::b2_dynamicBody;
		b2Body* physicsBody = physicsWorld->CreateBody(&bodyDef);
		DEngine::Box2DBody body{};
		body.ptr = physicsBody;
		myScene.b2Bodies.push_back({ a, body });
		b2MassData massData{};
		massData.mass = 1.f;
		physicsBody->SetMassData(&massData);
		b2PolygonShape boxShape{};
		boxShape.SetAsBox(0.5f, 0.5f);
		f32 density = 1.;
		physicsBody->CreateFixture(&boxShape, density);
	}

	/*
	uSize yo = 10;
	for (uSize i = 0; i < yo; i += 1)
	{
		for (uSize j = 0; j < yo; j += 1)
		{
			Entity a = myScene.NewEntity();

			f32 spacing = 3.f;

			Transform transform{};
			transform.position.x -= (spacing * yo) / 2.f;
			transform.position.x += spacing * i;
			transform.position.y -= (spacing * yo) / 2.f;
			transform.position.y += spacing * j;
			myScene.transforms.push_back({ a, transform });

			Physics::Rigidbody2D rb{};
			myScene.rigidbodies.push_back({ a, rb });

			if ((i + j) % 2)
			{
				Gfx::TextureID textureId{ 2 };
				myScene.textureIDs.push_back({ a, textureId });

				Physics::CircleCollider2D collider{};
				myScene.circleColliders.push_back({ a, collider });
			}
			else
			{
				Gfx::TextureID textureId{ 0 };
				myScene.textureIDs.push_back({ a, textureId });

				Physics::BoxCollider2D collider{};
				myScene.boxColliders.push_back({ a, collider });
			}
		}
	}
	*/

	Move move{};
	myScene.moves.push_back({ Entity(), move });

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

			for (auto item : myScene.moves)
				item.b.Update(item.a, myScene, Time::Delta());

			// First copy our positions into every physics body
			for (auto const& physicsBodyPair : myScene.b2Bodies)
			{
				Entity a = physicsBodyPair.a;
				auto const& transform = Std::FindIf(
					myScene.transforms.begin(),
					myScene.transforms.end(),
					[a](auto const& val) -> bool { return val.a == a; })->b;
				physicsBodyPair.b.ptr->SetTransform({ transform.position.x, transform.position.y }, transform.rotation);
			}
			physicsWorld->Step(Time::Delta(), 8, 8);
			// Then copy the stuff back
			for (auto const& physicsBodyPair : myScene.b2Bodies)
			{
				Entity a = physicsBodyPair.a;
				auto& transform = Std::FindIf(
					myScene.transforms.begin(),
					myScene.transforms.end(),
					[a](auto const& val) -> bool { return val.a == a; })->b;
				auto physicsBodyTransform = physicsBodyPair.b.ptr->GetTransform();
				transform.position = { physicsBodyTransform.p.x, physicsBodyTransform.p.y };
				transform.rotation = physicsBodyTransform.q.GetAngle();
			}
		}

		detail::SubmitRendering(
			gfxCtx, 
			editorCtx, 
			myScene);
	}

	return 0;
}

void DEngine::detail::SubmitRendering(
	Gfx::Context& gfxData,
	Editor::Context& editorCtx,
	Scene& scene)
{
	Gfx::DrawParams params{};

	for (auto item : scene.textureIDs)
	{
		auto& entity = item.a;

		// First check if this entity has a position
		auto posIt = Std::FindIf(
			scene.transforms.begin(),
			scene.transforms.end(),
			[&entity](auto const& val) -> bool { return val.a == entity; });
		if (posIt == scene.transforms.end())
			continue;
		auto& transform = posIt->b;

		params.textureIDs.push_back(item.b);

		Math::Mat4 transformMat = Math::LinTran3D::Translate(transform.position) * 
			Math::LinTran3D::Rotate_Homo(Math::ElementaryAxis::Z, transform.rotation) * 
			Math::LinTran3D::Scale_Homo(transform.scale.AsVec3());
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