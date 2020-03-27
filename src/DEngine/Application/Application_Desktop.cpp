#include "DEngine/Application.hpp"
#include "detail_Application.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
#endif

#include <iostream>

namespace DEngine::Application::detail
{
	static GLFWwindow* mainWindow = nullptr;
	struct WindowData
	{
		std::chrono::high_resolution_clock::time_point now = {};
	};
	static WindowData mainWindowData{};

	static void Backend_GLFW_ErrorCallback(
		int error, 
		const char* description);
	static Button Backend_GLFW_KeyboardKeyToRawButton(i32 input);
	static Button Backend_GLFW_MouseButtonToRawButton(i32 input);
	static void Backend_GLFW_KeyboardKeyCallback(
		GLFWwindow* window, 
		int key, 
		int scancode, 
		int action, 
		int mods);
	static void Backend_GLFW_MouseButtonCallback(
		GLFWwindow* window,
		int button,
		int action,
		int mods);
	static void Backend_GLFW_MousePosCallback(
		GLFWwindow* window,
		double xpos,
		double ypos);
	static void Backend_GLFW_WindowPosCallback(
		GLFWwindow* window, 
		int xpos,
		int ypos);
	static void Backend_GLFW_WindowSizeCallback(
		GLFWwindow* window,
		int width,
		int height);
	static void Backend_GLFW_WindowFramebufferSizeCallback(
		GLFWwindow* window, 
		int width, 
		int height);
	static void Backend_GLFW_WindowCloseCallback(
		GLFWwindow* window);
	static void Backend_GLFW_WindowFocusCallback(
		GLFWwindow* window, 
		int focused);
	static void Backend_GLFW_WindowMinimizeCallback(
		GLFWwindow* window, 
		int iconified);
}

void DEngine::Application::Log(char const* msg)
{
	std::cout << msg << std::endl;
}

static void DEngine::Application::detail::Backend_GLFW_ErrorCallback(
	int error, 
	const char* description)
{
	fputs(description, stderr);
}

