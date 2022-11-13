#include <DEngine/impl/AppAssert.hpp>
#include <DEngine/impl/Application.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Math/Common.hpp>

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
#include <new>

namespace DEngine::Application::impl
{
	struct Backend_Data
	{
		bool cursorLocked = false;
		Math::Vec2Int virtualCursorPos{};

		GLFWcursor* cursorTypes[(int)CursorType::COUNT] = {};
	};

	static Button Backend_GLFWButtonToDEngineButton(i32 input);

	/*
	static void Backend_GLFW_ScrollCallback(
		GLFWwindow* window,
		double xoffset,
		double yoffset);
	static void Backend_GLFW_JoystickConnectedCallback(
		int jid,
		int event);
	*/
	static void Backend_Glfw_WindowContentScaleCallback(
		GLFWwindow* window,
		float scaleX,
		float scaleY);
	static void Backend_GLFW_WindowCursorEnterCallback(
		GLFWwindow* window,
		int entered);
	static void Backend_GLFW_WindowCloseCallback(
		GLFWwindow* window);
	static void Backend_GLFW_WindowFocusCallback(
		GLFWwindow* window,
		int focused);
	static void Backend_GLFW_WindowPosCallback(
		GLFWwindow* window,
		int xpos,
		int ypos);
	static void Backend_GLFW_WindowSizeCallback(
		GLFWwindow* window,
		int width,
		int height);

	static void Backend_GLFW_CursorPosCallback(
		GLFWwindow* window,
		double xpos,
		double ypos);
	static void Backend_GLFW_MouseButtonCallback(
		GLFWwindow* window,
		int button,
		int action,
		int mods);
	static void Backend_GLFW_KeyboardKeyCallback(
		GLFWwindow* window,
		int key,
		int scancode,
		int action,
		int mods);
	static void Backend_GLFW_CharCallback(
		GLFWwindow* window,
		unsigned int codepoint);
	/*

	static void Backend_GLFW_WindowFramebufferSizeCallback(
		GLFWwindow* window,
		int width,
		int height);
	 */
}

using namespace DEngine;
using namespace DEngine::Application;

void* Application::impl::Backend::Initialize(Context& ctx, Context::Impl& implData)
{
	bool glfwSuccess = glfwInit();
	if (!glfwSuccess)
		return nullptr;

	auto* backendData = new Backend_Data;

	implData.cursorOpt = CursorData();

	backendData->cursorTypes[(u8)CursorType::Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	backendData->cursorTypes[(u8)CursorType::VerticalResize] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
	backendData->cursorTypes[(u8)CursorType::HorizontalResize] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);

	return backendData;
}

void Application::impl::Backend::ProcessEvents(
	Context& ctx,
	Context::Impl& implData,
	void* pBackendData,
	bool waitForEvents,
	u64 timeoutNs)
{
	if (waitForEvents)
	{
		glfwWaitEvents();
	}
	else
	{
		glfwPollEvents();
	}
}

void Application::impl::Backend::Destroy(void* data)
{
	DENGINE_IMPL_APPLICATION_ASSERT(data);

	auto* const backendData = static_cast<Backend_Data*>(data);

	glfwTerminate();

	delete backendData;
}

