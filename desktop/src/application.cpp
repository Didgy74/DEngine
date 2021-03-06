#define DENGINE_APPLICATION_CURSORTYPE_COUNT
#define DENGINE_APPLICATION_BUTTON_COUNT

#include <DEngine/detail/AppAssert.hpp>
#include <DEngine/detail/Application.hpp>
#include <DEngine/Std/Utility.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/* Work around old GLFW on debian */
#ifndef GLFW_GAMEPAD_AXIS_LEFT_X
#define GLFW_GAMEPAD_AXIS_LEFT_X        0
#define GLFW_GAMEPAD_AXIS_LEFT_Y        1
#endif

#include <iostream>
#include <cstdio>
#include <cstring>

namespace DEngine::Application::detail
{
	struct Backend_Data
	{
		bool cursorLocked = false;
		Math::Vec2Int virtualCursorPos{};

		GLFWcursor* cursorTypes[(int)CursorType::COUNT] = {};
	};

	static Backend_Data* pBackendData = nullptr;

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
	static void Backend_GLFW_CharCallback(
		GLFWwindow* window, 
		unsigned int codepoint);
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
	static void Backend_GLFW_WindowCursorEnterCallback(
		GLFWwindow* window,
		int entered);
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

using namespace DEngine;

void Application::LockCursor(bool state)
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;

	auto const& window = *Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[](decltype(appData.windows)::value_type const& val) -> bool {
			return val.id == (WindowID)0; });
	if (state)
		glfwSetInputMode((GLFWwindow*)window.platformHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	else
		glfwSetInputMode((GLFWwindow*)window.platformHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	backendData.cursorLocked = state;
}

void Application::OpenSoftInput(Std::Str text, SoftInputFilter filter)
{
}

void Application::UpdateCharInputContext(Std::Str text)
{

}

void Application::HideSoftInput()
{
}

void Application::detail::Backend_Log(char const* msg)
{
	std::cout << msg << std::endl;
}

void Application::SetCursor(WindowID windowIn, CursorType cursor) noexcept
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;

	auto& window = *Std::FindIf(
		appData.windows.begin(),
		appData.windows.end(),
		[windowIn](decltype(appData.windows)::value_type& val) -> bool {
			return val.id == windowIn; });

	if (window.windowData.currentCursorType != cursor)
	{
		glfwSetCursor((GLFWwindow*)window.platformHandle, backendData.cursorTypes[(u8)cursor]);
		window.windowData.currentCursorType = cursor;
	}
}

Application::WindowID Application::CreateWindow(
	char const* title,
	Extent extents)
{
	auto& appData = *detail::pAppData;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* rawHandle = glfwCreateWindow((int)extents.width, (int)extents.height, title, nullptr, nullptr);
	if (!rawHandle)
		throw std::runtime_error("DEngine - Application: Could not make a new window.");

	detail::AppData::WindowNode newNode{};
	newNode.id = (WindowID)appData.windowIdTracker;
	appData.windowIdTracker++;
	newNode.platformHandle = rawHandle;
	newNode.events = {};
	
	// Get window size
	int widthX;
	int heightY;
	glfwGetWindowSize(rawHandle, &widthX, &heightY);
	newNode.windowData.size = { (u32)widthX, (u32)heightY };
	newNode.windowData.visiblePosition = {};
	newNode.windowData.visibleSize = { (u32)widthX, (u32)heightY };
	
	int windowPosX = 0;
	int windowPosY = 0;
	glfwGetWindowPos(rawHandle, &windowPosX, &windowPosY);
	newNode.windowData.position = { (i32)windowPosX,(i32)windowPosY };
	
	appData.windows.push_back(newNode);

	glfwSetWindowPosCallback(rawHandle, &detail::Backend_GLFW_WindowPosCallback);
	glfwSetWindowSizeCallback(rawHandle, &detail::Backend_GLFW_WindowSizeCallback);
	glfwSetWindowFocusCallback(rawHandle, &detail::Backend_GLFW_WindowFocusCallback);
	glfwSetWindowIconifyCallback(rawHandle, &detail::Backend_GLFW_WindowMinimizeCallback);
	glfwSetWindowCloseCallback(rawHandle, &detail::Backend_GLFW_WindowCloseCallback);
	glfwSetCursorEnterCallback(rawHandle, &detail::Backend_GLFW_WindowCursorEnterCallback);


	glfwSetCursorPosCallback(rawHandle, &detail::Backend_GLFW_CursorPosCallback);
	glfwSetMouseButtonCallback(rawHandle, &detail::Backend_GLFW_MouseButtonCallback);
	glfwSetCharCallback(rawHandle, &detail::Backend_GLFW_CharCallback);
	glfwSetKeyCallback(rawHandle, &detail::Backend_GLFW_KeyboardKeyCallback);

	return newNode.id;
}

