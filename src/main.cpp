
#include "ImGui/imgui.h"

#include "DEngine/Application/detail_Application.hpp"
#include "DEngine/Time.hpp"
#include "DEngine/Editor.hpp"

#include "DEngine/Gfx/Gfx.hpp"
#include "DEngine/FixedWidthTypes.hpp"

#include "DEngine/Math/Vector/Vector.hpp"
#include "DEngine/Math/UnitQuaternion.hpp"
#include "DEngine/Math/LinearTransform3D.hpp"

#include <utility>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

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
	virtual DEngine::i32 createVkSurface(DEngine::u64 vkInstance, void const* allocCallbacks, DEngine::u64* outSurface) override
	{
		 bool result = DEngine::App::detail::CreateVkSurface(vkInstance, allocCallbacks, nullptr, outSurface);
		 if (result)
			 return 0; // 0 is VkResult_Success
		 else
			 return -1;
	}
};

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
		imguiIO.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_IsTouchScreen;
		//if constexpr (App::targetOS == App::OS::Windows)
			//imguiIO.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_ViewportsEnable

		//ImGui::StyleColorsDark();
		
		App::detail::ImgGui_Initialize();
	}
	Editor::EditorData editorData = Editor::Initialize();


	// Initialize the renderer
	auto requiredInstanceExtensions = App::detail::GetRequiredVulkanInstanceExtensions();
	GfxLogger gfxLogger{};
	GfxWsiInterfacer gfxWsiInterface{};
	Gfx::InitInfo rendererInitInfo{};
	rendererInitInfo.iWsi = &gfxWsiInterface;
	rendererInitInfo.optional_iLog = &gfxLogger;
	rendererInitInfo.requiredVkInstanceExtensions = requiredInstanceExtensions.ToSpan();
	Std::Optional<Gfx::Data> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
	if (!rendererDataOpt.HasValue())
	{
		std::cout << "Could not initialize renderer." << std::endl;
		std::abort();
	}
	Gfx::Data rendererData = std::move(rendererDataOpt.Value());


	while (true)
	{
		Time::TickStart();
		App::detail::ProcessEvents();
		if (App::detail::ShouldShutdown())
			break;
		App::detail::ImGui_NewFrame();


		Editor::RenderImGuiStuff(editorData, rendererData);

		if (!App::MainWindowMinimized())
		{
			Gfx::Draw_Params params{};

			for (auto const& [vpID, viewport] : editorData.viewports)
			{
				if (viewport.visible && !viewport.paused)
				{
					Gfx::ViewportUpdateData viewportData{};
					viewportData.id = viewport.gfxViewportRef.ViewportID();
					viewportData.width = viewport.renderWidth;
					viewportData.height = viewport.renderHeight;

					f32 aspectRatio = (f32)viewport.width / viewport.height;

					Editor::Camera const* camPtr = nullptr;
					if (viewport.cameraID == Editor::Viewport::invalidCamID)
						camPtr = &viewport.camera;
					else
					{
						for (auto const& [camID, camera] : editorData.cameras)
						{
							if (camID == viewport.cameraID)
							{
								camPtr = &camera;
								break;
							}
						}
					}
					Math::Mat4 camMat = Math::LinTran3D::Rotate_Homo(camPtr->rotation);

					camMat = Math::LinTran3D::Translate(camPtr->position) * camMat;

					Math::Mat4 test = Math::Mat4::Identity();
					test.At(2, 2) = -test.At(2, 2);
					test.At(0, 0) = -test.At(0, 0);
					//camMat = test * camMat;


					camMat = camMat.GetInverse().Value();

					camMat = Math::LinTran3D::Perspective_RH_ZO(camPtr->fov, aspectRatio, camPtr->zNear, camPtr->zFar) * camMat;

					viewportData.transform = camMat;

					params.viewportUpdates.push_back(viewportData);
				}
			}

			params.presentMainWindow = true;

			if (App::detail::MainWindowSurfaceInitializeEvent())
				params.rebuildSurface = true;

			if (App::detail::MainWindowRestoreEvent())
				params.rebuildSurface = true;

			if (App::detail::ResizeEvent())
				params.resizeEvent = true;

			rendererData.Draw(params);
		}






	}



	return 0;
}
