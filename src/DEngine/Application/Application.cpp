#define DENGINE_APPLICATION_BUTTON_COUNT
#include "DEngine/Application.hpp"
#include "detail_Application.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
#endif

#include "ImGui/imgui.h"

#include <iostream>
#include <cstring>
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

	static u32 displaySize[2] = {};
	static GLFWwindow* mainWindow = nullptr;
	static i32 mainWindowPos[2] = {};
	static u32 mainWindowSize[2] = {};
	static u32 mainWindowFramebufferSize[2] = {};
	static bool mainWindowIsInFocus = false;
	static bool mainWindowIsMinimized = false;
	static bool mainWindowRestoreEvent = false;
	static bool mainWindowResizeEvent = false;
	static bool shouldShutdown = false;
	struct WindowData
	{
		std::chrono::high_resolution_clock::time_point now = {};
	};
	static WindowData mainWindowData{};



	Button Backend_GLFW_KeyboardKeyToRawButton(i32 input);
	Button Backend_GLFW_MouseButtonToRawButton(i32 input);
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

DEngine::Std::Array<DEngine::i32, 2> DEngine::Application::MouseDelta()
{
	return { detail::mouseDelta[0], detail::mouseDelta[1] };
}

DEngine::Std::Array<DEngine::u32, 2> DEngine::Application::MousePosition()
{
	return { detail::mousePosition[0], detail::mousePosition[1] };
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

void DEngine::Application::detail::Backend_GLFW_WindowSizeCallback(
	GLFWwindow* window, 
	int width, 
	int height)
{
	detail::mainWindowResizeEvent = true;
	detail::mainWindowSize[0] = (u32)width;
	detail::mainWindowSize[1] = (u32)height;
}

void DEngine::Application::detail::Backend_GLFW_WindowFramebufferSizeCallback(
	GLFWwindow* window, 
	int width, 
	int height)
{
	detail::mainWindowResizeEvent = true;
	detail::mainWindowFramebufferSize[0] = (u32)width;
	detail::mainWindowFramebufferSize[1] = (u32)height;
}

void DEngine::Application::detail::Backend_GLFW_WindowCloseCallback(
	GLFWwindow* window)
{
	detail::shouldShutdown = true;
}

void DEngine::Application::detail::Backend_GLFW_WindowFocusCallback(GLFWwindow* window, int focused)
{
	if (focused == GLFW_TRUE)
		detail::mainWindowIsInFocus = true;
	else if (focused == GLFW_FALSE)
		detail::mainWindowIsInFocus = false;
}

void DEngine::Application::detail::Backend_GLFW_WindowMinimizeCallback(
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

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

bool DEngine::Application::detail::Initialize()
{
	glfwSetErrorCallback(error_callback);

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

void DEngine::Application::detail::ImgGui_Initialize()
{
	// Setup back-end capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos; // We can honor io.WantSetMousePos requests (optional, rarely used)
	//io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports; // We can create multi-viewports on the Platform side (optional)
#if GLFW_HAS_GLFW_HOVERED && defined(_WIN32)
	io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
#endif
	io.BackendPlatformName = "DEngine";

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
	io.KeyMap[ImGuiKey_Tab] = (int)Button::Tab;
	io.KeyMap[ImGuiKey_LeftArrow] = (int)Button::Left;
	io.KeyMap[ImGuiKey_RightArrow] = (int)Button::Right;
	io.KeyMap[ImGuiKey_UpArrow] = (int)Button::Up;
	io.KeyMap[ImGuiKey_DownArrow] = (int)Button::Down;
	io.KeyMap[ImGuiKey_PageUp] = (int)Button::PageUp;
	io.KeyMap[ImGuiKey_PageDown] = (int)Button::PageDown;
	io.KeyMap[ImGuiKey_Home] = (int)Button::Home;
	io.KeyMap[ImGuiKey_End] = (int)Button::End;
	io.KeyMap[ImGuiKey_Insert] = (int)Button::Insert;
	io.KeyMap[ImGuiKey_Delete] = (int)Button::Delete;
	io.KeyMap[ImGuiKey_Backspace] = (int)Button::Backspace;
	io.KeyMap[ImGuiKey_Space] = (int)Button::Space;
	io.KeyMap[ImGuiKey_Enter] = (int)Button::Enter;
	io.KeyMap[ImGuiKey_Escape] = (int)Button::Escape;
	io.KeyMap[ImGuiKey_KeyPadEnter] = (int)Button::Enter;
	io.KeyMap[ImGuiKey_A] = (int)Button::A;
	io.KeyMap[ImGuiKey_C] = (int)Button::C;
	io.KeyMap[ImGuiKey_V] = (int)Button::V;
	io.KeyMap[ImGuiKey_X] = (int)Button::X;
	io.KeyMap[ImGuiKey_Y] = (int)Button::Y;
	io.KeyMap[ImGuiKey_Z] = (int)Button::Z;

	// Our mouse update function expect PlatformHandle to be filled for the main viewport
	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	main_viewport->PlatformHandle = (void*)detail::mainWindow;
#ifdef _WIN32
	main_viewport->PlatformHandleRaw = glfwGetWin32Window(detail::mainWindow);
#endif
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		//ImGui_ImplGlfw_InitPlatformInterface();

	//ImGui_ImplGlfw_InitForVulkan(window, true);
}

void DEngine::Application::detail::ProcessEvents()
{
	auto now = std::chrono::high_resolution_clock::now();
	detail::mainWindowData.now = now;
	// Clear the input-events array
	for (auto& item : detail::buttonEvents)
		item = InputEvent::Unchanged;
	for (auto& item : detail::mouseDelta)
		item = 0;
	detail::mainWindowRestoreEvent = false;
	detail::mainWindowResizeEvent = false;

	glfwPollEvents();

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
	return mainWindowIsMinimized;
}

bool DEngine::Application::detail::IsRestored()
{
	return mainWindowRestoreEvent;
}

bool DEngine::Application::detail::ResizeEvent()
{
	return mainWindowResizeEvent;
}

void DEngine::Application::detail::ImGui_NewFrame()
{
	// Update buttons
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDown[0] = ButtonValue(Button::LeftMouse);
	io.MouseDown[1] = ButtonValue(Button::RightMouse);

	io.DisplaySize = ImVec2((float)detail::mainWindowSize[0], (float)detail::mainWindowSize[1]);
	if (io.DisplaySize.x > 0 && io.DisplaySize.y > 0)
		io.DisplayFramebufferScale = ImVec2(
			(float)detail::mainWindowFramebufferSize[0] / io.DisplaySize.x,
			(float)detail::mainWindowFramebufferSize[1] / io.DisplaySize.y);
		

	// Update mouse position
	const ImVec2 mouse_pos_backup = io.MousePos;
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	io.MouseHoveredViewport = 0;
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	for (int n = 0; n < platform_io.Viewports.Size; n++)
	{
		if (detail::mainWindowIsInFocus)
		{
			if (io.WantSetMousePos)
			{
				//glfwSetCursorPos(window, (double)(mouse_pos_backup.x - viewport->Pos.x), (double)(mouse_pos_backup.y - viewport->Pos.y));
			}
			else
			{
				Std::Array<u32, 2> mousePos = App::MousePosition();
				if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					// Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePosition is (0,0) when the mouse is on the upper-left of the primary monitor)
					//int window_x, window_y;
					//glfwGetWindowPos(window, &window_x, &window_y);
					//io.MousePos = ImVec2((float)mousePos[0] + window_x, (float)mousePos[1] + window_y);
				}
				else
				{
					// Single viewport mode: mouse position in client window coordinates (io.MousePosition is (0,0) when the mouse is on the upper-left corner of the app window)
					io.MousePos = ImVec2((float)mousePos[0], (float)mousePos[1]);
				}
			}
			//for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
				//io.MouseDown[i] |= glfwGetMouseButton(window, i) != 0;
		}

		// (Optional) When using multiple viewports: set io.MouseHoveredViewport to the viewport the OS mouse cursor is hovering.
		// Important: this information is not easy to provide and many high-level windowing library won't be able to provide it correctly, because
		// - This is _ignoring_ viewports with the ImGuiViewportFlags_NoInputs flag (pass-through windows).
		// - This is _regardless_ of whether another viewport is focused or being dragged from.
		// If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the back-end, imgui will ignore this field and infer the information by relying on the
		// rectangles and last focused time of every viewports it knows about. It will be unaware of other windows that may be sitting between or over your windows.
		// [GLFW] FIXME: This is currently only correct on Win32. See what we do below with the WM_NCHITTEST, missing an equivalent for other systems.
		// See https://github.com/glfw/glfw/issues/1236 if you want to help in making this a GLFW feature.
#if GLFW_HAS_GLFW_HOVERED && defined(_WIN32)
		if (glfwGetWindowAttrib(window, GLFW_HOVERED) && !(viewport->Flags & ImGuiViewportFlags_NoInputs))
			io.MouseHoveredViewport = viewport->ID;
#endif
	}

	//ImGui_ImplGlfw_NewFrame();
}

void DEngine::Application::Log(char const* msg)
{
	std::cout << msg << std::endl;
}

void DEngine::Application::SetRelativeMouseMode(bool enabled)
{
	if (enabled)
		glfwSetInputMode(detail::mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	else
		glfwSetInputMode(detail::mainWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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