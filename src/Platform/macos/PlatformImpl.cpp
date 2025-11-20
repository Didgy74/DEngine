#include <DEngine/Platform/PlatformAssert.hpp>
#include <DEngine/Platform/PlatformImpl.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "gamecontrollerdb.hpp"

#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <dlfcn.h>

import DEngine.Math.Common;

extern int dengine_impl_main(int argc, char const* const* argv);

using namespace DEngine;
using namespace DEngine::Platform;

namespace XPlatformToBackend = ::DEngine::Platform::impl::XPlatformToBackend;

namespace DEngine::Platform::impl::Backend {
	struct GamepadSlot {
		bool connected = false;
	};

	struct BackendData : public Context::Impl::PlatformBackendBase {
		GLFWcursor* cursorTypes[(int)CursorType::COUNT] = {};

		// One entry per GLFW joystick slot. Connected slots are polled every frame;
		// disconnected slots are probed only every gamepadDisconnectedPollInterval
		// frames to avoid wasting cycles.
		GamepadSlot gamepadSlots[GLFW_JOYSTICK_LAST + 1] = {};
		u32 gamepadDisconnectedPollCounter = 0;
		static constexpr u32 gamepadDisconnectedPollInterval = 60;
	};

	struct PerWindowData : public impl::WindowPlatformBackendBase {
		GLFWwindow* glfwWindow = nullptr;

		[[nodiscard]] void* GetRawHandle() override { return glfwWindow; }
		[[nodiscard]] void const* GetRawHandle() const override { return glfwWindow; }
	};

	// Forward declarations for callbacks
	static void Glfw_WindowContentScaleCallback(GLFWwindow* window, float scaleX, float scaleY);
	static void Glfw_WindowCursorEnterCallback(GLFWwindow* window, int entered);
	static void Glfw_WindowCloseCallback(GLFWwindow* window);
	static void Glfw_WindowFocusCallback(GLFWwindow* window, int focused);
	static void Glfw_WindowPosCallback(GLFWwindow* window, int xpos, int ypos);
	static void Glfw_FramebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void Glfw_CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
	static void Glfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void Glfw_KeyboardKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void Glfw_CharCallback(GLFWwindow* window, unsigned int codepoint);

	[[nodiscard]] static Std::Opt<Button> GlfwButtonToPlatformButton(i32 input);
	[[nodiscard]] static Std::Opt<CursorButton> GlfwButtonToCursorButton(i32 input);
}

namespace Backend = DEngine::Platform::impl::Backend;

Context::Impl::PlatformBackendBase* XPlatformToBackend::Initialize(
	Context& ctx,
	Context::Impl& implData)
{
	// Set environment variable for MoltenVK ICD (matching DynamicDispatch.cpp)
	setenv("VK_ICD_FILENAMES", "./share/vulkan/icd.d/MoltenVK_icd.json", 1);

	// Load the Vulkan dylib (matching DynamicDispatch.cpp)
	void* vulkanLib = dlopen("./lib/libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
	if (vulkanLib == nullptr) {
		const char* error = dlerror();
		std::cerr << "Unable to load libvulkan.dylib: " << (error ? error : "unknown error") << std::endl;
		return nullptr;
	}

	// Get vkGetInstanceProcAddr function pointer
	auto vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vulkanLib, "vkGetInstanceProcAddr");
	if (vkGetInstanceProcAddr == nullptr) {
		const char* error = dlerror();
		std::cerr << "Unable to load vkGetInstanceProcAddr: " << (error ? error : "unknown error") << std::endl;
		dlclose(vulkanLib);
		return nullptr;
	}

	// Tell GLFW to use our Vulkan loader
	glfwInitVulkanLoader(vkGetInstanceProcAddr);

	// Now initialize GLFW
	bool glfwSuccess = glfwInit();
	if (!glfwSuccess)
		return nullptr;

	// Load the bundled SDL gamecontroller DB so non-builtin gamepads (newer Xbox /
	// PlayStation revisions, third-party pads) get a valid mapping and show up via
	// glfwJoystickIsGamepad / glfwGetGamepadState.
	if (!glfwUpdateGamepadMappings(Backend::gamepadMappings)) {
		std::cout << "Failed to update gamepad mappings" << std::endl;
	}

	auto* backendData = new Backend::BackendData;

	implData.cursorOpt = CursorData{};

	backendData->cursorTypes[(u8)CursorType::Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	backendData->cursorTypes[(u8)CursorType::VerticalResize] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	backendData->cursorTypes[(u8)CursorType::HorizontalResize] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);

	return backendData;
}

