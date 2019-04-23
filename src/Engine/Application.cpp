#include "Application.hpp"

#include "DEngine/Input/InputRaw.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <memory>
#include <iostream>

namespace Engine
{
	namespace Application
	{
		namespace Core
		{
			struct Data
			{
				bool isRunning = false;
				bool isMinimized = false;
				Utility::ImgDim windowSize;
				uint16_t refreshRate;
				std::string applicationAbsolutePath;
				std::string applicationName;
				Application::API3D activeAPI;

				void* windowHandle = nullptr;
			};

			static std::unique_ptr<Data> data;

			Input::Raw::Button APIKeyToButton(int32_t apiKey);

			void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
			void MousePosCallback(GLFWwindow* window, double x, double y);
			void MouseButtonCallback(GLFWwindow* window, int button, int action, int modifierBits);
		}
	}
}

void error_callback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << std::endl;;
}

namespace Engine
{
	namespace Application
	{
		namespace Core
		{
			void Initialize(API3D api)
			{
				if (!glfwInit())
				{
					// Initialization failed
					assert(false);
				}

				data = std::make_unique<Data>();
				data->isRunning = true;

				glfwSetErrorCallback(error_callback);

				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
				GLFWwindow* window = glfwCreateWindow(defaultWindowSize.width, defaultWindowSize.height, "My Title", NULL, NULL);
				if (!window)
				{
					// Window or OpenGL context creation failed
					assert(false);
				}
				glfwMakeContextCurrent(window);
				glfwSwapInterval(1);

				glfwSetKeyCallback(window, &KeyCallback);
				glfwSetCursorPosCallback(window, &MousePosCallback);
				glfwSetMouseButtonCallback(window, &MouseButtonCallback);

				data->windowHandle = window;
				data->windowSize = defaultWindowSize;
			}
		}
	}
}

Utility::ImgDim Engine::Application::GetWindowSize() { return Core::data->windowSize; }

uint16_t Engine::Application::GetRefreshRate() { return Core::data->refreshRate; }

bool Engine::Application::IsMinimized() { return Core::data->isMinimized; }

std::string Engine::Application::GetApplicationName() { return Core::data->applicationName; }

std::string Engine::Application::GetAbsolutePath() { return Core::data->applicationAbsolutePath; }

bool Engine::Application::IsRunning()
{
	if (Core::data)
		return Core::data->isRunning;
	else
		return false;
}

void* Engine::Application::Core::GetMainWindowHandle() { return data->windowHandle; }

void Engine::Application::Core::GL_SwapWindow(void* window)
{
	glfwSwapBuffers(static_cast<GLFWwindow*>(window));
}

void(*Engine::Application::Core::Vk_Test())()
{
	return (void(*)())&glfwGetInstanceProcAddress;
}

void Engine::Application::Core::UpdateEvents()
{
	glfwPollEvents();

	if (glfwWindowShouldClose(static_cast<GLFWwindow*>(data->windowHandle)))
	{
		data->isRunning = false;
	}
}

void Engine::Application::Core::Terminate()
{
	glfwTerminate();
}

void Engine::Application::Core::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Input::Raw::Button button = APIKeyToButton(key);
	if (action == GLFW_PRESS)
		Input::Core::UpdateKey(true, button);
	else if (action == GLFW_RELEASE)
		Input::Core::UpdateKey(false, button);
}

void Engine::Application::Core::MousePosCallback(GLFWwindow* window, double x, double y)
{
	Input::Core::UpdateMouseInfo(uint16_t(x), uint16_t(y));
}

void Engine::Application::Core::MouseButtonCallback(GLFWwindow* window, int button, int action, int modifierBits)
{
	if (button == GLFW_MOUSE_BUTTON_1)
	{
		if (action == GLFW_PRESS)
			Input::Core::UpdateKey(true, Input::Raw::Button::LeftMouse);
		else if (action == GLFW_RELEASE)
			Input::Core::UpdateKey(false, Input::Raw::Button::LeftMouse);
	}
	else if (button == GLFW_MOUSE_BUTTON_2)
	{
		if (action == GLFW_PRESS)
			Input::Core::UpdateKey(true, Input::Raw::Button::RightMouse);
		else if (action == GLFW_RELEASE)
			Input::Core::UpdateKey(false, Input::Raw::Button::RightMouse);
	}
}

Engine::Input::Raw::Button Engine::Application::Core::APIKeyToButton(int32_t apiKey)
{
	using namespace Input::Raw;
	switch (apiKey)
	{
	case GLFW_KEY_SPACE:
		return Button::Space;

	case GLFW_KEY_LEFT_CONTROL:
		return Button::LeftCtrl;

	case GLFW_KEY_UP:
		return Button::Up;
	case GLFW_KEY_DOWN:
		return Button::Down;
	case GLFW_KEY_LEFT:
		return Button::Left;
	case GLFW_KEY_RIGHT:
		return Button::Right;

	case GLFW_KEY_A:
		return Button::A;
	case GLFW_KEY_B:
		return Button::B;
	case GLFW_KEY_C:
		return Button::C;
	case GLFW_KEY_D:
		return Button::D;
	case GLFW_KEY_E:
		return Button::E;
	case GLFW_KEY_F:
		return Button::F;
	case GLFW_KEY_G:
		return Button::G;
	case GLFW_KEY_H:
		return Button::H;
	case GLFW_KEY_I:
		return Button::I;
	case GLFW_KEY_J:
		return Button::J;
	case GLFW_KEY_K:
		return Button::K;
	case GLFW_KEY_L:
		return Button::L;
	case GLFW_KEY_M:
		return Button::M;
	case GLFW_KEY_N:
		return Button::N;
	case GLFW_KEY_O:
		return Button::O;
	case GLFW_KEY_P:
		return Button::P;
	case GLFW_KEY_Q:
		return Button::Q;
	case GLFW_KEY_R:
		return Button::R;
	case GLFW_KEY_S:
		return Button::S;
	case GLFW_KEY_T:
		return Button::T;
	case GLFW_KEY_U:
		return Button::U;
	case GLFW_KEY_V:
		return Button::V;
	case GLFW_KEY_W:
		return Button::W;
	case GLFW_KEY_X:
		return Button::X;
	case GLFW_KEY_Y:
		return Button::Y;
	case GLFW_KEY_Z:
		return Button::Z;
	default:
		return Button::Undefined;
	}
	return Button();
}