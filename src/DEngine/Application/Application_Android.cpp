#include "DEngine/Application.hpp"
#include "detail_Application.hpp"

#include <android/log.h>
#include <android_native_app_glue.h>

#define VK_USE_PLATFORM_ANDROID_KHR
#include "vulkan/vulkan.h"

#include <dlfcn.h>

#include <string>

extern int dengine_app_detail_main(int argc, char** argv);

namespace DEngine::Application::detail
{
	static android_app* androidAppPtr = nullptr;
	static bool androidWindowReady = false;

	static void HandleCmdInit(android_app* app, int32_t cmd)
	{
		switch (cmd)
		{
			case APP_CMD_INIT_WINDOW:
				// The window/surface is ready now.
				detail::androidWindowReady = true;
				break;
		}
	}

	static void HandleCmd(android_app* app, int32_t cmd)
	{
		switch (cmd)
		{
			case APP_CMD_LOST_FOCUS:
				Log("Lost focus");
				detail::mainWindowIsInFocus = false;
				break;
			case APP_CMD_GAINED_FOCUS:
				Log("Gained focus");
				detail::mainWindowIsInFocus = true;
				break;
			case APP_CMD_CONFIG_CHANGED:
				Log("Config changed");
				break;
			case APP_CMD_CONTENT_RECT_CHANGED:
				Log("Content rect changed");
				break;
			case APP_CMD_WINDOW_RESIZED:
				Log("Window resized");
				break;
			case APP_CMD_START:
				Log("Start");
				break;
			case APP_CMD_STOP:
				Log("Stop");
				break;
			case APP_CMD_PAUSE:
				Log("Pause");
				break;
			case APP_CMD_RESUME:
				Log("Resume");
				break;
			case APP_CMD_WINDOW_REDRAW_NEEDED:
				Log("Window redraw needed");
				break;
			case APP_CMD_INPUT_CHANGED:
				Log("Input changed");
				break;
			case APP_CMD_DESTROY:
				Log("Destroy");
				break;
			case APP_CMD_SAVE_STATE:
				Log("Save state");
				break;
		}
	}

	static void HandleMotionEvent(AInputEvent* event)
	{
		int32_t inputSource = AInputEvent_getSource(event);
		if (inputSource == AINPUT_SOURCE_TOUCHSCREEN)
		{
			int32_t motionAction = AMotionEvent_getAction(event);
			switch (motionAction)
			{
				case AMOTION_EVENT_ACTION_DOWN:
					Log("Motion event: Down");
					break;
				case AMOTION_EVENT_ACTION_UP:
					Log("Motion event: Up");
					break;
				case AMOTION_EVENT_ACTION_MOVE:
					Log("Motion event: Move");
					break;
				case AMOTION_EVENT_ACTION_CANCEL:
					Log("Motion event: Cancel");
					break;
			}

			size_t pointerCount = AMotionEvent_getPointerCount(event);
			std::string pointerCountString = std::string("Pointer count") + std::to_string(pointerCount);
			Log(pointerCountString.c_str());


			for (size_t i = 0; i < pointerCount; i++)
			{
				int32_t pointerID = AMotionEvent_getPointerId(event, i);
				std::string pointerCountString = std::string("ID ") + std::to_string(pointerID) + "  ";

				float x = AMotionEvent_getX(event, i);
				float y = AMotionEvent_getY(event, i);

				pointerCountString += std::to_string(x) + "  ";
				pointerCountString += std::to_string(y);

				Log(pointerCountString.c_str());
			}
		}

	}

	// Return 1 if you have handled the event, 0 for any default
	// dispatching.
	static int32_t HandleInputEvents(android_app* app, AInputEvent* event)
	{
		int32_t inputType = AInputEvent_getType(event);
		switch (inputType)
		{
			case AINPUT_EVENT_TYPE_KEY:
				Log("Key event type");
				break;
			case AINPUT_EVENT_TYPE_MOTION:
				Log("Motion event type");
				HandleMotionEvent(event);
				break;
		}
		return 0;
	}
}

