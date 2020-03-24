#define DENGINE_APPLICATION_BUTTON_COUNT
#include "DEngine/Application.hpp"
#include "detail_Application.hpp"

#include "DEngine/FixedWidthTypes.hpp"

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl.h"

#include <iostream>
#include <chrono>

namespace DEngine::Application::detail
{
	static bool buttonValues[(int)Button::COUNT] = {};
	static std::chrono::high_resolution_clock::time_point buttonHeldStart[(int)Button::COUNT] = {};
	static f32 buttonHeldDuration[(int)Button::COUNT] = {};
	static InputEvent buttonEvents[(int)Button::COUNT] = {};
	static u32 mousePosition[2] = {};
	static i32 mouseDelta[2] = {};
	static void UpdateButton(Button button, bool pressed, std::chrono::high_resolution_clock::time_point now);
	static void UpdateMouse(u32 posX, u32 posY, i32 deltaX, i32 deltaY);
	static void UpdateMouse(u32 posX, u32 posY);
	static Button SDLKeyboardKeyToRawButton(i32 input);
	static Button SDLMouseKeyToRawButton(i32 input);

	SDL_Window* mainWindow = nullptr;
	bool isMinimized = false;
	bool shouldShutdown = false;
	bool isRestored = false;
	bool resizeEvent = false;


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
	i32 sdlWindowFlags = 0;
	sdlWindowFlags |= SDL_WINDOW_VULKAN;
	//sdlWindowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;
	sdlWindowFlags |= SDL_WINDOW_RESIZABLE;
	i32 windowWidth = 0;
	i32 windowHeight = 0;
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
	detail::mainWindow = SDL_CreateWindow(
		"Dear ImGui SDL2+Vulkan example", 
		SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED, 
		windowWidth, 
		windowHeight,
		sdlWindowFlags);
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
	auto now = std::chrono::high_resolution_clock::now();
	// Clear the input-events array
	for (auto& item : detail::buttonEvents)
		item = InputEvent::Unchanged;
	for (auto& item : detail::mouseDelta)
		item = 0;
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
		else if (event.type == SDL_EventType::SDL_WINDOWEVENT && event.window.windowID)
		{
			if (event.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_CLOSE)
			{
				shouldShutdown = true;
			}
			else if (event.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_RESIZED)
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
			if (event.key.repeat == 0)
				detail::UpdateButton(SDLKeyboardKeyToRawButton(event.key.keysym.sym), true, now);
		}
		else if (event.type == SDL_EventType::SDL_KEYUP)
		{
			detail::UpdateButton(SDLKeyboardKeyToRawButton(event.key.keysym.sym), false, now);
		}
		else if (event.type == SDL_EventType::SDL_MOUSEMOTION)
		{
			detail::UpdateMouse(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
		}
		else if (event.type == SDL_EventType::SDL_MOUSEBUTTONDOWN)
		{
			detail::UpdateButton(SDLMouseKeyToRawButton(event.button.button), true, now);
		}
		else if (event.type == SDL_EventType::SDL_MOUSEBUTTONUP)
		{
			detail::UpdateButton(SDLMouseKeyToRawButton(event.button.button), false, now);
		}
	}


