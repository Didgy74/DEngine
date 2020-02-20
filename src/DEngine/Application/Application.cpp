#include "DEngine/Application/Application.hpp"
#include "detail_Application.hpp"

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl.h"

#include <iostream>
#include <assert.h>

namespace DEngine::Application::detail
{
	SDL_Window* mainWindow = nullptr;
	bool isMinimized = false;
	bool shouldShutdown = false;
}

bool DEngine::Application::detail::Initialize()
{
	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return false;
	}

	// Setup window
	Uint32 sdlWindowFlags = 0;
	sdlWindowFlags |= SDL_WINDOW_VULKAN;
	sdlWindowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
	sdlWindowFlags |= SDL_WINDOW_RESIZABLE;
	int windowWidth = 0;
	int windowHeight = 0;
	if constexpr (targetOSType == Platform::Desktop)
	{
		windowWidth = 1280;
		windowHeight = 720;
	}
	else if (targetOSType == Platform::Mobile)
	{
		sdlWindowFlags |= SDL_WINDOW_FULLSCREEN;
		windowWidth = 400;
		windowHeight = 400;
	}
	detail::mainWindow = SDL_CreateWindow("Dear ImGui SDL2+Vulkan example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, sdlWindowFlags);
	assert(detail::mainWindow != nullptr);
	if constexpr (targetOSType == Platform::Desktop)
	{
		SDL_SetWindowMinimumSize(detail::mainWindow, 1280, 720);
	}

	return true;
}

void DEngine::Application::detail::ImgGui_Initialize()
{
	assert(detail::mainWindow);
	ImGui_ImplSDL2_InitForVulkan(detail::mainWindow);
}

void DEngine::Application::detail::ProcessEvents()
{
	shouldShutdown = false;

	SDL_Event event{};
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_EventType::SDL_QUIT)
		{
			shouldShutdown = true;
		}
		else if (event.type == SDL_EventType::SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(detail::mainWindow))
		{
			if (event.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_RESIZED)
			{
				Log("Resize event");
				//rendererData.resizeEvent = true;
			}
			else if (event.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_MINIMIZED)
			{
				Log("Window minimized event");
				isMinimized = true;
			}
			else if (event.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_RESTORED)
			{
				Log("Window restored event");
				isMinimized = false;
			}
		}
	}
}

bool DEngine::Application::detail::ShouldShutdown()
{
	return shouldShutdown;
}

bool DEngine::Application::detail::IsMinimized()
{
	return isMinimized;
}

void DEngine::Application::detail::ImGui_NewFrame()
{
	ImGui_ImplSDL2_NewFrame(detail::mainWindow);
}

void DEngine::Application::Log(char const* msg)
{
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "DENGINE GFX ERROR: %s\n", msg);
}

DEngine::Cont::FixedVector<char const*, 5> DEngine::Application::detail::GetRequiredVulkanInstanceExtensions()
{
	unsigned int count = 0;
	SDL_bool result = SDL_Vulkan_GetInstanceExtensions(detail::mainWindow, &count, nullptr);
	if (result == SDL_FALSE)
		throw std::runtime_error("DEngine::Application: Unable to grab required Vulkan instance extensions.");

	Cont::FixedVector<char const*, 5> ptrs{};
	ptrs.resize(count);

	SDL_Vulkan_GetInstanceExtensions(detail::mainWindow, &count, ptrs.data());

	return ptrs;
}

bool DEngine::Application::detail::CreateVkSurface(u64 vkInstance, void const* vkAllocationCallbacks, void* userData, u64* vkSurface)
{
	SDL_Window* sdlWindow = detail::mainWindow;

	SDL_bool result = SDL_Vulkan_CreateSurface(sdlWindow, (VkInstance)vkInstance, reinterpret_cast<VkSurfaceKHR*>(vkSurface));

	return result;
}