void Application::detail::Backend_DestroyWindow(AppData::WindowNode& windowNode)
{
	glfwDestroyWindow((GLFWwindow*)windowNode.platformHandle);
}

static void Application::detail::Backend_GLFW_ErrorCallback(
	int error, 
	const char* description)
{
	fputs(description, stderr);
}

bool Application::detail::Backend_Initialize() noexcept
{
	glfwSetErrorCallback(Backend_GLFW_ErrorCallback);

	if (!glfwInit())
		return false;

	pAppData->cursorOpt = CursorData();

	pBackendData = new Backend_Data;

	pBackendData->cursorTypes[(u8)CursorType::Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	pBackendData->cursorTypes[(u8)CursorType::VerticalResize] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	pBackendData->cursorTypes[(u8)CursorType::HorizontalResize] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);

	return true;
}

void Application::detail::Backend_ProcessEvents()
{
	glfwPollEvents();

	if (detail::pAppData->gamepadConnected)
	{
		int gamepadCount = 0;
		float const* axes = glfwGetJoystickAxes(detail::pAppData->gamepadID, &gamepadCount);

		detail::pAppData->gamepadState.leftStickX = axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		detail::pAppData->gamepadState.leftStickY = axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
	}
}

static void Application::detail::Backend_GLFW_KeyboardKeyCallback(
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

static void Application::detail::Backend_GLFW_CharCallback(
	GLFWwindow* window, 
	unsigned int codepoint)
{
	detail::PushCharInput(codepoint);
}

static void Application::detail::Backend_GLFW_MouseButtonCallback(
	GLFWwindow* window, 
	int button, 
	int action, 
	int mods)
{
	if (action != GLFW_PRESS && action != GLFW_RELEASE)
		throw std::runtime_error("DEngine - Application: Encountered unexpected action value in GLFW mouse button press callback.");

	bool wasPressed = false;
	if (action == GLFW_PRESS)
		wasPressed = true;
	else if (action == GLFW_RELEASE)
		wasPressed = false;

	detail::UpdateButton(Backend_GLFW_MouseButtonToRawButton(button), wasPressed);
}

static void Application::detail::Backend_GLFW_CursorPosCallback(
	GLFWwindow* window, 
	double xpos, 
	double ypos)
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;
	if (backendData.cursorLocked)
	{
		auto const& windowNode = appData.windows.front();
		Math::Vec2Int newVirtualCursorPos = { (i32)xpos, (i32)ypos };
		Math::Vec2Int cursorDelta = newVirtualCursorPos - backendData.virtualCursorPos;
		backendData.virtualCursorPos = newVirtualCursorPos;
		CursorData cursorData = Cursor().Value();
		detail::UpdateCursor(*GetWindowNode(window), cursorData.position - windowNode.windowData.position, cursorDelta);
	}
	else
	{
		backendData.virtualCursorPos = { (i32)xpos, (i32)ypos };
		detail::UpdateCursor(*GetWindowNode(window), { (i32)xpos, (i32)ypos });
	}
}

static void Application::detail::Backend_GLFW_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	CursorData& cursor = detail::pAppData->cursorOpt.Value();
	cursor.scrollDeltaY = (f32)yoffset;
}

void Application::detail::Backend_GLFW_JoystickConnectedCallback(int jid, int event)
{
	if (event == GLFW_CONNECTED)
	{
		detail::pAppData->gamepadConnected = true;
		detail::pAppData->gamepadID = jid;
	}
	else if (event == GLFW_DISCONNECTED)
	{
		detail::pAppData->gamepadConnected = false;
	}
	else
	{
		// This can happen in future releases.

	}
}

static void Application::detail::Backend_GLFW_WindowPosCallback(
	GLFWwindow* window,
	int xpos,
	int ypos)
{
	if (xpos != -32000 && ypos != -32000)
		detail::UpdateWindowPosition(window, { (i32)xpos, (i32)ypos });
}

static void Application::detail::Backend_GLFW_WindowSizeCallback(
	GLFWwindow* window, 
	int width, 
	int height)
{
	auto windowNodePtr = detail::GetWindowNode(window);

	if (width != 0 && height != 0)
		detail::UpdateWindowSize(
			*windowNodePtr, 
			{ (u32)width, (u32)height },
			{},
			{ (u32)width, (u32)height });
}

void Application::detail::Backend_GLFW_WindowCursorEnterCallback(
	GLFWwindow* window,
	int entered)
{
	if (entered == GLFW_TRUE)
		detail::UpdateWindowCursorEnter(window, true);
	else if (entered == GLFW_FALSE)
		detail::UpdateWindowCursorEnter(window, false);
	else
		throw std::runtime_error("DEngine - App: GLFW window cursor enter event called with unexpected enter argument.");
}

static void Application::detail::Backend_GLFW_WindowFramebufferSizeCallback(
	GLFWwindow* window, 
	int width, 
	int height)
{
}