// Main entry point for our program.
// This gets ran by the new thread that our Android app starts.
void android_main(android_app* app)
{
	DEngine::Application::detail::androidAppPtr = app;
	dengine_app_detail_main(0, nullptr);
}

bool DEngine::Application::detail::Backend_Initialize()
{
	// Set the callback to process system events
	detail::androidAppPtr->onAppCmd = HandleCmdInit;
	while (!detail::androidWindowReady)
	{
		// Keep polling events until the window is ready
		int fileDescriptors = 0; // What even is this?
		android_poll_source* source = nullptr;
		if (ALooper_pollAll(-1, nullptr, &fileDescriptors, (void**)&source) >= 0)
		{
			if (source != nullptr)
			{
				source->process(detail::androidAppPtr, source);
			}
		}
	}

	int width = ANativeWindow_getWidth(androidAppPtr->window);
	int height = ANativeWindow_getHeight(androidAppPtr->window);

	detail::displaySize[0] = (u32)width;
	detail::displaySize[1] = (u32)width;
	detail::mainWindowPos[0] = 0;
	detail::mainWindowPos[1] = 1;
	detail::mainWindowSize[0] = (u32)width;
	detail::mainWindowSize[1] = (u32)height;
	detail::mainWindowFramebufferSize[0] = (u32)width;
	detail::mainWindowFramebufferSize[1] = (u32)height;

	detail::mainWindowIsInFocus = true;
	detail::mainWindowIsMinimized = false;

	detail::androidAppPtr->onAppCmd = HandleCmd;
	detail::androidAppPtr->onInputEvent = HandleInputEvents;

	return true;
}

void DEngine::Application::detail::Backend_ProcessEvents(
	std::chrono::high_resolution_clock::time_point now)
{

	int fileDescriptors = 0; // What even is this?
	android_poll_source* source = nullptr;
	if (ALooper_pollAll(0, nullptr, &fileDescriptors, (void**)&source) >= 0)
	{
		if (source != nullptr)
		{
			source->process(detail::androidAppPtr, source);
		}
	}
}

bool DEngine::Application::detail::CreateVkSurface(
	u64 vkInstance, 
	void const* vkAllocationCallbacks, 
	void* userData, 
	u64* vkSurface)
{
	PFN_vkGetInstanceProcAddr procAddr = nullptr;
	void* lib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (lib == nullptr)
		return false;
	procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(lib, "vkGetInstanceProcAddr"));
	int closeReturnCode = dlclose(lib);
	lib = nullptr;
	if (procAddr == nullptr && closeReturnCode != 0)
		return false;

	PFN_vkCreateAndroidSurfaceKHR funcPtr = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
			procAddr(
					*reinterpret_cast<VkInstance*>(&vkInstance),
					"vkCreateAndroidSurfaceKHR"));
	if (funcPtr == nullptr)
		return false;

	VkAndroidSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.window = detail::androidAppPtr->window;

	VkSurfaceKHR returnVal{};
	VkResult result = funcPtr(
			*reinterpret_cast<VkInstance*>(&vkInstance),
			&createInfo,
			reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks),
			&returnVal);
	if (result != VK_SUCCESS || returnVal == VkSurfaceKHR())
		return false;

	*reinterpret_cast<VkSurfaceKHR*>(vkSurface) = returnVal;

	return true;
}

DEngine::Std::StaticVector<char const*, 5> DEngine::Application::detail::GetRequiredVulkanInstanceExtensions()
{
	Std::StaticVector<char const*, 5> returnVal{};
	returnVal.PushBack("VK_KHR_surface");
	returnVal.PushBack("VK_KHR_android_surface");
	return returnVal;
}

void DEngine::Application::Log(char const* msg)
{
	__android_log_print(ANDROID_LOG_ERROR, "DEngine: ", "%s", msg);
}