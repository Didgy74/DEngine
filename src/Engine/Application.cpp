#include "Application.hpp"

#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"
#include "SDL2/SDL_vulkan.h"

#include "Input/InputRaw.hpp"

#include <iostream>
#include <string>
#include <cassert>
#include <memory>

namespace Engine
{
	namespace Application
	{
		namespace Core
		{
			struct Data
			{
				SDL_Window* sdlWindow = nullptr;
				bool isRunning = false;
				bool isMinimized = false;
				Utility::ImgDim windowSize;
				uint16_t refreshRate;
				std::string applicationAbsolutePath;
				std::string applicationName;

				Application::API3D activeAPI;

				SDL_GLContext glContext = nullptr;
			};

			static std::unique_ptr<Data> data;

			Input::Raw::Button SDLKeyToButton(size_t sdlKey);
		}
	}
}

Utility::ImgDim Engine::Application::GetWindowSize() { return Core::data->windowSize; }

Utility::ImgDim Engine::Application::GetViewportSize() { return IsMinimized() ? Utility::ImgDim{ 0, 0 } : GetWindowSize(); }

uint16_t Engine::Application::GetRefreshRate() { return Core::data->refreshRate; }

bool Engine::Application::IsMinimized() { return Core::data->isMinimized; }

std::string Engine::Application::GetApplicationName() { return Core::data->applicationName; }

std::string Engine::Application::GetAbsolutePath() { return Core::data->applicationAbsolutePath; }

void Engine::Application::Core::UpdateEvents()
{
	Input::Core::ClearValues();

	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_KEYDOWN && e.key.repeat == false)
			Input::Core::UpdateSingle(true, SDLKeyToButton(e.key.keysym.sym));
		else if (e.type == SDL_KEYUP && e.key.repeat == false)
			Input::Core::UpdateSingle(false, SDLKeyToButton(e.key.keysym.sym));

		else if (e.type == SDL_MOUSEMOTION)
			Input::Core::UpdateMouseInfo(e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel);
		else if (e.type == SDL_MOUSEBUTTONDOWN)
		{
			if (e.button.button == SDL_BUTTON_LEFT)
				Input::Core::UpdateSingle(true, Input::Raw::Button::LeftMouse);
			else if (e.button.button == SDL_BUTTON_RIGHT)
				Input::Core::UpdateSingle(true, Input::Raw::Button::RightMouse);
		}
		else if (e.type == SDL_MOUSEBUTTONUP)
		{
			if (e.button.button == SDL_BUTTON_LEFT)
				Input::Core::UpdateSingle(false, Input::Raw::Button::LeftMouse);
			else if (e.button.button == SDL_BUTTON_RIGHT)
				Input::Core::UpdateSingle(false, Input::Raw::Button::RightMouse);
		}

		else if (e.type == SDL_QUIT)
		{
			data->isRunning = false;
		}
		else if (e.type == SDL_WINDOWEVENT)
		{
			if (e.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				data->windowSize.width = static_cast<Utility::ImgDim::ValueType>(e.window.data1);
				data->windowSize.height = static_cast<Utility::ImgDim::ValueType>(e.window.data2);
				//Renderer::ViewportResized();
			}
			else if (e.window.event == SDL_WINDOWEVENT_MINIMIZED)
			{
				data->isMinimized = true;
			}
			else if (e.window.event == SDL_WINDOWEVENT_RESTORED)
			{
				data->isMinimized = false;
			}
		}
	}
}

bool Engine::Application::IsRunning() 
{
	if (Core::data)
		return Core::data->isRunning;
	else
		return false;
}

