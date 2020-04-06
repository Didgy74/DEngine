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
	static android_app* Backend_androidAppPtr = nullptr;
	static bool Backend_androidWindowReady = false;
	static int Backend_currentOrientation = 0;
	static bool Backend_configChangedThisTick = false;

	static TouchEventType Backend_Android_MotionActionToTouchEventType(int32_t in);

	static void HandleConfigChanged(android_app* app)
	{
		int width = ANativeWindow_getWidth(app->window);
		int height = ANativeWindow_getHeight(app->window);
		std::string a = std::string("Queried dims are: ") + std::to_string(width) + ", " + std::to_string(height);
		Log(a.c_str());

		int newOrientation = AConfiguration_getOrientation(app->config);
		if (newOrientation == Backend_currentOrientation)
		{
			Log("New orientation is same as old one.");
		}
		else if ((detail::Backend_currentOrientation == ACONFIGURATION_ORIENTATION_LAND &&
		     newOrientation == ACONFIGURATION_ORIENTATION_PORT) ||
		     (detail::Backend_currentOrientation == ACONFIGURATION_ORIENTATION_PORT &&
		     newOrientation == ACONFIGURATION_ORIENTATION_LAND))
		{
			// We went from portrait to landscape, or reverse
			detail::mainWindowResizeEvent = true;

			int width = ANativeWindow_getWidth(app->window);
			int height = ANativeWindow_getHeight(app->window);

			detail::displaySize[0] = (u32)width;
			detail::displaySize[1] = (u32)height;
			detail::mainWindowSize[0] = (u32)width;
			detail::mainWindowSize[1] = (u32)height;
			detail::mainWindowFramebufferSize[0] = (u32)width;
			detail::mainWindowFramebufferSize[1] = (u32)height;

		}
		else
		{
			Log("Error. Encountered an orientation that was not landscape or portrait.");
			//abort();
		}
	}

	static void HandleCmdInit(android_app* app, int32_t cmd)
	{
		switch (cmd)
		{
			case APP_CMD_INIT_WINDOW:
				// The window/surface is ready now.
				detail::Backend_androidWindowReady = true;
				detail::mainWindowSurfaceInitialized = true;
				detail::mainWindowSurfaceInitializeEvent = true;
				break;
		}
	}

	static void HandleCmd(android_app* app, int32_t cmd)
	{
		switch (cmd)
		{
			case APP_CMD_INIT_WINDOW:
				Log("Init window");
				detail::mainWindowSurfaceInitialized = true;
				detail::mainWindowSurfaceInitializeEvent = true;

				detail::Backend_androidWindowReady = true;
				break;
			case APP_CMD_TERM_WINDOW:
				Log("Terminate window");
				detail::mainWindowSurfaceInitialized = false;
				detail::mainWindowSurfaceTerminateEvent = true;

				detail::Backend_androidWindowReady = false;
				break;
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
				detail::Backend_configChangedThisTick = true;
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
				detail::mainWindowIsMinimized = true;
				Log("Pause");
				break;
			case APP_CMD_RESUME:
				detail::mainWindowIsMinimized = false;
				Log("Resume");
				detail::mainWindowSurfaceInitialized = true;
				detail::mainWindowSurfaceInitializeEvent = true;
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
		if (inputSource != AINPUT_SOURCE_TOUCHSCREEN)
			return;
		int32_t motionAction = AMotionEvent_getAction(event);

		size_t pointerCount = AMotionEvent_getPointerCount(event);
		for (size_t i = 0; i < pointerCount; i++)
		{
			Application::TouchInput values{};

			values.id = (uSize)AMotionEvent_getPointerId(event, i);
			values.x = AMotionEvent_getX(event, i);
			values.y = AMotionEvent_getY(event, i);
			values.eventType = Backend_Android_MotionActionToTouchEventType(motionAction);
			Application::detail::UpdateTouchInput(values);
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
				break;
			case AINPUT_EVENT_TYPE_MOTION:
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
	DEngine::Application::detail::Backend_androidAppPtr = app;
	dengine_app_detail_main(0, nullptr);
}

bool DEngine::Application::detail::Backend_Initialize()
{
	// Set default values
	detail::mainWindowIsInFocus = true;
	detail::mainWindowIsMinimized = false;
	detail::shouldShutdown = false;

	// Set the callback to process system events
	detail::Backend_androidAppPtr->onAppCmd = HandleCmdInit;
	while (!detail::Backend_androidWindowReady)
	{
		// Keep polling events until the window is ready
		int fileDescriptors = 0; // What even is this?
		android_poll_source* source = nullptr;
		if (ALooper_pollAll(-1, nullptr, &fileDescriptors, (void**)&source) >= 0)
		{
			if (source != nullptr)
			{
				source->process(detail::Backend_androidAppPtr, source);
			}
		}
	}

	int width = ANativeWindow_getWidth(Backend_androidAppPtr->window);
	int height = ANativeWindow_getHeight(Backend_androidAppPtr->window);

	detail::displaySize[0] = (u32)width;
	detail::displaySize[1] = (u32)height;
	detail::mainWindowPos[0] = 0;
	detail::mainWindowPos[1] = 0;
	detail::mainWindowSize[0] = (u32)width;
	detail::mainWindowSize[1] = (u32)height;
	detail::mainWindowFramebufferSize[0] = (u32)width;
	detail::mainWindowFramebufferSize[1] = (u32)height;

	detail::mainWindowIsInFocus = true;
	detail::mainWindowIsMinimized = false;

	detail::Backend_androidAppPtr->onAppCmd = HandleCmd;
	detail::Backend_androidAppPtr->onInputEvent = HandleInputEvents;

	detail::Backend_currentOrientation = AConfiguration_getOrientation(Backend_androidAppPtr->config);
	if (detail::Backend_currentOrientation != ACONFIGURATION_ORIENTATION_LAND &&
		detail::Backend_currentOrientation != ACONFIGURATION_ORIENTATION_PORT)
	{
		Log("Error. Could not determine the orientation of the device.");
		abort();
	}
	return true;
}

void DEngine::Application::detail::Backend_ProcessEvents(
	std::chrono::high_resolution_clock::time_point now)
{
	if (!detail::Backend_androidWindowReady)
	{
		while (!detail::Backend_androidWindowReady)
		{
			// Keep polling events until the window is ready
			int events = 0; // What even is this?
			android_poll_source* source = nullptr;
			if (ALooper_pollAll(-1, nullptr, &events, (void**)&source) >= 0)
			{
				if (source != nullptr)
				{
					source->process(detail::Backend_androidAppPtr, source);
				}
			}
		}
	}
	else
	{
		int fileDescriptors = 0; // What even is this?
		android_poll_source* source = nullptr;
		if (ALooper_pollAll(1, nullptr, &fileDescriptors, (void**)&source) >= 0)
		{
			if (source != nullptr)
			{
				source->process(detail::Backend_androidAppPtr, source);
			}
		}
	}

	if (detail::Backend_configChangedThisTick)
		HandleConfigChanged(detail::Backend_androidAppPtr);
	detail::Backend_configChangedThisTick = false;
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
	createInfo.window = detail::Backend_androidAppPtr->window;

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

static DEngine::Application::TouchEventType DEngine::Application::detail::Backend_Android_MotionActionToTouchEventType(int32_t in)
{
	int32_t test = in & AMOTION_EVENT_ACTION_MASK;
	switch (test)
	{
		case AMOTION_EVENT_ACTION_DOWN:
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			return TouchEventType::Down;
		case AMOTION_EVENT_ACTION_UP:
		case AMOTION_EVENT_ACTION_POINTER_UP:
			return TouchEventType::Up;
		case AMOTION_EVENT_ACTION_MOVE:
			return TouchEventType::Moved;
		case AMOTION_EVENT_ACTION_CANCEL:
			return TouchEventType::Cancelled;
	}

	throw std::runtime_error("Unexpected motion-action");
	return TouchEventType(-1);
}

//
// File IO
//

namespace DEngine::Application::detail
{
	struct Backend_FileStreamData
	{
		AAsset* file = nullptr;
		off64_t pos = 0;
	};


}

DEngine::Std::Opt<DEngine::Application::FileStream> DEngine::Application::FileStream::OpenPath(char const* path)
{
	static_assert(sizeof(detail::Backend_FileStreamData) <= sizeof(FileStream::m_buffer),
				  "Error. Cannot fit implementation data in FileStream internal buffer.");

	AAsset* file = AAssetManager_open(detail::Backend_androidAppPtr->activity->assetManager, path, AASSET_MODE_RANDOM);
	if (file == nullptr)
		return {};

	detail::Backend_FileStreamData implData{};
	implData.file = file;
	implData.pos = 0;
	FileStream stream{};
	std::memcpy(&stream.m_buffer[0], &implData, sizeof(implData));
	return Std::Opt<FileStream>(static_cast<FileStream&&>(stream));
}

bool DEngine::Application::FileStream::Seek(i64 offset, SeekOrigin origin)
{
	detail::Backend_FileStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));

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
	off64_t result = AAsset_seek64(implData.file, (off64_t)offset, posixOrigin);
	if (result == -1)
		return false;

	// Update pos of our original buffer
	implData.pos = result;
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	return true;
}

bool DEngine::Application::FileStream::Read(char* output, u64 size)
{
	detail::Backend_FileStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));


	int result = AAsset_read(implData.file, output, (size_t)size);
	if (result < 0)
		return false;

	implData.pos += size;
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	return true;
}

DEngine::u64 DEngine::Application::FileStream::Tell()
{
	detail::Backend_FileStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));

	return (u64)implData.pos;
}

DEngine::Application::FileStream::FileStream(FileStream&& other) noexcept
{
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(detail::Backend_FileStreamData));
	std::memset(&other.m_buffer[0], 0, sizeof(detail::Backend_FileStreamData));
}

DEngine::Application::FileStream::~FileStream()
{
	detail::Backend_FileStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file != nullptr)
		AAsset_close(implData.file);
}