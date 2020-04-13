#include "detail_Application.hpp"

#include <android/log.h>
#include <android_native_app_glue.h>
#include <jni.h>

#define VK_USE_PLATFORM_ANDROID_KHR
#include "vulkan/vulkan.h"

#include <dlfcn.h>

#include <string>

extern int dengine_app_detail_main(int argc, char** argv);

namespace DEngine::Application::detail
{
	enum class Backend_Orientation
	{
		Invalid,
		Portrait,
		Landscape
	};
	static Backend_Orientation Backend_ToOrientation(int aconfiguration_orientation);
	static Backend_Orientation Backend_initialOrientation = Backend_Orientation::Invalid;
	static Backend_Orientation Backend_currentOrientation = Backend_Orientation::Invalid;

	static android_app* Backend_app = nullptr;
	static bool Backend_androidAppReady = false;
	static bool Backend_androidWindowReady = false;
	static bool Backend_configChangedThisTick = false;
	static u32 Backend_initialWindowResolution[2] = {};

	static JNIEnv* Backend_jniEnv = nullptr;
	static jclass Backend_jniActivity = nullptr;
	static jmethodID Backend_jniGetCurrentOrientation{};
	static bool Backend_initJNI();

	// Grabs the updated orientation
	static Backend_Orientation Backend_GetUpdatedOrientation();
	static Button Backend_AndroidKeyCodeToButton(int32_t in);

	static TouchEventType Backend_Android_MotionActionToTouchEventType(int32_t in);

	static void Backend_HandleConfigChanged_Deferred();

	static void HandleCmd(android_app* app, int32_t cmd)
	{
		switch (cmd)
		{
			case APP_CMD_INIT_WINDOW:
				Log("Init window");
				detail::mainWindowSurfaceInitialized = true;
				detail::mainWindowSurfaceInitializeEvent = true;

				detail::Backend_androidWindowReady = true;

				if (detail::tickCount > 0)
					detail::mainWindowRestoreEvent = true;
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
				detail::Backend_androidAppReady = true;
				break;
			case APP_CMD_STOP:
				Log("Stop");
				detail::Backend_androidAppReady = false;
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
				detail::shouldShutdown = true;
				break;
			case APP_CMD_SAVE_STATE:
				Log("Save state");
				break;
		}
	}

