#define DENGINE_APPLICATION_BUTTON_COUNT

// Stops the warnings made by MSVC when using "unsafe" CRT fopen functions.
#ifdef _MSC_VER
#   define _CRT_SECURE_NO_WARNINGS
#endif

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
#include <cmath>
#include <cstring>
#include <cstdio>

namespace DEngine::Application::detail
{
	static GLFWwindow* mainWindow = nullptr;

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
	static void Backend_GLFW_CharCallback(GLFWwindow* window, unsigned int codepoint);
	static void Backend_GLFW_MouseButtonCallback(
		GLFWwindow* window,
		int button,
		int action,
		int mods);
	static void Backend_GLFW_CursorPosCallback(
		GLFWwindow* window,
		double xpos,
		double ypos);
	static void Backend_GLFW_ScrollCallback(
		GLFWwindow* window, 
		double xoffset,
		double yoffset);
	static void Backend_GLFW_JoystickConnectedCallback(
		int jid, 
		int event);
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

void DEngine::Application::OpenSoftInput()
{
	
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
	detail::mainWindowPos[0] = (i32)windowPosX;
	detail::mainWindowPos[1] = (i32)windowPosY;
	int windowSizeX = 0;
	int windowSizeY = 0;
	glfwGetWindowSize(detail::mainWindow, &windowSizeX, &windowSizeY);
	detail::mainWindowSize[0] = (u32)windowSizeX;
	detail::mainWindowSize[1] = (u32)windowSizeY;
	int windowFramebufferSizeX = 0;
	int windowFramebufferSizeY = 0;
	glfwGetFramebufferSize(detail::mainWindow, &windowFramebufferSizeX, &windowFramebufferSizeY);
	detail::mainWindowFramebufferSize[0] = (u32)windowFramebufferSizeX;
	detail::mainWindowFramebufferSize[1] = (u32)windowFramebufferSizeY;

	detail::mainWindowIsInFocus = glfwGetWindowAttrib(detail::mainWindow, GLFW_FOCUSED);

	detail::cursorOpt = CursorData();
	CursorData& cursor = detail::cursorOpt.Value();
	double mouseXPos = 0;
	double mouseYPos = 0;
	glfwGetCursorPos(detail::mainWindow, &mouseXPos, &mouseYPos);
	cursor.posX = (u32)std::floor(mouseXPos);
	cursor.posY = (u32)std::floor(mouseYPos);

	for (uSize i = 0; i < 8; i += 1)
	{
		int result = glfwJoystickPresent((int)i);
		if (result == GLFW_TRUE)
		{
			detail::gamepadConnected = true;
			break;
		}
	}
	
	glfwSetKeyCallback(detail::mainWindow, Backend_GLFW_KeyboardKeyCallback);
	glfwSetCharCallback(detail::mainWindow, Backend_GLFW_CharCallback);
	glfwSetMouseButtonCallback(detail::mainWindow, Backend_GLFW_MouseButtonCallback);
	glfwSetCursorPosCallback(detail::mainWindow, Backend_GLFW_CursorPosCallback);
	glfwSetScrollCallback(detail::mainWindow, Backend_GLFW_ScrollCallback);
	
	glfwSetJoystickCallback(Backend_GLFW_JoystickConnectedCallback);

	glfwSetWindowPosCallback(detail::mainWindow, Backend_GLFW_WindowPosCallback);
	glfwSetWindowSizeCallback(detail::mainWindow, Backend_GLFW_WindowSizeCallback);
	glfwSetFramebufferSizeCallback(detail::mainWindow, Backend_GLFW_WindowFramebufferSizeCallback);
	glfwSetWindowCloseCallback(detail::mainWindow, Backend_GLFW_WindowCloseCallback);
	glfwSetWindowFocusCallback(detail::mainWindow, Backend_GLFW_WindowFocusCallback);
	glfwSetWindowIconifyCallback(detail::mainWindow, Backend_GLFW_WindowMinimizeCallback);
	

	return true;
}

void DEngine::Application::detail::Backend_ProcessEvents()
{
	glfwPollEvents();

	if (detail::gamepadConnected)
	{
		int gamepadCount = 0;
		float const* axes = glfwGetJoystickAxes(detail::gamepadID, &gamepadCount);

		detail::gamepadState.leftStickX = axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		detail::gamepadState.leftStickY = axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
	}
	
}

static void DEngine::Application::detail::Backend_GLFW_KeyboardKeyCallback(
	GLFWwindow* window,
	int key, 
	int scancode, 
	int action, 
	int mods)
{
	if (action == GLFW_PRESS)
		detail::UpdateButton(Backend_GLFW_KeyboardKeyToRawButton(key), true);
	else if (action == GLFW_RELEASE)
		detail::UpdateButton(Backend_GLFW_KeyboardKeyToRawButton(key), false);
}

static void DEngine::Application::detail::Backend_GLFW_CharCallback(
	GLFWwindow* window, 
	unsigned int codepoint)
{
	detail::charInputs.PushBack((char)codepoint);
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

	detail::UpdateButton(Backend_GLFW_MouseButtonToRawButton(button), wasPressed);
}

static void DEngine::Application::detail::Backend_GLFW_CursorPosCallback(
	GLFWwindow* window, 
	double xpos, 
	double ypos)
{
	detail::UpdateCursor((u32)std::floor(xpos), (u32)std::floor(ypos));
}

static void DEngine::Application::detail::Backend_GLFW_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	CursorData& cursor = detail::cursorOpt.Value();
	cursor.scrollDeltaY = (f32)yoffset;
}