bool DEngine::Application::detail::Backend_Initialize()
{
	glfwSetErrorCallback(Backend_GLFW_ErrorCallback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	detail::mainWindow = glfwCreateWindow(1700, 900, "Simple example", NULL, NULL);
	if (!detail::mainWindow)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	int windowPosX = 0;
	int windowPosY = 0;
	glfwGetWindowPos(detail::mainWindow, &windowPosX, &windowPosY);
	detail::mainWindowPos[0] = windowPosX;
	detail::mainWindowPos[1] = windowPosY;
	int windowSizeX = 0;
	int windowSizeY = 0;
	glfwGetWindowSize(detail::mainWindow, &windowSizeX, &windowSizeY);
	detail::mainWindowSize[0] = windowSizeX;
	detail::mainWindowSize[1] = windowSizeY;
	int windowFramebufferSizeX = 0;
	int windowFramebufferSizeY = 0;
	glfwGetFramebufferSize(detail::mainWindow, &windowFramebufferSizeX, &windowFramebufferSizeY);
	detail::mainWindowFramebufferSize[0] = windowFramebufferSizeX;
	detail::mainWindowFramebufferSize[1] = windowFramebufferSizeY;

	detail::mainWindowIsInFocus = glfwGetWindowAttrib(detail::mainWindow, GLFW_FOCUSED);

	glfwSetWindowUserPointer(detail::mainWindow, &detail::mainWindowData);
	glfwSetKeyCallback(detail::mainWindow, Backend_GLFW_KeyboardKeyCallback);
	glfwSetMouseButtonCallback(detail::mainWindow, Backend_GLFW_MouseButtonCallback);
	glfwSetCursorPosCallback(detail::mainWindow, Backend_GLFW_MousePosCallback);
	glfwSetWindowPosCallback(detail::mainWindow, Backend_GLFW_WindowPosCallback);
	glfwSetWindowSizeCallback(detail::mainWindow, Backend_GLFW_WindowSizeCallback);
	glfwSetFramebufferSizeCallback(detail::mainWindow, Backend_GLFW_WindowFramebufferSizeCallback);
	glfwSetWindowCloseCallback(detail::mainWindow, Backend_GLFW_WindowCloseCallback);
	glfwSetWindowFocusCallback(detail::mainWindow, Backend_GLFW_WindowFocusCallback);
	glfwSetWindowIconifyCallback(detail::mainWindow, Backend_GLFW_WindowMinimizeCallback);

	return true;
}

void DEngine::Application::detail::Backend_ProcessEvents(
	std::chrono::high_resolution_clock::time_point now)
{
	mainWindowData.now = now;

	glfwPollEvents();
}

static void DEngine::Application::detail::Backend_GLFW_KeyboardKeyCallback(
	GLFWwindow* window,
	int key, 
	int scancode, 
	int action, 
	int mods)
{
	bool wasPressed = false;
	if (action == GLFW_PRESS)
		wasPressed = true;
	else if (action == GLFW_RELEASE)
		wasPressed = false;

	WindowData const& mainWindowData = *reinterpret_cast<WindowData const*>(glfwGetWindowUserPointer(window));

	detail::UpdateButton(Backend_GLFW_KeyboardKeyToRawButton(key), wasPressed, mainWindowData.now);
}

static void DEngine::Application::detail::Backend_GLFW_MouseButtonCallback(
	GLFWwindow* window, 
	int button, 
	int action, 
	int mods)
{
	if (action != GLFW_PRESS && action != GLFW_RELEASE)
		return;

	bool wasPressed = false;
	if (action == GLFW_PRESS)
		wasPressed = true;
	else if (action == GLFW_RELEASE)
		wasPressed = false;

	WindowData const& mainWindowData = *reinterpret_cast<WindowData const*>(glfwGetWindowUserPointer(window));

	detail::UpdateButton(Backend_GLFW_MouseButtonToRawButton(button), wasPressed, mainWindowData.now);
}

static void DEngine::Application::detail::Backend_GLFW_MousePosCallback(
	GLFWwindow* window, 
	double xpos, 
	double ypos)
{
	detail::UpdateMouse((u32)xpos, (u32)ypos);
}

static void DEngine::Application::detail::Backend_GLFW_WindowPosCallback(
	GLFWwindow* window,
	int xpos,
	int ypos)
{
	detail::mainWindowPos[0] = (i32)xpos;
	detail::mainWindowPos[1] = (i32)ypos;
}

static void DEngine::Application::detail::Backend_GLFW_WindowSizeCallback(
	GLFWwindow* window, 
	int width, 
	int height)
{
	detail::mainWindowResizeEvent = true;
	detail::mainWindowSize[0] = (u32)width;
	detail::mainWindowSize[1] = (u32)height;
}

static void DEngine::Application::detail::Backend_GLFW_WindowFramebufferSizeCallback(
	GLFWwindow* window, 
	int width, 
	int height)
{
	detail::mainWindowResizeEvent = true;
	detail::mainWindowFramebufferSize[0] = (u32)width;
	detail::mainWindowFramebufferSize[1] = (u32)height;
}

static void DEngine::Application::detail::Backend_GLFW_WindowCloseCallback(
	GLFWwindow* window)
{
	detail::shouldShutdown = true;
}

static void DEngine::Application::detail::Backend_GLFW_WindowFocusCallback(GLFWwindow* window, int focused)
{
	if (focused == GLFW_TRUE)
		detail::mainWindowIsInFocus = true;
	else if (focused == GLFW_FALSE)
		detail::mainWindowIsInFocus = false;
}

static void DEngine::Application::detail::Backend_GLFW_WindowMinimizeCallback(
	GLFWwindow* window, 
	int iconified)
{
	if (iconified == GLFW_TRUE)
	{
		detail::mainWindowIsMinimized = true;
	}
	else if (iconified == GLFW_FALSE)
	{
		detail::mainWindowIsMinimized = false;
		detail::mainWindowRestoreEvent = true;
	}
}

bool DEngine::Application::detail::CreateVkSurface(
	u64 vkInstance, 
	void const* vkAllocationCallbacks, 
	void* userData, 
	u64* vkSurface)
{
#pragma warning( suppress : 26812)
	VkResult err = glfwCreateWindowSurface(
	(VkInstance)vkInstance, 
		detail::mainWindow, 
		(VkAllocationCallbacks const*)vkAllocationCallbacks, 
		(VkSurfaceKHR*)vkSurface);
	if (err != 0)
		return false;

	return true;
}

DEngine::Std::StaticVector<char const*, 5> DEngine::Application::detail::GetRequiredVulkanInstanceExtensions()
{
	uint32_t count = 0;

	char const** exts = glfwGetRequiredInstanceExtensions(&count);

	Std::StaticVector<char const*, 5> returnVal{};
	returnVal.Resize(count);
	std::memcpy(returnVal.Data(), exts, count * sizeof(char const*));

	return returnVal;
}

DEngine::Application::Button DEngine::Application::detail::Backend_GLFW_MouseButtonToRawButton(i32 input)
{
	switch (input)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		return Button::LeftMouse;
	case GLFW_MOUSE_BUTTON_RIGHT:
		return Button::RightMouse;

	}

	return Button::Undefined;
}

DEngine::Application::Button DEngine::Application::detail::Backend_GLFW_KeyboardKeyToRawButton(i32 input)
{
	switch (input)
	{
	case GLFW_KEY_W:
		return Button::W;
	case GLFW_KEY_A:
		return Button::A;
	case GLFW_KEY_S:
		return Button::S;
	case GLFW_KEY_D:
		return Button::D;

	case GLFW_KEY_SPACE:
		return Button::Space;
	case GLFW_KEY_LEFT_CONTROL:
		return Button::LeftCtrl;

	}


	return Button::Undefined;
}