void XPlatformToBackend::ProcessEvents(
	Context& ctx,
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	bool waitForEvents,
	u64 timeoutNs)
{
	auto& backendData = (Backend::BackendData&)backendDataIn;

	if (waitForEvents) {
		glfwWaitEvents();
	} else {
		glfwPollEvents();
	}

	// Poll GLFW gamepads and fire events for any changes.
	// Connected slots are polled every frame; disconnected slots are probed
	// only every gamepadDisconnectedPollInterval frames to save cycles.
	bool const probeDisconnected =
		(backendData.gamepadDisconnectedPollCounter % Backend::BackendData::gamepadDisconnectedPollInterval) == 0;
	backendData.gamepadDisconnectedPollCounter += 1;

	for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; jid++) {
		auto& slot = backendData.gamepadSlots[jid];
		if (!slot.connected && !probeDisconnected)
			continue;

		GLFWgamepadstate gpState = {};
		if (!glfwJoystickIsGamepad(jid) || !glfwGetGamepadState(jid, &gpState)) {
			slot.connected = false;
			continue;
		}
		slot.connected = true;

		auto const deviceId = (GamepadDeviceId)jid;

		// GLFW reports stick Y with positive=down; flip to match the convention
		// where positive=up.
		impl::BackendInterface::UpdateGamepadAxis(
			implData, deviceId, GamepadAxis::LeftX, gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
		impl::BackendInterface::UpdateGamepadAxis(
			implData, deviceId, GamepadAxis::LeftY, -gpState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
		impl::BackendInterface::UpdateGamepadAxis(
			implData, deviceId, GamepadAxis::RightX, gpState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]);
		impl::BackendInterface::UpdateGamepadAxis(
			implData, deviceId, GamepadAxis::RightY, -gpState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);

		auto pollKey = [&](int glfwButton, GamepadKey key) {
			bool const pressed = gpState.buttons[glfwButton] == GLFW_PRESS;
			if (pressed != implData.gamepadState.keyStates[(int)key])
				impl::BackendInterface::UpdateGamepadKey(implData, deviceId, key, pressed);
		};
		pollKey(GLFW_GAMEPAD_BUTTON_A, GamepadKey::A);
		pollKey(GLFW_GAMEPAD_BUTTON_B, GamepadKey::B);
	}
}

void XPlatformToBackend::Destroy(Context::Impl::PlatformBackendBase* data) {
	DENGINE_IMPL_APPLICATION_ASSERT(data);
	auto* backendData = static_cast<Backend::BackendData*>(data);

	for (auto& cursor : backendData->cursorTypes) {
		if (cursor != nullptr) {
			glfwDestroyCursor(cursor);
		}
	}

	glfwTerminate();
	delete backendData;
}