auto Application::impl::Backend::NewWindow(
	Context& ctx,
	Context::Impl& implData,
	void* pBackendData,
	WindowID windowId,
	Std::Span<char const> const& title,
	Extent extent) -> Std::Opt<NewWindow_ReturnT>
{
	std::string titleString;
	titleString.resize(title.Size());
	std::memcpy(titleString.data(), title.Data(), title.Size());

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* rawHandle = glfwCreateWindow(
		(int)extent.width,
		(int)extent.height,
		titleString.c_str(),
		nullptr,
		nullptr);

	if (!rawHandle)
		throw std::runtime_error("DEngine - Application: Could not make a new window.");

	Context::Impl::WindowData windowData = {};

	// Get window size
	int width = 0;
	int height = 0;
	glfwGetWindowSize(rawHandle, &width, &height);
	windowData.extent = { (u32)width, (u32)height };
	windowData.visibleOffset = {};
	windowData.visibleExtent = windowData.extent;

	int windowPosX = 0;
	int windowPosY = 0;
	glfwGetWindowPos(rawHandle, &windowPosX, &windowPosY);
	windowData.position = { (i32)windowPosX,(i32)windowPosY };

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

	glfwSetCursorEnterCallback(rawHandle, &impl::Backend_GLFW_WindowCursorEnterCallback);

	glfwSetWindowContentScaleCallback(rawHandle, &impl::Backend_Glfw_WindowContentScaleCallback);
	glfwSetWindowCloseCallback(rawHandle, &impl::Backend_GLFW_WindowCloseCallback);
	glfwSetWindowPosCallback(rawHandle, &impl::Backend_GLFW_WindowPosCallback);
	glfwSetWindowSizeCallback(rawHandle, &impl::Backend_GLFW_WindowSizeCallback);
	glfwSetWindowFocusCallback(rawHandle, &impl::Backend_GLFW_WindowFocusCallback);

	glfwSetCursorPosCallback(rawHandle, &impl::Backend_GLFW_CursorPosCallback);
	glfwSetMouseButtonCallback(rawHandle, &impl::Backend_GLFW_MouseButtonCallback);
	glfwSetKeyCallback(rawHandle, &impl::Backend_GLFW_KeyboardKeyCallback);
	glfwSetCharCallback(rawHandle, &impl::Backend_GLFW_CharCallback);

	/*
	glfwSetWindowSizeCallback(rawHandle, &impl::Backend_GLFW_WindowSizeCallback);
	glfwSetWindowFocusCallback(rawHandle, &impl::Backend_GLFW_WindowFocusCallback);
	glfwSetWindowIconifyCallback(rawHandle, &impl::Backend_GLFW_WindowMinimizeCallback);


	glfwSetCursorPosCallback(rawHandle, &impl::Backend_GLFW_CursorPosCallback);

	 */
	glfwSetWindowUserPointer(rawHandle, &implData);

	NewWindow_ReturnT returnVal = {};
	returnVal.windowData = windowData;
	returnVal.platformHandle = rawHandle;

	return returnVal;
}

void Application::impl::Backend::DestroyWindow(
	Context::Impl& implData,
	void* platformHandle,
	Context::Impl::WindowNode const& windowNode)
{
	glfwDestroyWindow((GLFWwindow*)windowNode.platformHandle);
}

Context::CreateVkSurface_ReturnT Application::impl::Backend::CreateVkSurface(
	Context::Impl& implData,
	void* pBackendDataIn,
	void* platformHandle,
	uSize vkInstance,
	void const* vkAllocationCallbacks) noexcept
{
	VkSurfaceKHR newSurface;
	int err = glfwCreateWindowSurface(
		reinterpret_cast<VkInstance>(vkInstance),
		(GLFWwindow*)platformHandle,
		reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks),
		&newSurface);

	Context::CreateVkSurface_ReturnT returnVal = {};
	returnVal.vkResult = err;
	returnVal.vkSurface = (uSize)newSurface;

	return returnVal;
}

void Application::impl::Backend::Log(
	Context::Impl& implData,
	LogSeverity severity,
	Std::Span<char const> const& msg)
{
	std::cout.write(msg.Data(), (std::streamsize)msg.Size()) << std::endl;
}

bool Application::impl::Backend::StartTextInputSession(
	Context::Impl& ctx,
	void* backendDataIn,
	SoftInputFilter inputFilter,
	Std::Span<char const> const& text)
{
	return true;
}

void Application::impl::Backend::StopTextInputSession(
	Context::Impl& implData,
	void* backendData)
{
	implData.textInputSessionActive = false;
}

void Application::impl::Backend_GLFW_WindowCursorEnterCallback(
	GLFWwindow* window,
	int entered)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	BackendInterface::UpdateWindowCursorEnter(
		implData,
		implData.GetWindowId(window),
		entered);
}

void Application::impl::Backend_Glfw_WindowContentScaleCallback(
	GLFWwindow* window,
	float scaleX,
	float scaleY)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	// I don't know what to do if the scale is very different in one direction.
	DENGINE_IMPL_APPLICATION_ASSERT(Math::Abs(scaleX - scaleY) < 0.1f);

	BackendInterface::WindowContentScale(
		implData,
		implData.GetWindowId(window),
		scaleX);
}

void Application::impl::Backend_GLFW_WindowCloseCallback(
	GLFWwindow *window)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	BackendInterface::PushWindowCloseSignal(
		implData,
		implData.GetWindowId(window));
}

void Application::impl::Backend_GLFW_WindowFocusCallback(
	GLFWwindow* window,
	int focused)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	BackendInterface::UpdateWindowFocus(
		implData,
		implData.GetWindowId(window),
		focused);
}

