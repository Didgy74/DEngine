#include "DEngine/Application.hpp"
#include "detail_Application.hpp"

#include "DEngine/InputRaw.hpp"

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl.h"

#include <iostream>

namespace DEngine::Application::detail
{
	SDL_Window* mainWindow = nullptr;
	bool isMinimized = false;
	bool shouldShutdown = false;
	bool isRestored = false;
	bool resizeEvent = false;

	Input::Raw::Button SDLKeyboardKeyToRawButton(i32 input);
	Input::Raw::Button SDLMouseKeyToRawButton(i32 input);
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
	if constexpr (targetOSType == Platform::Desktop)
	{
		SDL_SetWindowMinimumSize(detail::mainWindow, 800, 600);
	}

	return true;
}

void DEngine::Application::detail::ImgGui_Initialize()
{
	ImGui_ImplSDL2_InitForVulkan(detail::mainWindow);
}

void DEngine::Application::detail::ProcessEvents()
{
	shouldShutdown = false;
	isRestored = false;
	resizeEvent = false;

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
				resizeEvent = true;
			}
			else if (event.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_MINIMIZED)
			{
				isMinimized = true;
			}
			else if (event.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_RESTORED)
			{
				isMinimized = false;
				isRestored = true;
			}
		}
		else if (event.type == SDL_EventType::SDL_KEYDOWN)
		{
			Input::Core::UpdateKey(SDLKeyboardKeyToRawButton(event.key.keysym.scancode), true);
		}
		else if (event.type == SDL_EventType::SDL_KEYUP)
		{
			Input::Core::UpdateKey(SDLKeyboardKeyToRawButton(event.key.keysym.scancode), false);
		}
		else if (event.type == SDL_EventType::SDL_MOUSEMOTION)
		{
			Input::Core::UpdateMouseInfo(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
			std::cout << event.motion.xrel << " , " << event.motion.yrel << std::endl;
		}
		else if (event.type == SDL_EventType::SDL_MOUSEBUTTONDOWN)
		{
			Input::Core::UpdateKey(SDLMouseKeyToRawButton(event.button.button), true);
		}
		else if (event.type == SDL_EventType::SDL_MOUSEBUTTONUP)
		{
			Input::Core::UpdateKey(SDLMouseKeyToRawButton(event.button.button), false);
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

bool DEngine::Application::detail::IsRestored()
{
	return isRestored;
}

bool DEngine::Application::detail::ResizeEvent()
{
	return resizeEvent;
}

void DEngine::Application::detail::ImGui_NewFrame()
{
	ImGui_ImplSDL2_NewFrame(detail::mainWindow);
}

void DEngine::Application::Log(char const* msg)
{
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "DENGINE GFX ERROR: %s\n", msg);
}

void DEngine::Application::SetRelativeMouseMode(bool enabled)
{
	int errorCode = SDL_SetRelativeMouseMode(static_cast<SDL_bool>(enabled));
	if (errorCode != 0)
		throw std::runtime_error("Failedto set relative mouse mode");
}

DEngine::Cont::FixedVector<char const*, 5> DEngine::Application::detail::GetRequiredVulkanInstanceExtensions()
{
	unsigned int count = 0;
	SDL_bool result = SDL_Vulkan_GetInstanceExtensions(detail::mainWindow, &count, nullptr);
	if (result == SDL_FALSE)
		throw std::runtime_error("DEngine::Application: Unable to grab required Vulkan instance extensions.");

	Cont::FixedVector<char const*, 5> ptrs{};
	ptrs.Resize(count);

	SDL_Vulkan_GetInstanceExtensions(detail::mainWindow, &count, ptrs.Data());

	return ptrs;
}

bool DEngine::Application::detail::CreateVkSurface(u64 vkInstance, void const* vkAllocationCallbacks, void* userData, u64* vkSurface)
{
	SDL_Window* sdlWindow = detail::mainWindow;

	SDL_bool result = SDL_Vulkan_CreateSurface(sdlWindow, (VkInstance)vkInstance, reinterpret_cast<VkSurfaceKHR*>(vkSurface));

	return result;
}

DEngine::Input::Raw::Button DEngine::Application::detail::SDLMouseKeyToRawButton(i32 input)
{
	using namespace Input::Raw;
	switch (input)
	{
	case SDL_BUTTON_LEFT:
		return Button::LeftMouse;
	case SDL_BUTTON_RIGHT:
		return Button::RightMouse;
	default:
		return Button::Undefined;
	}
}

DEngine::Input::Raw::Button DEngine::Application::detail::SDLKeyboardKeyToRawButton(i32 input)
{
	using namespace Input::Raw;
	switch (input)
	{
	case SDL_SCANCODE_SPACE:
		return Button::Space;

	case SDL_SCANCODE_LCTRL:
		return Button::LeftCtrl;

	case SDL_SCANCODE_UP:
		return Button::Up;
	case SDL_SCANCODE_DOWN:
		return Button::Down;
	case SDL_SCANCODE_LEFT:
		return Button::Left;
	case SDL_SCANCODE_RIGHT:
		return Button::Right;

	case SDL_SCANCODE_A:
		return Button::A;
	case SDL_SCANCODE_B:
		return Button::B;
	case SDL_SCANCODE_C:
		return Button::C;
	case SDL_SCANCODE_D:
		return Button::D;
	case SDL_SCANCODE_E:
		return Button::E;
	case SDL_SCANCODE_F:
		return Button::F;
	case SDL_SCANCODE_G:
		return Button::G;
	case SDL_SCANCODE_H:
		return Button::H;
	case SDL_SCANCODE_I:
		return Button::I;
	case SDL_SCANCODE_J:
		return Button::J;
	case SDL_SCANCODE_K:
		return Button::K;
	case SDL_SCANCODE_L:
		return Button::L;
	case SDL_SCANCODE_M:
		return Button::M;
	case SDL_SCANCODE_N:
		return Button::N;
	case SDL_SCANCODE_O:
		return Button::O;
	case SDL_SCANCODE_P:
		return Button::P;
	case SDL_SCANCODE_Q:
		return Button::Q;
	case SDL_SCANCODE_R:
		return Button::R;
	case SDL_SCANCODE_S:
		return Button::S;
	case SDL_SCANCODE_T:
		return Button::T;
	case SDL_SCANCODE_U:
		return Button::U;
	case SDL_SCANCODE_V:
		return Button::V;
	case SDL_SCANCODE_W:
		return Button::W;
	case SDL_SCANCODE_X:
		return Button::X;
	case SDL_SCANCODE_Y:
		return Button::Y;
	case SDL_SCANCODE_Z:
		return Button::Z;
	default:
		return Button::Undefined;
	}
}