auto XPlatformToBackend::NewWindow_Blocking(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	Std::Span<char const> const& title,
	Extent extent)
	-> Std::Opt<NewWindow_ReturnT>
{
	std::string titleString;
	titleString.resize(title.Size());
	std::memcpy(titleString.data(), title.Data(), title.Size());

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// `extent` is requested in pixels. On macOS, GLFW window creation expects
	// screen-coordinates, where the resulting pixel framebuffer is scaled by the
	// display's content scale (the Retina backing-scale-factor). We don't know
	// which monitor the window will land on yet, so translate using the primary
	// monitor's scale. This is exact for the common single-scale case; the true
	// pixel size is only known after creation (see glfwGetFramebufferSize below).
	float monitorScaleX = 1.f;
	float monitorScaleY = 1.f;
	glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &monitorScaleX, &monitorScaleY);
	// Round-to-nearest; extents are always positive so the +0.5 bias is safe.
	int const requestWidth = (int)((float)extent.width / monitorScaleX + 0.5f);
	int const requestHeight = (int)((float)extent.height / monitorScaleY + 0.5f);

	GLFWwindow* rawHandle = glfwCreateWindow(
		requestWidth,
		requestHeight,
		titleString.c_str(),
		nullptr,
		nullptr);

	if (!rawHandle)
		return Std::nullOpt;

	Context::Impl::WindowData windowData = {};

	// Get the framebuffer size in pixels. We report extents in pixels, not in
	// screen-coordinates, so query the framebuffer rather than the window size.
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(rawHandle, &width, &height);
	windowData.extent = { (u32)width, (u32)height };

	// Find the DPI of the monitor
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	int physSizeX = 0;
	int physSizeY = 0;
	glfwGetMonitorPhysicalSize(monitor, &physSizeX, &physSizeY);
	float inchesX = (float)physSizeX / 10.f / 2.54f;
	float inchesY = (float)physSizeY / 10.f / 2.54f;
	auto* monitorVideoMode = glfwGetVideoMode(monitor);
	windowData.dpiX = (float)monitorVideoMode->width / inchesX;
	windowData.dpiY = (float)monitorVideoMode->height / inchesY;

	float scaleX = 0;
	float scaleY = 0;
	glfwGetWindowContentScale(rawHandle, &scaleX, &scaleY);
	windowData.contentScale = scaleX;

	windowData.orientation = Orientation::Landscape;
	windowData.isMinimized = false;

	// Register callbacks
	glfwSetCursorEnterCallback(rawHandle, &Backend::Glfw_WindowCursorEnterCallback);
	glfwSetWindowContentScaleCallback(rawHandle, &Backend::Glfw_WindowContentScaleCallback);
	glfwSetWindowCloseCallback(rawHandle, &Backend::Glfw_WindowCloseCallback);
	glfwSetWindowPosCallback(rawHandle, &Backend::Glfw_WindowPosCallback);
	glfwSetFramebufferSizeCallback(rawHandle, &Backend::Glfw_FramebufferSizeCallback);
	glfwSetWindowFocusCallback(rawHandle, &Backend::Glfw_WindowFocusCallback);
	glfwSetCursorPosCallback(rawHandle, &Backend::Glfw_CursorPosCallback);
	glfwSetMouseButtonCallback(rawHandle, &Backend::Glfw_MouseButtonCallback);
	glfwSetKeyCallback(rawHandle, &Backend::Glfw_KeyboardKeyCallback);
	glfwSetCharCallback(rawHandle, &Backend::Glfw_CharCallback);

	glfwSetWindowUserPointer(rawHandle, &implData);

	auto* perWindowData = new Backend::PerWindowData;
	perWindowData->glfwWindow = rawHandle;

	WindowID windowId = {};
	{
		std::scoped_lock idLock{implData.windowsLock};
		windowId = (WindowID) implData.windowIdTracker;
		implData.windowIdTracker++;
		implData.windows.push_back(Context::Impl::WindowNode{
			.id = windowId,
			.windowData = windowData,
			.events = {},
			.backendData = Std::Box{perWindowData},
		});
	}

	NewWindow_ReturnT returnVal = {};
	returnVal.windowId = windowId;
	returnVal.windowData = windowData;

	return returnVal;
}

void XPlatformToBackend::DestroyWindow(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	Context::Impl::WindowNode const& windowNode)
{
	auto& perWindowData = (Backend::PerWindowData&)*windowNode.backendData.Get();
	glfwDestroyWindow(perWindowData.glfwWindow);
}

