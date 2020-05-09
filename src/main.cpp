
#include "DEngine/Scene.hpp"
#include "DEngine/Application/detail_Application.hpp"
#include "DEngine/Time.hpp"
#include "DEngine/Editor.hpp"

#include "DEngine/Gfx/Gfx.hpp"
#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Utility.hpp"
#include "DEngine/Math/Vector.hpp"
#include "DEngine/Math/UnitQuaternion.hpp"
#include "DEngine/Math/LinearTransform3D.hpp"

#include <iostream>
#include <vector>
#include <string>

extern DEngine::Editor::EditorData* DEngine_Debug_globEditorData;

class GfxLogger : public DEngine::Gfx::ILog
{
public:
	virtual void log(const char* msg) override 
	{
		DEngine::App::Log(msg);
	}

	virtual ~GfxLogger() override
	{

	}
};

class GfxWsiInterfacer : public DEngine::Gfx::IWsi
{
public:
	virtual ~GfxWsiInterfacer() 
	{
	};

	// Return type is VkResult
	//
	// Argument #1: VkInstance - The Vulkan instance handle
	// Argument #2: VkAllocationCallbacks const* - Allocation callbacks for surface creation.
	// Argument #3: VkSurfaceKHR* - The output surface handle
	virtual DEngine::i32 CreateVkSurface(DEngine::uSize vkInstance, void const* allocCallbacks, DEngine::u64& outSurface) override
	{
		 bool result = DEngine::App::detail::CreateVkSurface(vkInstance, allocCallbacks, nullptr, outSurface);
		 if (result)
			 return 0; // 0 is VkResult_Success
		 else
			 return -1;
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
		[entity](Std::Pair<Entity, Physics::Rigidbody2D> const& val) -> bool
		{
			return entity == val.a;
		});
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

int DENGINE_APP_MAIN_ENTRYPOINT(int argc, char** argv)
{
	using namespace DEngine;

	Time::Initialize();
	App::detail::Initialize();

	{
		// Initialize ImGui stuff
		IMGUI_CHECKVERSION();
		ImGuiContext* imguiContext = ImGui::CreateContext();
		ImGui::SetCurrentContext(imguiContext);
		ImGuiIO& imguiIO = ImGui::GetIO();
		imguiIO.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_DockingEnable;

		//if constexpr (App::targetOS == App::OS::Windows)
			//imguiIO.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_ViewportsEnable

		//ImGui::StyleColorsDark();
		
		App::detail::ImgGui_Initialize();
	}
	Editor::EditorData editorData = Editor::Initialize();
	DEngine_Debug_globEditorData = &editorData;



	// Initialize the renderer
	auto requiredInstanceExtensions = App::detail::GetRequiredVulkanInstanceExtensions();
	GfxLogger gfxLogger{};
	GfxWsiInterfacer gfxWsiInterface{};
	GfxTexAssetInterfacer gfxTexAssetInterfacer{};
	Gfx::InitInfo rendererInitInfo{};
	rendererInitInfo.iWsi = &gfxWsiInterface;
	rendererInitInfo.texAssetInterface = &gfxTexAssetInterfacer;
	rendererInitInfo.optional_iLog = &gfxLogger;
	rendererInitInfo.requiredVkInstanceExtensions = requiredInstanceExtensions.ToSpan();
	Std::Opt<Gfx::Data> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
	if (!rendererDataOpt.HasValue())
	{
		std::cout << "Could not initialize renderer." << std::endl;
		std::abort();
	}
	Gfx::Data rendererData = Std::Move(rendererDataOpt.Value());


	Scene myScene;


	while (true)
	{
		Time::TickStart();
		App::detail::ProcessEvents();
		if (App::detail::ShouldShutdown())
			break;
		App::detail::ImGui_NewFrame();

		Editor::RenderImGuiStuff(editorData, myScene, rendererData);

		Physics::Update(myScene, Time::Delta());

		for (auto item : myScene.moves)
			item.b.Update(item.a, myScene, Time::Delta());

		if (!App::MainWindowMinimized())
		{
			Gfx::DrawParams params{};

			for (auto const& [vpID, viewport] : editorData.viewports)
			{
				if (viewport.visible && !viewport.paused)
				{
					Gfx::ViewportUpdateData viewportData{};
					viewportData.id = viewport.gfxViewportRef.ViewportID();
					viewportData.width = viewport.renderWidth;
					viewportData.height = viewport.renderHeight;
					uSize camIndex = static_cast<uSize>(-1);
					for (uSize i = 0; i < editorData.cameras.size(); i += 1)
					{
						if (viewport.cameraID == editorData.cameras[i].a)
						{
							camIndex = i;
							break;
						}
					}
					Editor::Camera const& cam = camIndex == static_cast<uSize>(-1) ? viewport.camera : editorData.cameras[camIndex].b;
					Math::Mat4 camMat = Math::LinTran3D::Rotate_Homo(cam.rotation);
					camMat = Math::LinTran3D::Translate(cam.position) * camMat;
					camMat = camMat.GetInverse().Value();
					f32 aspectRatio = (f32)viewport.width / viewport.height;
					camMat = Math::LinTran3D::Perspective_RH_ZO(cam.fov, aspectRatio, cam.zNear, cam.zFar) * camMat;
					viewportData.transform = camMat;
					params.viewportUpdates.push_back(viewportData);
				}
			}

			for (auto item : myScene.textureIDs)
			{
				auto& entity = item.a;

				// First check if this entity has a position
				auto posIndex = myScene.transforms.FindIf([&entity](Std::Pair<Entity, Transform> const& val) -> bool {
					return val.a == entity;
					});
				if (!posIndex.HasValue())
					continue;
				auto& transform = myScene.transforms[posIndex.Value()].b;

				params.textureIDs.push_back(item.b);
				params.transforms.push_back(Math::LinTran3D::Translate(transform.position));
			}


			params.swapchainWidth = App::detail::mainWindowSize[0];
			params.swapchainHeight = App::detail::mainWindowSize[1];
			if (App::detail::MainWindowRestoreEvent())
				params.restoreEvent = true;

			rendererData.Draw(params);
		}
	}

	return 0;
}