	// Calculate duration for each button being held.
	for (uSize i = 0; i < (uSize)Button::COUNT; i += 1)
	{
		if (detail::buttonValues[i])
		{
			buttonHeldDuration[i] = std::chrono::duration<f32>(now - buttonHeldStart[i]).count();
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

bool DEngine::Application::ButtonValue(Button input)
{
	return detail::buttonValues[(int)input];
}

DEngine::Application::InputEvent DEngine::Application::ButtonEvent(Button input)
{
	return detail::buttonEvents[(int)input];
}

DEngine::f32 DEngine::Application::ButtonDuration(Button input)
{
	return detail::buttonHeldDuration[(int)input];
}

DEngine::Cont::Array<DEngine::i32, 2> DEngine::Application::MouseDelta()
{
	return { detail::mouseDelta[0], detail::mouseDelta[1] };
}

void DEngine::Application::Log(char const* msg)
{
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "DENGINE GFX ERROR: %s\n", msg);
}

void DEngine::Application::SetRelativeMouseMode(bool enabled)
{
	std::cout << "Set mouse mode: " << enabled << std::endl;
	//i32 errorCode = SDL_SetRelativeMouseMode(static_cast<SDL_bool>(enabled));
	//if (errorCode != 0)
		//throw std::runtime_error("Failedto set relative mouse mode");
}

DEngine::Cont::FixedVector<char const*, 5> DEngine::Application::detail::GetRequiredVulkanInstanceExtensions()
{
	u32 count = 0;
	SDL_bool result = SDL_Vulkan_GetInstanceExtensions(detail::mainWindow, &count, nullptr);
	if (result == SDL_FALSE)
		throw std::runtime_error("DEngine::Application: Unable to grab required Vulkan instance extensions.");

	Cont::FixedVector<char const*, 5> ptrs{};
	ptrs.Resize(count);

	SDL_Vulkan_GetInstanceExtensions(detail::mainWindow, &count, ptrs.Data());

	return ptrs;
}

bool DEngine::Application::detail::CreateVkSurface(
	u64 vkInstance, 
	void const* vkAllocationCallbacks, 
	void* userData, 
	u64* vkSurface)
{
	SDL_Window* sdlWindow = detail::mainWindow;

	SDL_bool result = SDL_Vulkan_CreateSurface(sdlWindow, (VkInstance)vkInstance, reinterpret_cast<VkSurfaceKHR*>(vkSurface));

	return result;
}

static void DEngine::Application::detail::UpdateButton(
	Button button, 
	bool pressed, 
	std::chrono::high_resolution_clock::time_point now)
{
	detail::buttonValues[(int)button] = pressed;
	detail::buttonEvents[(int)button] = pressed ? InputEvent::Pressed : InputEvent::Unpressed;

	if (pressed)
	{
		detail::buttonHeldStart[(int)button] = now;
	}
	else
	{
		detail::buttonHeldDuration[(int)button] = 0.f;
		detail::buttonHeldStart[(int)button] = std::chrono::high_resolution_clock::time_point();
	}
}

static void DEngine::Application::detail::UpdateMouse(u32 posX, u32 posY, i32 deltaX, i32 deltaY)
{
	detail::mouseDelta[0] = deltaX;
	detail::mouseDelta[1] = deltaY;

	detail::mousePosition[0] = posX;
	detail::mousePosition[1] = posY;
}

static void DEngine::Application::detail::UpdateMouse(u32 posX, u32 posY)
{
	detail::mouseDelta[0] = (i32)posX - (i32)detail::mousePosition[0];
	detail::mouseDelta[1] = (i32)posY - (i32)detail::mousePosition[1];

	detail::mousePosition[0] = posX;
	detail::mousePosition[1] = posY;
}

DEngine::Application::Button DEngine::Application::detail::SDLMouseKeyToRawButton(i32 input)
{
	switch (input)
	{
	case SDL_BUTTON_LEFT:
		return Button::LeftMouse;
	case SDL_BUTTON_RIGHT:
		return Button::RightMouse;
	}

	return Button::Undefined;
}

DEngine::Application::Button DEngine::Application::detail::SDLKeyboardKeyToRawButton(i32 input)
{
	switch (input)
	{
	case SDLK_ESCAPE:
		return Button::Escape;

	case SDLK_AC_BACK:
		return Button::Back;

	case SDLK_SPACE:
		return Button::Space;

	case SDLK_LCTRL:
		return Button::LeftCtrl;

	case SDLK_UP:
		return Button::Up;
	case SDLK_DOWN:
		return Button::Down;
	case SDLK_LEFT:
		return Button::Left;
	case SDLK_RIGHT:
		return Button::Right;

	case SDLK_a:
		return Button::A;
	case SDLK_b:
		return Button::B;
	case SDLK_c:
		return Button::C;
	case SDLK_d:
		return Button::D;
	case SDLK_e:
		return Button::E;
	case SDLK_f:
		return Button::F;
	case SDLK_g:
		return Button::G;
	case SDLK_h:
		return Button::H;
	case SDLK_i:
		return Button::I;
	case SDLK_j:
		return Button::J;
	case SDLK_k:
		return Button::K;
	case SDLK_l:
		return Button::L;
	case SDLK_m:
		return Button::M;
	case SDLK_n:
		return Button::N;
	case SDLK_o:
		return Button::O;
	case SDLK_p:
		return Button::P;
	case SDLK_q:
		return Button::Q;
	case SDLK_r:
		return Button::R;
	case SDLK_s:
		return Button::S;
	case SDLK_t:
		return Button::T;
	case SDLK_u:
		return Button::U;
	case SDLK_v:
		return Button::V;
	case SDLK_w:
		return Button::W;
	case SDLK_x:
		return Button::X;
	case SDLK_y:
		return Button::Y;
	case SDLK_z:
		return Button::Z;
	}

	return Button::Undefined;
}