
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl.h"

#include "DEngine/Application/Application.hpp"
#include "DEngine/Application/detail_Application.hpp"

#include "DEngine/Gfx/Gfx.hpp"
#include "Dengine/FixedWidthTypes.hpp"

#include "DEngine/Math/Vector/Vector.hpp"

#include <optional>
#include <utility>
#include <iostream>
#include <vector>
#include <string>

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

namespace DEngine
{
	struct EditorData
	{
		struct ViewportData
		{
			bool initialized = false;
			u8 id = 0;
			bool visible = false;
			ImTextureID imguiTextureID = nullptr;
			u32 width = 0;
			u32 height = 0;
			u32 renderWidth = 0;
			u32 renderHeight = 0;
			bool currentlyResizing = false;
		};

		std::vector<ViewportData> viewportData{};
	};

	void RenderImGuiStuff(EditorData& editorData, Gfx::Data& gfx)
	{
		App::detail::ImGui_NewFrame();
		ImGui::NewFrame();

		static bool testShowWindow = true;
		ImGui::ShowDemoWindow(nullptr);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.5f, 7.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));


		if (ImGui::Begin("Testerrr", nullptr))
		{
			if (ImGui::Button("New viewport!"))
			{
				EditorData::ViewportData newViewport{};
				newViewport.id = (u8)editorData.viewportData.size();
				editorData.viewportData.push_back(newViewport);
			}
		}
		ImGui::End();


		for (auto& virtualViewport : editorData.viewportData)
		{
			std::string name = std::string("Viewport #") + std::to_string(virtualViewport.id);

			ImGui::SetNextWindowSize({ 250, 250 }, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints({ 100, 100 }, { 8192, 8192 });
			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
			if (ImGui::Begin(name.data(), nullptr, windowFlags))
			{
				ImVec2 newSize = ImGui::GetContentRegionAvail();
				newSize.x -= 25;
				newSize.y -= 25;
				if (!virtualViewport.initialized)
				{
					Gfx::ViewportRef newGfxVirtualViewport = gfx.NewViewport(virtualViewport.id);
					virtualViewport.imguiTextureID = newGfxVirtualViewport.ImGuiTexID();
					virtualViewport.width = (u32)newSize.x;
					virtualViewport.height = (u32)newSize.y;
					virtualViewport.renderWidth = (u32)newSize.x;
					virtualViewport.renderHeight = (u32)newSize.y;
					virtualViewport.initialized = true;
				}


				virtualViewport.visible = true;

				{
					// Manage the resizing of the viewport.
					if (newSize.x != virtualViewport.width || newSize.y != virtualViewport.height)
						virtualViewport.currentlyResizing = true;
					if (virtualViewport.currentlyResizing && newSize.x == virtualViewport.width && newSize.y == virtualViewport.height)
					{
						virtualViewport.renderWidth = (u32)newSize.x;
						virtualViewport.renderHeight = (u32)newSize.y;
						virtualViewport.currentlyResizing = false;
					}
					virtualViewport.width = (u32)newSize.x;
					virtualViewport.height = (u32)newSize.y;
				}


				ImGui::Image(virtualViewport.imguiTextureID, newSize);

				/*
				if (ImGui::IsItemHovered())
				{
					ImVec2 absoluteMousePos = ImGui::GetMousePos();
					ImVec2 windowPos = ImGui::GetWindowPos();

					ImVec2 test = { absoluteMousePos.x - windowPos.x, absoluteMousePos.y - windowPos.y };

					std::cout << test.x << " " << test.y << std::endl;
				}
				*/

			}
			else
			{
				virtualViewport.visible = false;
			}
			ImGui::End();
		}


		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();


		ImGui::EndFrame();
		ImGui::Render();
	}
}

#include "SDL2/SDL_main.h"
int main(int argc, char** argv)
{
	using namespace DEngine;

	App::detail::Initialize();

	EditorData editorData{};
	{
		// Initialize ImGui stuff
		IMGUI_CHECKVERSION();
		ImGuiContext* imguiContext = ImGui::CreateContext();
		ImGui::SetCurrentContext(imguiContext);
		ImGuiIO& imguiIO = ImGui::GetIO();
		imguiIO.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_DockingEnable;
		imguiIO.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_IsTouchScreen;
		imguiIO.ConfigFlags |= ImGuiConfigFlags_::ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();
		
		App::detail::ImgGui_Initialize();
	}

	// Initialize the renderer
	auto requiredInstanceExtensions = App::detail::GetRequiredVulkanInstanceExtensions();
	GfxLogger gfxLogger{};
	GfxWsiInterfacer gfxWsiInterface{};
	Gfx::InitInfo rendererInitInfo{};
	rendererInitInfo.iWsi = &gfxWsiInterface;
	rendererInitInfo.optional_iLog = &gfxLogger;
	rendererInitInfo.requiredVkInstanceExtensions = requiredInstanceExtensions.ToSpan();
	Cont::Optional<Gfx::Data> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
	if (!rendererDataOpt.hasValue())
	{
		std::cout << "Could not initialize renderer." << std::endl;
		std::abort();
	}
	Gfx::Data rendererData = std::move(rendererDataOpt.value());

	




	while (App::detail::ProcessEvents(), !App::detail::ShouldShutdown())
	{
		RenderImGuiStuff(editorData, rendererData);

		Gfx::Draw_Params params{};
		params.presentMainWindow = !App::detail::IsMinimized();

		for (auto const& item : editorData.viewportData)
		{
			if (item.visible)
			{
				Gfx::ViewportUpdateData viewportData{};
				viewportData.id = item.id;
				viewportData.width = item.renderWidth;
				viewportData.height = item.renderHeight;
				viewportData.transform = Math::Mat4::Identity();

				params.viewports.PushBack(viewportData);
			}
		}


		if (App::detail::ResizeEvent())
			params.resizeEvent = true;

		rendererData.Draw(params);
		// Update and Render additional Platform Windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

	}



	return 0;
}
