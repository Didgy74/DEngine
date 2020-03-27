#define DENGINE_APPLICATION_BUTTON_COUNT
#include "DEngine/Application.hpp"
#include "detail_Application.hpp"
#include "DEngine/Containers/StaticVector.hpp"

#include "ImGui/imgui.h"

#include <iostream>
#include <cstring>
#include <chrono>

namespace DEngine::Application::detail
{
	static bool buttonValues[(int)Button::COUNT] = {};
	static std::chrono::high_resolution_clock::time_point buttonHeldStart[(int)Button::COUNT] = {};
	static f32 buttonHeldDuration[(int)Button::COUNT] = {};
	static KeyEventType buttonEvents[(int)Button::COUNT] = {};
	static u32 mousePosition[2] = {};
	static i32 mouseDelta[2] = {};
	static Std::StaticVector<TouchInput, 10> touchInputs{};

	u32 displaySize[2] = {};
	i32 mainWindowPos[2] = {};
	u32 mainWindowSize[2] = {};
	u32 mainWindowFramebufferSize[2] = {};
	bool mainWindowIsInFocus = false;
	bool mainWindowIsMinimized = false;
	bool mainWindowRestoreEvent = false;
	bool mainWindowResizeEvent = false;
	bool shouldShutdown = false;
}

bool DEngine::Application::ButtonValue(Button input)
{
	return detail::buttonValues[(int)input];
}

DEngine::Application::KeyEventType DEngine::Application::ButtonEvent(Button input)
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

DEngine::Std::StaticVector<DEngine::Application::TouchInput, 10> DEngine::Application::GetTouchInputs()
{
	return detail::touchInputs;
}

void DEngine::Application::detail::UpdateButton(
	Button button, 
	bool pressed, 
	std::chrono::high_resolution_clock::time_point now)
{
	detail::buttonValues[(int)button] = pressed;
	detail::buttonEvents[(int)button] = pressed ? KeyEventType::Pressed : KeyEventType::Unpressed;

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

void DEngine::Application::detail::UpdateMouse(u32 posX, u32 posY, i32 deltaX, i32 deltaY)
{
	detail::mouseDelta[0] = deltaX;
	detail::mouseDelta[1] = deltaY;

	detail::mousePosition[0] = posX;
	detail::mousePosition[1] = posY;
}

void DEngine::Application::detail::UpdateMouse(u32 posX, u32 posY)
{
	detail::mouseDelta[0] = (i32)posX - (i32)detail::mousePosition[0];
	detail::mouseDelta[1] = (i32)posY - (i32)detail::mousePosition[1];

	detail::mousePosition[0] = posX;
	detail::mousePosition[1] = posY;
}

bool DEngine::Application::detail::Initialize()
{
	Backend_Initialize();

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

	/*
	// Our mouse update function expect PlatformHandle to be filled for the main viewport
	ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	main_viewport->PlatformHandle = (void*)detail::mainWindow;
#ifdef _WIN32
	main_viewport->PlatformHandleRaw = glfwGetWin32Window(detail::mainWindow);
#endif
	//if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		//ImGui_ImplGlfw_InitPlatformInterface();
	*/
}

void DEngine::Application::detail::ProcessEvents()
{
	auto now = std::chrono::high_resolution_clock::now();

	// Clear the input-events array
	for (auto& item : detail::buttonEvents)
		item = KeyEventType::Unchanged;
	for (auto& item : detail::mouseDelta)
		item = 0;
	detail::mainWindowRestoreEvent = false;
	detail::mainWindowResizeEvent = false;

	Backend_ProcessEvents(now);

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
	ImVec2 const mouse_pos_backup = io.MousePos;
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	io.MouseHoveredViewport = 0;
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	if (detail::mainWindowIsInFocus)
	{
		Std::Array<u32, 2> mousePos = App::MousePosition();
		// Single viewport mode: mouse position in client window coordinates (io.MousePosition is (0,0) when the mouse is on the upper-left corner of the app window)
		io.MousePos = ImVec2((float)mousePos[0], (float)mousePos[1]);
	}
}

void DEngine::Application::SetRelativeMouseMode(bool enabled)
{

}