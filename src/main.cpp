
#include <DEngine/Scene.hpp>
#include "DEngine/Application/detail_Application.hpp"
#include <DEngine/Time.hpp>
#include "DEngine/Editor/Editor.hpp"

#include "DEngine/Gfx/Gfx.hpp"
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/UnitQuaternion.hpp>
#include <DEngine/Math/LinearTransform3D.hpp>

#include <iostream>
#include <vector>
#include <string>

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
	auto rbIt = Std::FindIf(
		scene.rigidbodies.begin(),
		scene.rigidbodies.end(),
		[entity](Std::Pair<Entity, Physics::Rigidbody2D> const& val) -> bool { return entity == val.a; });

	if (rbIt == scene.rigidbodies.end())
		return;

	Physics::Rigidbody2D& rb = rbIt->b;

	Math::Vec2 addAcceleration{};

	f32 amount = 2.f * deltaTime;

	if (App::ButtonValue(App::Button::Up))
		addAcceleration.y += amount;
	if (App::ButtonValue(App::Button::Down))
		addAcceleration.y -= amount;
	if (App::ButtonValue(App::Button::Right))
		addAcceleration.x += amount;
	if (App::ButtonValue(App::Button::Left))
		addAcceleration.x -= amount;

	if (addAcceleration.MagnitudeSqrd() != 0)
	{
		addAcceleration.Normalize();

		rb.acceleration += addAcceleration;
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
		{ 750, 750 });

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
	Std::Opt<Gfx::Context> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
	if (!rendererDataOpt.HasValue())
	{
		std::cout << "Could not initialize renderer." << std::endl;
		std::abort();
	}
	Gfx::Context gfxCtx = Std::Move(rendererDataOpt.Value());

	Scene myScene;

	{
		Entity a = myScene.NewEntity();

		Transform transform{};
		transform.position.x = 0.f;
		transform.position.y = 0.f;
		transform.rotation = 0.707f;
		transform.scale = { 1.f, 1.f };
		myScene.transforms.push_back({ a, transform });

		Physics::Rigidbody2D rb{};
		myScene.rigidbodies.push_back({ a, rb });

		Gfx::TextureID textureId{ 0 };
		myScene.textureIDs.push_back({ a, textureId });

		Physics::BoxCollider2D collider{};
		myScene.boxColliders.push_back({ a, collider });
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
	}

	{
		Entity a = myScene.NewEntity();

		Transform transform{};
		transform.position.x = -2.f;
		transform.position.y = 0.f;
		transform.scale = { 1.f, 25.f };
		myScene.transforms.push_back({ a, transform });

		Physics::Rigidbody2D rb{};
		rb.inverseMass = 1 / 100.f;
		myScene.rigidbodies.push_back({ a, rb });

		Gfx::TextureID textureId{ 0 };
		myScene.textureIDs.push_back({ a, textureId });

		Physics::BoxCollider2D collider{};
		myScene.boxColliders.push_back({ a, collider });
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
			Physics::Update(myScene, Time::Delta());

			for (auto item : myScene.moves)
				item.b.Update(item.a, myScene, Time::Delta());
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
			[&entity](decltype(scene.transforms[0]) const& val) -> bool { return val.a == entity; });
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
	params.guiVerts = editorDrawData.vertices;
	params.guiIndices = editorDrawData.indices;
	params.guiDrawCmds = editorDrawData.drawCmds;
	params.nativeWindowUpdates = editorDrawData.windowUpdates;
	params.viewportUpdates = editorDrawData.viewportUpdates;

	gfxData.Draw(params);
}