Context::CreateVkSurface_ReturnT XPlatformToBackend::CreateVkSurface(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	impl::WindowPlatformBackendBase& windowBackend,
	void const* vkGetInstanceProcAddrFn,
	uSize vkInstance,
	void const* vkAllocationCallbacks) noexcept
{
	auto& perWindowData = (Backend::PerWindowData&)windowBackend;
	VkSurfaceKHR newSurface;
	int err = glfwCreateWindowSurface(
		reinterpret_cast<VkInstance>(vkInstance),
		perWindowData.glfwWindow,
		reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks),
		&newSurface);

	Context::CreateVkSurface_ReturnT returnVal = {};
	returnVal.vkResult = err;
	returnVal.vkSurface = (uSize)newSurface;

	return returnVal;
}

void XPlatformToBackend::Log(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	LogSeverity severity,
	Std::Span<char const> const& msg)
{
	std::cout.write(msg.Data(), (std::streamsize)msg.Size()) << std::endl;
}

bool XPlatformToBackend::StartTextInputSession(
	Context::Impl& implData,
	WindowID windowId,
	Context::Impl::PlatformBackendBase& backendDataIn,
	SoftInputFilter inputFilter,
	Std::Span<char const> const& text,
	u64 selectionStart,
	u64 selectionCount)
{
	return true;
}

bool XPlatformToBackend::UpdateTextInputConnection(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendData,
	WindowID windowId,
	u64 selIndex,
	u64 selCount,
	Std::Span<u32 const> text)
{
	return true;
}

void XPlatformToBackend::UpdateTextInputConnectionSelection(
	Context::Impl::PlatformBackendBase& backendDataIn,
	u64 selIndex,
	u64 selCount)
{
}

void XPlatformToBackend::StopTextInputSession(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn)
{
}

void XPlatformToBackend::UpdateAccessibility(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	WindowID windowId,
	Std::RangeFnRef<AccessibilityUpdateElement> const& range,
	Std::Span<char const> textData)
{
	// Empty implementation - no accessibility support for basic backend
}

// GLFW Callback implementations
void Backend::Glfw_WindowCursorEnterCallback(GLFWwindow* window, int entered)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	impl::BackendInterface::UpdateWindowCursorEnter(implData, windowIdOpt.Get(), entered);
}

void Backend::Glfw_WindowContentScaleCallback(GLFWwindow* window, float scaleX, float scaleY)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	// I don't know what to do if the scale is very different in one direction.
	DENGINE_IMPL_APPLICATION_ASSERT(Math::Abs(scaleX - scaleY) < 0.1f);

	impl::BackendInterface::WindowContentScale(implData, windowIdOpt.Get(), scaleX);
}

void Backend::Glfw_WindowCloseCallback(GLFWwindow* window)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	impl::BackendInterface::PushWindowCloseSignal(implData, windowIdOpt.Get());
}

void Backend::Glfw_WindowFocusCallback(GLFWwindow* window, int focused)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	impl::BackendInterface::UpdateWindowFocus(implData, windowIdOpt.Get(), focused);
}

void Backend::Glfw_WindowPosCallback(GLFWwindow* window, int xpos, int ypos)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	// Update window position - but the BackendInterface doesn't have this function
	// So we'll skip it for now
}

void Backend::Glfw_FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// width/height arrive in pixels (framebuffer size), matching the pixel
	// extents we report at window creation.
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	Extent extent = { (u32)width, (u32)height };
	Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> insets = {};

	impl::BackendInterface::UpdateWindowSize(implData, windowIdOpt.Get(), extent, insets);
}