static void Application::detail::Backend_GLFW_WindowCloseCallback(
	GLFWwindow* window)
{
	AppData::WindowNode* windowNode = detail::GetWindowNode(window);
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNode);
	detail::UpdateWindowClose(*windowNode);
}

static void Application::detail::Backend_GLFW_WindowFocusCallback(GLFWwindow* window, int focused)
{
	if (focused == GLFW_TRUE)
		detail::UpdateWindowFocus(window, true);
	else if (focused == GLFW_FALSE)
		detail::UpdateWindowFocus(window, false);
	else
		throw std::runtime_error("DEngine - App: GLFW window focus callback called with unexpected focus argument.");
}

static void Application::detail::Backend_GLFW_WindowMinimizeCallback(
	GLFWwindow* window, 
	int iconified)
{
	AppData::WindowNode* windowNode = detail::GetWindowNode(window);
	DENGINE_DETAIL_APPLICATION_ASSERT(windowNode);
	detail::UpdateWindowMinimized(*windowNode, iconified);
}

Std::Opt<u64> Application::CreateVkSurface(
	WindowID window,
	uSize vkInstance,
	void const* vkAllocationCallbacks)
{
	auto const& windowContainer = detail::pAppData->windows;
	auto windowIt = Std::FindIf(
		windowContainer.begin(),
		windowContainer.end(),
		[window](detail::AppData::WindowNode const& val) -> bool {
			return val.id == window; });
	if (windowIt == windowContainer.end())
		throw std::runtime_error("Could not find window.");


	VkSurfaceKHR newSurface;
	int err = glfwCreateWindowSurface(
	reinterpret_cast<VkInstance>(vkInstance), 
		(GLFWwindow*)windowIt->platformHandle,
		reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks), 
		&newSurface);
	if (err != 0)
		return {};
	else
		return Std::Opt{ (u64)newSurface };
}

Std::StackVec<char const*, 5> Application::RequiredVulkanInstanceExtensions() noexcept
{
	uint32_t count = 0;

	char const** exts = glfwGetRequiredInstanceExtensions(&count);

	Std::StackVec<char const*, 5> returnVal{};
	returnVal.Resize(count);
	std::memcpy(returnVal.Data(), exts, count * sizeof(char const*));

	return returnVal;
}

Application::Button Application::detail::Backend_GLFW_MouseButtonToRawButton(i32 input)
{
	switch (input)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		return Button::LeftMouse;
	case GLFW_MOUSE_BUTTON_RIGHT:
		return Button::RightMouse;

	case GLFW_KEY_BACKSPACE:
		return Button::Backspace;
	case GLFW_KEY_DELETE:
		return Button::Delete;
	}

	return Button::Undefined;
}

Application::Button Application::detail::Backend_GLFW_KeyboardKeyToRawButton(i32 input)
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
	case GLFW_KEY_ENTER:
		return Button::Enter;

	}

	return Button::Undefined;
}

Application::FileInputStream::FileInputStream()
{
	static_assert(sizeof(std::FILE*) <= sizeof(FileInputStream::m_buffer));
}

Application::FileInputStream::FileInputStream(char const* path)
{
	Open(path);
}

Application::FileInputStream::FileInputStream(FileInputStream&& other) noexcept
{
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));
}

Application::FileInputStream::~FileInputStream()
{
	Close();
}

Application::FileInputStream& Application::FileInputStream::operator=(FileInputStream&& other) noexcept
{
	if (this == &other)
		return *this;

	Close();

	std::memcpy(&this->m_buffer[0], &other.m_buffer[0], sizeof(std::FILE*));
	std::memset(&other.m_buffer[0], 0, sizeof(std::FILE*));

	return *this;
}

bool Application::FileInputStream::Seek(i64 offset, SeekOrigin origin)
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
	return result == 0;
}

bool Application::FileInputStream::Read(char* output, u64 size)
{
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file == nullptr)
		return false;

	size_t result = std::fread(output, 1, (size_t)size, file);
	return result == (size_t)size;
}

Std::Opt<u64> Application::FileInputStream::Tell() const
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
		return Std::Opt{ static_cast<u64>(result) };
}

bool Application::FileInputStream::IsOpen() const
{
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	return file != nullptr;
}

bool Application::FileInputStream::Open(char const* path)
{
	Close();
	std::FILE* file = std::fopen(path, "rb");
	std::memcpy(&m_buffer[0], &file, sizeof(std::FILE*));
	return file != nullptr;
}

void Application::FileInputStream::Close()
{
	std::FILE* file = nullptr;
	std::memcpy(&file, &m_buffer[0], sizeof(std::FILE*));
	if (file != nullptr)
		std::fclose(file);

	std::memset(&m_buffer[0], 0, sizeof(std::FILE*));
}