	// Return 1 if you have handled the event, 0 for any default
	// dispatching.
	static int32_t HandleInputEvents(android_app* app, AInputEvent* event)
	{
		bool handled = false;

		int32_t inputType = AInputEvent_getType(event);
		int32_t source = AInputEvent_getSource(event);

		switch (inputType)
		{
			case AINPUT_EVENT_TYPE_KEY:
			{
				int32_t keyAction = AKeyEvent_getAction(event);
				if (keyAction != AKEY_EVENT_ACTION_DOWN && keyAction != AKEY_EVENT_ACTION_UP)
					break;

				bool buttonValue = false;
				switch (keyAction)
				{
					case AKEY_EVENT_ACTION_DOWN:
						buttonValue = true;
						break;
					case AKEY_EVENT_ACTION_UP:
						buttonValue = false;
						break;
				}
				int32_t keyCode = AKeyEvent_getKeyCode(event);
				Button buttonCode = Backend_AndroidKeyCodeToButton(keyCode);
				if (buttonCode != Button::Undefined)
				{
					detail::UpdateButton(buttonCode, buttonValue);
					handled = true;
				}

			}
				break;
			case AINPUT_EVENT_TYPE_MOTION:
			{
				int32_t pointer = AMotionEvent_getAction(event);
				int32_t action = pointer & AMOTION_EVENT_ACTION_MASK;
				size_t index = size_t((pointer & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
				if (source == AINPUT_SOURCE_TOUCHSCREEN)
				{
					switch (action)
					{
						case AMOTION_EVENT_ACTION_DOWN:
						case AMOTION_EVENT_ACTION_POINTER_DOWN:
						{
							f32 x = AMotionEvent_getX(event, index);
							f32 y = AMotionEvent_getY(event, index);
							i32 id = AMotionEvent_getPointerId(event, index);
							detail::UpdateTouchInput_Down((u8)id, x, y);
							handled = true;
							break;
						}

						case AMOTION_EVENT_ACTION_MOVE:
						{
							size_t count = AMotionEvent_getPointerCount(event);
							for (size_t i = 0; i < count; i++)
							{
								f32 x = AMotionEvent_getX(event, i);
								f32 y = AMotionEvent_getY(event, i);
								i32 id = AMotionEvent_getPointerId(event, i);
								detail::UpdateTouchInput_Move((u8)id, x, y);
							}
							handled = true;
							break;
						}

						case AMOTION_EVENT_ACTION_UP:
						case AMOTION_EVENT_ACTION_POINTER_UP:
						{
							f32 x = AMotionEvent_getX(event, index);
							f32 y = AMotionEvent_getY(event, index);
							i32 id = AMotionEvent_getPointerId(event, index);
							detail::UpdateTouchInput_Up((u8)id, x, y);
							handled = true;
							break;
						}

						default:
							break;
					}
				}
			}
			break;
		}



		return handled ? 1 : 0;
	}
}

static DEngine::Application::detail::Backend_Orientation DEngine::Application::detail::Backend_ToOrientation(int aconfiguration_orientation)
{
	switch (aconfiguration_orientation)
	{
		case ACONFIGURATION_ORIENTATION_PORT:
			return Backend_Orientation::Portrait;
		case ACONFIGURATION_ORIENTATION_LAND:
			return Backend_Orientation::Landscape;
		default:
			return Backend_Orientation::Invalid;
	}
}

static bool DEngine::Application::detail::Backend_initJNI()
{
	return true;
}

static DEngine::Application::detail::Backend_Orientation DEngine::Application::detail::Backend_GetUpdatedOrientation()
{
	int a = Backend_jniEnv->CallIntMethod(Backend_app->activity->clazz, Backend_jniGetCurrentOrientation);
	Backend_Orientation b = Backend_ToOrientation(a);
	return b;
}

static void DEngine::Application::detail::Backend_HandleConfigChanged_Deferred()
{
	// AConfiguration_getOrientation is broken
	// So we use JNI.
	Backend_Orientation newOrientation = Backend_GetUpdatedOrientation();
	if (newOrientation != Backend_currentOrientation)
	{
		// Our orientation changed
		Backend_currentOrientation = newOrientation;
		// TODO: Add an orientation event to the front-end. Then fill it out here.

		// We had a reorientation, so we need to update resolution.
		// Normally we would use `ANativeWindow_getWidth` but
		// it will only report the *previous* value it was holding.
		// It's probably bugged...
		// So we use a workaround.
		// Since we either went from portrait->landscape or viceversa, we just
		// mirror the existing dimensions.
		u32 tempSize[2] = { detail::displaySize[0], detail::displaySize[1] };
		detail::displaySize[0] = tempSize[1];
		detail::displaySize[1] = tempSize[0];
		detail::mainWindowSize[0] = tempSize[1];
		detail::mainWindowSize[1] = tempSize[0];
		detail::mainWindowFramebufferSize[0] = tempSize[1];
		detail::mainWindowFramebufferSize[1] = tempSize[0];

		detail::mainWindowResizeEvent = true;

		std::string a = std::string("Queried dims are: ") + std::to_string(detail::displaySize[0]) + ", " + std::to_string(detail::displaySize[1]);
		Log(a.c_str());
	}
}

// Main entry point for our program.
// This gets ran by the new thread that our Android app starts.
void android_main(android_app* app)
{
	DEngine::Application::detail::Backend_app = app;
	dengine_app_detail_main(0, nullptr);
}

bool DEngine::Application::detail::Backend_Initialize()
{
	Backend_app->activity->vm->AttachCurrentThread(&Backend_jniEnv, nullptr);

	jclass clazz = Backend_jniEnv->GetObjectClass(Backend_app->activity->clazz);
	jmethodID getApplication = Backend_jniEnv->GetMethodID(clazz, "getApplication", "()Landroid/app/Application;");
	jobject application = Backend_jniEnv->CallObjectMethod(Backend_app->activity->clazz, getApplication);

	jclass applicationClass = Backend_jniEnv->GetObjectClass(application);
	jmethodID getApplicationContext = Backend_jniEnv->GetMethodID(applicationClass, "getApplicationContext", "()Landroid/content/Context;");
	jobject context = Backend_jniEnv->CallObjectMethod(application, getApplicationContext);


	jclass contextClass = Backend_jniEnv->GetObjectClass(context);
	jmethodID getClassLoader = Backend_jniEnv->GetMethodID(contextClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
	jobject classLoader = Backend_jniEnv->CallObjectMethod(context, getClassLoader);

	jclass classLoaderClass = Backend_jniEnv->GetObjectClass(classLoader);
	jmethodID loadClass = Backend_jniEnv->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");


	jstring activityName = Backend_jniEnv->NewStringUTF("didgy.dengine.editor.DEngineActivity");
	Backend_jniActivity = static_cast<jclass>(Backend_jniEnv->CallObjectMethod(classLoader, loadClass, activityName));

	Backend_jniGetCurrentOrientation = Backend_jniEnv->GetMethodID(Backend_jniActivity, "getCurrentOrientation", "()I");



	// Set the callback to process system events
	detail::Backend_app->onAppCmd = HandleCmd;
	while (!detail::Backend_androidAppReady || !detail::Backend_androidWindowReady)
	{
		// Keep polling events until the window is ready
		int fileDescriptors = 0; // What even is this?
		android_poll_source* source = nullptr;
		if (ALooper_pollAll(-1, nullptr, &fileDescriptors, (void**)&source) >= 0)
		{
			if (source != nullptr)
			{
				source->process(detail::Backend_app, source);
			}
		}
	}

	int width = ANativeWindow_getWidth(Backend_app->window);
	int height = ANativeWindow_getHeight(Backend_app->window);
	std::string a = std::string("Queried dims are: ") + std::to_string(width) + ", " + std::to_string(height);
	Log(a.c_str());

	Backend_initialWindowResolution[0] = (u32)width;
	Backend_initialWindowResolution[1] = (u32)height;
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

	detail::Backend_app->onAppCmd = HandleCmd;
	detail::Backend_app->onInputEvent = HandleInputEvents;

	Backend_initialOrientation = Backend_GetUpdatedOrientation();
	Backend_currentOrientation = Backend_initialOrientation;

	return true;
}

void DEngine::Application::detail::Backend_ProcessEvents()
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
					source->process(detail::Backend_app, source);
				}
			}