void Backend::Glfw_CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	// GLFW reports the cursor position in screen-coordinates on macOS. We report
	// positions in pixels, so scale by the window's (cached) content scale rather
	// than querying Cocoa on every cursor move.
	float contentScale = 1.f;
	if (auto const* windowNode = implData.GetWindowNode(windowIdOpt.Get()))
		contentScale = windowNode->windowData.contentScale;

	impl::BackendInterface::UpdateCursorPosition(
		implData,
		windowIdOpt.Get(),
		{ (i32)(xpos * contentScale), (i32)(ypos * contentScale) });
}

void Backend::Glfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	auto cursorButtonOpt = GlfwButtonToCursorButton(button);
	if (!cursorButtonOpt.Has()) {
		std::cerr << "Unknown GLFW mouse button: " << button << std::endl;
		return;
	}

	auto cursorButton = cursorButtonOpt.Get();

	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	bool wasPressed = false;
	if (action == GLFW_PRESS)
		wasPressed = true;
	else if (action == GLFW_RELEASE)
		wasPressed = false;
	else
		DENGINE_IMPL_APPLICATION_UNREACHABLE();

	impl::BackendInterface::UpdateCursorButton(
		implData,
		windowIdOpt.Get(),
		cursorButton,
		wasPressed);
}

void Backend::Glfw_KeyboardKeyCallback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	bool wasPressed = false;
	if (action == GLFW_PRESS)
		wasPressed = true;
	else if (action == GLFW_RELEASE)
		wasPressed = false;
	else if (action == GLFW_REPEAT)
		return;
	else
		DENGINE_IMPL_APPLICATION_UNREACHABLE();

	auto windowId = windowIdOpt.Get();
	auto dengineButtonOpt = GlfwButtonToPlatformButton(key);
	if (!dengineButtonOpt.Has())
		return;

	impl::BackendInterface::UpdateButton(
		implData,
		windowId,
		dengineButtonOpt.Get(),
		wasPressed);
}

void Backend::Glfw_CharCallback(GLFWwindow* window, unsigned int codepoint)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowIdOpt = implData.GetWindowId(window);
	if (!windowIdOpt.Has())
		return;

	auto windowId = windowIdOpt.Get();
	// Character input handling would go here if text input is active
}

Std::Opt<Button> Backend::GlfwButtonToPlatformButton(i32 input)
{
	switch (input) {
		case GLFW_KEY_BACKSPACE:
			return Button::Backspace;
		case GLFW_KEY_ENTER:
			return Button::Enter;
		case GLFW_KEY_SPACE:
			return Button::Space;
		case GLFW_KEY_DELETE:
			return Button::Delete;
		case GLFW_KEY_ESCAPE:
			return Button::Escape;
		case GLFW_KEY_LEFT:
			return Button::Left;
		case GLFW_KEY_RIGHT:
			return Button::Right;
		case GLFW_KEY_UP:
			return Button::Up;
		case GLFW_KEY_DOWN:
			return Button::Down;
		default:
			return Std::nullOpt;
	}
}

Std::Opt<CursorButton> Backend::GlfwButtonToCursorButton(i32 input) {
	switch (input) {
		case GLFW_MOUSE_BUTTON_LEFT:
			return CursorButton::Primary;
		case GLFW_MOUSE_BUTTON_RIGHT:
			return CursorButton::Secondary;
		default:
			DENGINE_IMPL_APPLICATION_UNREACHABLE();
			return Std::nullOpt;
	}
}

Std::StackVec<char const*, 5> Platform::GetRequiredVkInstanceExtensions() noexcept
{
	u32 count = 0;
	char const** exts = glfwGetRequiredInstanceExtensions(&count);
	DENGINE_IMPL_APPLICATION_ASSERT(exts != nullptr);

	Std::StackVec<char const*, 5> returnVal{};
	returnVal.Resize(count);
	for (int i = 0; i < count; i += 1)
		returnVal[i] = exts[i];

	returnVal.PushBack("VK_KHR_portability_enumeration");

	return returnVal;
}

int main(int argc, char const* const* argv) {
	dengine_impl_main(argc, argv);
	return 0;
}
