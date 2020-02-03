
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

//#include "imgui.h"
//#include "imgui_impl_sdl.h"
//#include "imgui_impl_vulkan.h"

#include "DEngine/Gfx/Gfx.hpp"

#include <optional>
#include <utility>
#include <iostream>

enum class OS
{
	Windows,
	Linux,
	Android
};

enum class OSType
{
	Desktop,
	Mobile
};

#if defined(_WIN32) || defined(_WIN64)
constexpr OS targetOS = OS::Windows;
constexpr OSType targetOSType = OSType::Desktop;
#elif defined(__ANDROID__)
constexpr OS targetOS = OS::Android;
constexpr OSType targetOsType = OSType::Mobile;
#elif defined(__GNUC__)
constexpr OS targetOS = OS::Linux;
constexpr OSType targetOSType = OSType::Desktop;
#endif



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
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, msg);
	}

	virtual ~GfxLogger() override
	{

	}
};

int main(int argc, char** argv)
{
	using namespace DEngine;

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// Setup window
	Uint32 sdlWindowFlags = 0;
	sdlWindowFlags |= SDL_WINDOW_VULKAN;
	sdlWindowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
	int windowWidth = 0;
	int windowHeight = 0;
	if constexpr (targetOSType == OSType::Desktop)
	{
		sdlWindowFlags |= SDL_WINDOW_RESIZABLE;
		windowWidth = 1280;
		windowHeight = 720;
	}
	else if (targetOSType == OSType::Mobile)
	{
		sdlWindowFlags |= SDL_WINDOW_FULLSCREEN;
		windowWidth = 400;
		windowHeight = 400;
	}
	

	SDL_Window* sdlWindow = SDL_CreateWindow("Dear ImGui SDL2+Vulkan example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, sdlWindowFlags);
	assert(sdlWindow != nullptr);
	if constexpr (targetOSType == OSType::Desktop)
	{
		SDL_SetWindowMinimumSize(sdlWindow, 1280, 720);
	}
	

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

	Cont::Optional<Gfx::Data> rendererDataOpt = Gfx::Initialize(rendererInitInfo);
	if (!rendererDataOpt.hasValue() == true)
	{
		std::cout << "Could not initialize renderer." << std::endl;
		std::abort();
	}
	auto& rendererData = rendererDataOpt.value();

	bool shutDownProgram = false;

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
		}
		if (shutDownProgram == true)
		{
			break;
		}


		Gfx::Draw(rendererData, 0.0f);

	}


	return 0;
}
