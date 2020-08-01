
#include <DEngine/Scene.hpp>
#include "DEngine/Application/detail_Application.hpp"
#include <DEngine/Time.hpp>
#include "DEngine/Editor/Editor.hpp"

#include "DEngine/Gfx/Gfx.hpp"
#include <DEngine/FixedWidthTypes.hpp>
#include "DEngine/Utility.hpp"
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
			return "data/Test.ktx";
		else
			return "data/2.png";
	}
};

void DEngine::Move::Update(Entity entity, Scene& scene, f32 deltaTime) const
{
	auto rbIndex = scene.rigidbodies.FindIf(
		[entity](Std::Pair<Entity, Physics::Rigidbody2D> const& val) -> bool { return entity == val.a; });
	if (!rbIndex.HasValue())
		return;

	Physics::Rigidbody2D& rb = scene.rigidbodies[rbIndex.Value()].b;

	Math::Vec2 addAcceleration{};

	f32 amount = 1.f * deltaTime;

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

	Time::Initialize();
	App::detail::Initialize();

	App::WindowID mainWindow = App::CreateWindow(
		"Main window",
		{ 500, 500 });

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

	Editor::Context editorCtx = Editor::Context::Create(
		mainWindow,
		&myScene,
		&gfxCtx);

	

	while (true)
	{
		Time::TickStart();
		App::detail::ProcessEvents();

		editorCtx.ProcessEvents();

		Physics::Update(myScene, Time::Delta());

		for (auto item : myScene.moves)
			item.b.Update(item.a, myScene, Time::Delta());

		
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
		auto posIndex = scene.transforms.FindIf([&entity](decltype(scene.transforms)::ValueType const& val) -> bool {
			return val.a == entity; });
		if (!posIndex.HasValue())
			continue;
		auto& transform = scene.transforms[posIndex.Value()].b;

		params.textureIDs.push_back(item.b);
		params.transforms.push_back(Math::LinTran3D::Translate(transform.position));
	}

	auto editorDrawData = editorCtx.GetDrawInfo();
	params.guiVerts = editorDrawData.vertices;
	params.guiIndices = editorDrawData.indices;
	params.guiDrawCmds = editorDrawData.drawCmds;
	params.nativeWindowUpdates = editorDrawData.windowUpdates;
	params.viewportUpdates = editorDrawData.viewportUpdates;
	

	gfxData.Draw(params);
}