			if (detail::shouldShutdown)
				break;
		}
	}
	else
	{
		int fileDescriptors = 0; // What even is this?
		android_poll_source* source = nullptr;
		if (ALooper_pollAll(0, nullptr, &fileDescriptors, (void**)&source) >= 0)
		{
			if (source != nullptr)
			{
				source->process(detail::Backend_app, source);
			}
		}
	}

	if (detail::Backend_configChangedThisTick)
		Backend_HandleConfigChanged_Deferred();
	detail::Backend_configChangedThisTick = false;
}

bool DEngine::Application::detail::CreateVkSurface(
	uSize vkInstance,
	void const* vkAllocationCallbacks, 
	void* userData, 
	u64& vkSurface)
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
	createInfo.window = detail::Backend_app->window;

	VkSurfaceKHR returnVal{};
	VkResult result = funcPtr(
			*reinterpret_cast<VkInstance*>(&vkInstance),
			&createInfo,
			reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks),
			&returnVal);
	if (result != VK_SUCCESS || returnVal == VkSurfaceKHR())
		return false;

	vkSurface = *reinterpret_cast<u64 const*>(&returnVal);

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
		case AMOTION_EVENT_ACTION_MOVE:
			return TouchEventType::Moved;
	}

	throw std::runtime_error("Unexpected motion-action");
	return TouchEventType(-1);
}

static DEngine::Application::Button DEngine::Application::detail::Backend_AndroidKeyCodeToButton(int32_t in)
{
	switch (in)
	{
		case AKEYCODE_BACK:
			return Button::Back;
		default:
			return Button::Undefined;
	}
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

	AAsset* file = AAssetManager_open(detail::Backend_app->activity->assetManager, path, AASSET_MODE_RANDOM);
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