void Application::impl::Backend_GLFW_WindowPosCallback(
	GLFWwindow* window,
	int xpos,
	int ypos)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	BackendInterface::UpdateWindowPosition(
		implData,
		implData.GetWindowId(window),
		{ (i32)xpos, (i32)ypos });
}

void Application::impl::Backend_GLFW_WindowSizeCallback(
	GLFWwindow *window,
	int width,
	int height)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	App::Extent extent = { (u32)width, (u32)height };

	impl::BackendInterface::UpdateWindowSize(
		implData,
		implData.GetWindowId(window),
		extent,
		0,
		0,
		extent);
}

void Application::impl::Backend_GLFW_CursorPosCallback(
	GLFWwindow* window,
	double xpos,
	double ypos)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	BackendInterface::UpdateCursorPosition(
		implData,
		implData.GetWindowId(window),
		{ (i32)xpos, (i32)ypos });
}

void Application::impl::Backend_GLFW_KeyboardKeyCallback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	bool wasPressed = false;
	if (action == GLFW_PRESS)
		wasPressed = true;
	else if (action == GLFW_RELEASE)
		wasPressed = false;
	else if (action == GLFW_REPEAT)
	{
		return;
	}
	else
		DENGINE_IMPL_APPLICATION_UNREACHABLE();

	auto windowId = implData.GetWindowId(window);
	auto dengineButton = Backend_GLFWButtonToDEngineButton(key);

	BackendInterface::UpdateButton(
		implData,
		windowId,
		dengineButton,
		wasPressed);

	bool doCharEnterEvent =
		dengineButton == Button::Enter &&
		wasPressed &&
		implData.textInputSessionActive;
	if (doCharEnterEvent)
	{
		BackendInterface::PushEndTextInputSessionEvent(implData, windowId);
	}


	bool doRemoveChar =
		dengineButton == Button::Backspace &&
		wasPressed &&
		implData.textInputSessionActive &&
		implData.textInputSelectedIndex > 0;
	if (doRemoveChar)
	{
		auto oldIndex = implData.textInputSelectedIndex - 1;
		auto oldCount = 1;
		BackendInterface::PushTextInputEvent(
			implData,
			windowId,
			oldIndex,
			oldCount,
			{});
	}

	bool endInputSession =
		implData.textInputSessionActive &&
		dengineButton == Button::Escape;
	if (endInputSession)
	{
		BackendInterface::PushEndTextInputSessionEvent(implData, windowId);
	}
}

void Application::impl::Backend_GLFW_MouseButtonCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	bool wasPressed = false;
	if (action == GLFW_PRESS)
		wasPressed = true;
	else if (action == GLFW_RELEASE)
		wasPressed = false;
	else
		DENGINE_IMPL_APPLICATION_UNREACHABLE();

	BackendInterface::UpdateButton(
		implData,
		implData.GetWindowId(window),
		Backend_GLFWButtonToDEngineButton(button),
		wasPressed);
}

void Application::impl::Backend_GLFW_CharCallback(
	GLFWwindow* window,
	unsigned int codepoint)
{
	auto implDataPtr = static_cast<Context::Impl*>(glfwGetWindowUserPointer(window));
	DENGINE_IMPL_APPLICATION_ASSERT(implDataPtr);
	auto& implData = *implDataPtr;

	auto windowId = implData.GetWindowId(window);

	auto value = (u32)codepoint;

	if (implData.textInputSessionActive)
	{
		BackendInterface::PushTextInputEvent(
			implData,
			windowId,
			implData.textInputSelectedIndex,
			0,
			{ &value, 1 });
	}
}

auto Application::impl::Backend_GLFWButtonToDEngineButton(i32 input) -> Button
{
	switch (input)
	{
		case GLFW_MOUSE_BUTTON_LEFT:
			return Button::LeftMouse;
		case GLFW_MOUSE_BUTTON_RIGHT:
			return Button::RightMouse;

		case GLFW_KEY_BACKSPACE:
			return Button::Backspace;
		case GLFW_KEY_ENTER:
			return Button::Enter;
		case GLFW_KEY_DELETE:
			return Button::Delete;
		case GLFW_KEY_ESCAPE:
			return Button::Escape;

		default:
			return Button::Undefined;
	}
}

Std::StackVec<char const*, 5> Application::GetRequiredVkInstanceExtensions() noexcept
{
	u32 count = 0;

	char const** exts = glfwGetRequiredInstanceExtensions(&count);

	Std::StackVec<char const*, 5> returnVal{};
	returnVal.Resize(count);
	for (int i = 0; i < count; i += 1)
		returnVal[i] = exts[i];

	return returnVal;
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
