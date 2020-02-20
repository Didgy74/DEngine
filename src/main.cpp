
#include "ImGui/imgui.h"

#include "DEngine/Application/Application.hpp"
#include "DEngine/Application/detail_Application.hpp"

#include "DEngine/Gfx/Gfx.hpp"
#include "Dengine/Int.hpp"

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
			DEngine::u8 id = 0;
			ImTextureID imguiTextureID = nullptr;
			DEngine::u32 width = 0;
			DEngine::u32 height = 0;
			bool currentlyResizing = false;
			bool doneResizing = false;
		};

		std::vector<ViewportData> viewportData{};
	};

	void RenderImGuiStuff(EditorData& editorData)
	{
		App::detail::ImGui_NewFrame();

		ImGui::NewFrame();
		static bool testShowWindow = true;
		ImGui::ShowDemoWindow(nullptr);
		static bool yo = true;
		static bool yo2 = true;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.5f, 7.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		for (auto& item : editorData.viewportData)
		{
			
			

			std::string name = std::string("Viewport #") + std::to_string(item.id);

			ImGui::SetNextWindowSize({ 250, 250 }, ImGuiCond_Once);
			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
			if (ImGui::Begin(name.data(), nullptr, windowFlags))
			{
				ImVec2 size = ImGui::GetContentRegionAvail();

				if (item.currentlyResizing)
				{
					if ((u32)size.x == item.width && (u32)size.y == item.height)
						item.doneResizing = true;
				}
				else
					item.doneResizing = false;
				if ((u32)size.x != item.width || (u32)size.y != item.height)
				{
					item.currentlyResizing = true;
				}
				else
					item.currentlyResizing = false;

				item.width = (u32)size.x;
				item.height = (u32)size.y;

				ImGui::Image(item.imguiTextureID, size);
			}
			ImGui::End();
		}


		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();

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

		ImGui::StyleColorsDark();
		
		App::detail::ImgGui_Initialize();
	}

	auto requiredInstanceExtensions = App::detail::GetRequiredVulkanInstanceExtensions();


	GfxLogger gfxLogger{};
	GfxWsiInterfacer gfxWsiInterface{};

	Gfx::InitInfo rendererInitInfo{};
	rendererInitInfo.iWsi = &gfxWsiInterface;
	rendererInitInfo.optional_iLog = &gfxLogger;
	rendererInitInfo.requiredVkInstanceExtensions = requiredInstanceExtensions.span();

	Cont::Optional<Gfx::Data> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
	if (!rendererDataOpt.hasValue())
	{
		std::cout << "Could not initialize renderer." << std::endl;
		std::abort();
	}
	Gfx::Data rendererData = std::move(rendererDataOpt.value());

	

	Gfx::ViewportRef viewportA = rendererData.NewViewport();
	EditorData::ViewportData a{};
	a.id = viewportA.GetViewportID();
	a.imguiTextureID = viewportA.GetImGuiTexID();
	editorData.viewportData.push_back(a);

	Gfx::ViewportRef viewportB = rendererData.NewViewport();
	EditorData::ViewportData b{};
	b.id = viewportB.GetViewportID();
	b.imguiTextureID = viewportB.GetImGuiTexID();
	editorData.viewportData.push_back(b);

	while (App::detail::ProcessEvents(), !App::detail::ShouldShutdown())
	{
		if (!App::detail::IsMinimized())
		{
			RenderImGuiStuff(editorData);

			Gfx::Draw_Params params{};

			for (auto const& item : editorData.viewportData)
			{
				if (item.doneResizing)
				{
					Gfx::ViewportResizeEvent resizeEvent{};
					resizeEvent.viewportID = item.id;
					resizeEvent.width = item.width;
					resizeEvent.height = item.height;
					params.viewportResizeEvents.pushBack(resizeEvent);
				}
			}

			rendererData.Draw(params);
		}
	}



	return 0;
}