void DEngine::Application::detail::Backend_GLFW_JoystickConnectedCallback(int jid, int event)
{
	if (event == GLFW_CONNECTED)
	{
		detail::gamepadConnected = true;
		detail::gamepadID = jid;
	}
	else if (event == GLFW_DISCONNECTED)
	{
		detail::gamepadConnected = false;
	}
	else
	{
		// This can happen in future releases.

	}

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
	uSize vkInstance, 
	void const* vkAllocationCallbacks, 
	void* userData, 
	u64& vkSurface)
{
#pragma warning( suppress : 26812)
	VkResult err = glfwCreateWindowSurface(
	reinterpret_cast<VkInstance>(vkInstance), 
		detail::mainWindow, 
		reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks), 
		reinterpret_cast<VkSurfaceKHR*>(&vkSurface));
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

	case GLFW_KEY_UP:
		return Button::Up;
	case GLFW_KEY_DOWN:
		return Button::Down;
	case GLFW_KEY_LEFT:
		return Button::Left;
	case GLFW_KEY_RIGHT:
		return Button::Right;

	case GLFW_KEY_SPACE:
		return Button::Space;
	case GLFW_KEY_LEFT_CONTROL:
		return Button::LeftCtrl;
	case GLFW_KEY_ESCAPE:
		return Button::Escape;
	case GLFW_KEY_BACKSPACE:
		return Button::Backspace;
	case GLFW_KEY_DELETE:
		return Button::Delete;

	}


	return Button::Undefined;
}

DEngine::Application::FileInputStream::FileInputStream()
{
	static_assert(sizeof(std::FILE*) <= sizeof(FileInputStream::m_buffer));
}

DEngine::Application::FileInputStream::FileInputStream(char const* path)
{
	Open(path);
}

DEngine::Application::FileInputStream::FileInputStream(FileInputStream&& other) noexcept
{
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));
}

DEngine::Application::FileInputStream::~FileInputStream()
{
	Close();
}

DEngine::Application::FileInputStream& DEngine::Application::FileInputStream::operator=(FileInputStream&& other) noexcept
{
	if (this == &other)
		return *this;

	Close();

	std::memcpy(&this->m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));

	return *this;
}

bool DEngine::Application::FileInputStream::Seek(i64 offset, SeekOrigin origin)
{
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return false;

	int posixOrigin = 0;
	switch (origin)
	{
	case SeekOrigin::Current:
		posixOrigin = SEEK_CUR;
		break;
	case SeekOrigin::Start:
		posixOrigin = SEEK_SET;
		break;
	case SeekOrigin::End:
		posixOrigin = SEEK_END;
		break;
	}
	int result = fseek(file, (long)offset, posixOrigin);
	if (result == 0)
		return true;
	else
		return false;

}

bool DEngine::Application::FileInputStream::Read(char* output, u64 size)
{
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return false;

	size_t result = std::fread(output, 1, (size_t)size, file);
	return result == (size_t)size;
}

DEngine::Std::Opt<DEngine::u64> DEngine::Application::FileInputStream::Tell() const
{
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return {};

	long result = ftell(file);
	if (result == long(-1))
		// Handle error
		return {};
	else
		return static_cast<u64>(result);
}

bool DEngine::Application::FileInputStream::IsOpen() const
{
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	return file != nullptr;
}

bool DEngine::Application::FileInputStream::Open(char const* path)
{
	Close();
	std::FILE* file = std::fopen(path, "rb");
	std::memcpy(&m_buffer[0], &file, sizeof(std::FILE*));
	return file != nullptr;
}

void DEngine::Application::FileInputStream::Close()
{
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file != nullptr)
		std::fclose(file);

	std::memset(&m_buffer[0], 0, sizeof(std::FILE*));
}