void Engine::Application::Core::Initialize(API3D api)
{
	data = std::make_unique<Data>();

	data->activeAPI = api;

	data->applicationName = defaultApplicationName;
	data->windowSize = defaultWindowSize;


	// Initialize SDL
	auto sdlStatus = SDL_Init(SDL_INIT_VIDEO);
	if (sdlStatus != 0)
    {
	    std::cerr << "Could not initialize SDL." << std::endl;
	    std::exit(0);
    }

	// Grab application path
	auto path = SDL_GetBasePath();
	assert(path != nullptr);
	data->applicationAbsolutePath = path;
	SDL_free(path);

	// Set up window
	SDL_Window* windowTest;
	if (data->activeAPI == API3D::OpenGL)
	{
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

		auto result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		if (result != 0)
        {
		    std::cerr << "Error. Could not find OpenGL 3.x support." << std::endl;
		    std::cerr << SDL_GetError() << std::endl;
		    std::exit(0);
        }
		result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		if (result != 0)
        {
            std::cerr << "Error. Could not find OpenGL 3.3 support." << std::endl;
            std::exit(0);
        }

		windowTest = SDL_CreateWindow(defaultApplicationName.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, data->windowSize.width, data->windowSize.height, SDL_WINDOW_OPENGL);
		assert(windowTest != nullptr);
		data->glContext = SDL_GL_CreateContext(windowTest);
		assert(data->glContext != nullptr);
	}
	else if (data->activeAPI == API3D::Vulkan)
	{
		windowTest = SDL_CreateWindow(defaultApplicationName.data(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, data->windowSize.width, data->windowSize.height, SDL_WINDOW_VULKAN);
		assert(windowTest != nullptr);
	}

	data->sdlWindow = windowTest;


	if (data->activeAPI == API3D::OpenGL)
		data->glContext = SDL_GL_CreateContext(data->sdlWindow);

	SDL_SetWindowMinimumSize(data->sdlWindow, minimumWindowSize.width, minimumWindowSize.height);

	// Get refresh rate of monitor.
	SDL_DisplayMode displayMode;
	auto getCurrentDisplayModeTest = SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(data->sdlWindow), &displayMode);
	assert(getCurrentDisplayModeTest == 0);
	data->refreshRate = static_cast<uint16_t>(displayMode.refresh_rate);

	data->isRunning = true;
}

void Engine::Application::Core::Terminate()
{
	if (data->activeAPI == API3D::OpenGL && data->glContext)
	{
		SDL_GL_DeleteContext(data->glContext);
		data->glContext = nullptr;
	}

	if (data->sdlWindow)
	{
		SDL_DestroyWindow(data->sdlWindow);
		data->sdlWindow = nullptr;
	}
	SDL_Quit();

	data = nullptr;
}

void* Engine::Application::Core::GetSDLWindowHandle() { return data->sdlWindow; }

void Engine::Application::Core::GL_SwapWindow(void* sdlWindow)
{
	SDL_GL_SwapWindow(static_cast<SDL_Window*>(sdlWindow));
}

std::vector<const char*> Engine::Application::Core::Vulkan_GetInstanceExtensions(void* windowHandle)
{
	uint32_t extensionCount;
	auto result = SDL_Vulkan_GetInstanceExtensions(static_cast<SDL_Window*>(windowHandle), &extensionCount, nullptr);
	assert(result == SDL_TRUE);
	std::vector<const char*> requiredExtensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(static_cast<SDL_Window*>(windowHandle), &extensionCount, requiredExtensions.data());
	return requiredExtensions;
}

void Engine::Application::Core::Vulkan_CreateSurface(void* windowHandle, void* vkInstance, void* surfaceHandle)
{
	auto result = SDL_Vulkan_CreateSurface(static_cast<SDL_Window*>(windowHandle), *static_cast<VkInstance*>(vkInstance), static_cast<VkSurfaceKHR*>(surfaceHandle));
	assert(result == SDL_TRUE);
}

Engine::Input::Raw::Button Engine::Application::Core::SDLKeyToButton(size_t keySymbol)
{
	using Input::Raw::Button;
	switch (keySymbol)
	{
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


	case SDLK_UP:
		return Button::Up;
	case SDLK_DOWN:
		return Button::Down;
	case SDLK_LEFT:
		return Button::Left;
	case SDLK_RIGHT:
		return Button::Right;

	case SDLK_SPACE:
		return Button::Space;

	case SDLK_LCTRL:
		return Button::Ctrl;

	default:
		return Button::Undefined;
	}
}