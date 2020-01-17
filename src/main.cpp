
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

//#include "imgui.h"
//#include "imgui_impl_sdl.h"
//#include "imgui_impl_vulkan.h"

#include "stuff.hpp"

#include "DEngine/Gfx/Gfx.hpp"

#include <optional>
#include <utility>
#include <iostream>

bool CreateVkSurface(DEngine::u64 vkInstance, void* userData, DEngine::u64* vkSurface)
{
	SDL_Window* sdlWindow = reinterpret_cast<SDL_Window*>(userData);

	SDL_bool result = SDL_Vulkan_CreateSurface(sdlWindow, (VkInstance)vkInstance, reinterpret_cast<VkSurfaceKHR*>(vkSurface));

	return result;
}

class GfxLogger : public DEngine::Gfx::ILog
{
public:
	virtual void log(const char* msg) const override 
	{
		std::cout << msg << std::endl;
	}

	virtual ~GfxLogger() override
	{

	}
};

int main(int argc, char** argv)
{
	constexpr auto test = alignof(std::uint32_t);

	using namespace DEngine;

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// Setup window
	SDL_WindowFlags sdlWindowFlags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window* sdlWindow = SDL_CreateWindow("Dear ImGui SDL2+Vulkan example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, sdlWindowFlags);
	assert(sdlWindow != nullptr);
	SDL_SetWindowMinimumSize(sdlWindow, 1280, 720);

	unsigned int requiredInstanceExtensionCount = 0;
	SDL_bool sdlGetInstanceExtensionsResult = SDL_Vulkan_GetInstanceExtensions(sdlWindow, &requiredInstanceExtensionCount, nullptr);
	assert(sdlGetInstanceExtensionsResult == SDL_TRUE);
	std::vector<const char*> requiredInstanceExtensions(requiredInstanceExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(sdlWindow, &requiredInstanceExtensionCount, requiredInstanceExtensions.data());

	GfxLogger gfxLogger{};

	Gfx::InitInfo rendererInitInfo{};
	rendererInitInfo.createVkSurfaceUserData = sdlWindow;
	rendererInitInfo.createVkSurfacePFN = CreateVkSurface;
	rendererInitInfo.requiredVkInstanceExtensions = Containers::Span<const char*>(requiredInstanceExtensions.data(), requiredInstanceExtensionCount);
	rendererInitInfo.iLog = &gfxLogger;

	auto rendererDataOpt = Gfx::Initialize(rendererInitInfo);
	if (!rendererDataOpt.hasValue() == true)
	{
		std::cout << "Could not initialize renderer." << std::endl;
		std::abort();
	}
	auto& rendererData = rendererDataOpt.value();

	bool show_demo_window = true;

	bool shutDownProgram = false;

	float scale = 1.0f;
	bool upIsBeingHeld = false;
	bool downIsBeingHeld = false;

	while (!shutDownProgram)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			//ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_EventType::SDL_QUIT)
				shutDownProgram = true;
			else if (event.type == SDL_EventType::SDL_WINDOWEVENT && event.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_RESIZED && event.window.windowID == SDL_GetWindowID(sdlWindow))
			{
				int g_SwapChainResizeWidth = (int)event.window.data1;
				int g_SwapChainResizeHeight = (int)event.window.data2;
			}
			else if (event.type == SDL_EventType::SDL_KEYDOWN)
			{
				if (event.key.keysym.scancode == SDL_Scancode::SDL_SCANCODE_UP)
				{
					upIsBeingHeld = true;
				}
				else if (event.key.keysym.scancode == SDL_Scancode::SDL_SCANCODE_DOWN)
				{
					downIsBeingHeld = true;
				}
			}
			else if (event.type == SDL_EventType::SDL_KEYUP)
			{
				if (event.key.keysym.scancode == SDL_Scancode::SDL_SCANCODE_UP)
				{
					upIsBeingHeld = false;
				}
				if (event.key.keysym.scancode == SDL_Scancode::SDL_SCANCODE_DOWN)
				{
					downIsBeingHeld = false;
				}
			}
		}
		if (shutDownProgram == true)
		{
			break;
		}

		if (upIsBeingHeld == true)
			scale += 0.005f;
		if (downIsBeingHeld == true)
			scale -= 0.005f;

		std::cout << "Scale: " << scale << std::endl;

		Gfx::Draw(rendererData, scale);


	}


	return 0;
}