#include "detail_Application.hpp"

#include <android/window.h>
#include <android/log.h>
#include <jni.h>
#include <android_native_app_glue.h>

#define VK_USE_PLATFORM_ANDROID_KHR
#include "vulkan/vulkan.h"

#include <dlfcn.h>

#include <string>
#include <vector>
#include <mutex>

extern int dengine_app_detail_main(int argc, char** argv);

namespace DEngine::Application::detail
{
	struct CustomEvent
	{
		enum class Type
		{
			CharInput,
			CharRemove,
		};
		Type type;
		union
		{
			u32 charInput;
		};
	};

	struct BackendData
	{
		android_app* app = nullptr;
		bool androidAppReady = false;
		bool androidWindowReady = false;
		bool configChangedThisTick = false;

		std::vector<CustomEvent> customEventQueue;
		std::mutex customEventQueueLock;

		JNIEnv* jniEnv = nullptr;
		jclass jniActivity = nullptr;
		jmethodID jniGetCurrentOrientation{};
		jmethodID jniOpenSoftInput{};
	};

	BackendData* pBackendData = nullptr;

	static bool Backend_initJNI();

	static Orientation Backend_ToOrientation(int aconfiguration_orientation);
	static Orientation Backend_GetUpdatedOrientation();

	static Button Backend_AndroidKeyCodeToButton(int32_t in);

	static void Backend_HandleConfigChanged_Deferred();

	static void HandleCmd(android_app* app, int32_t cmd);
	// Return 1 if you have handled the event, 0 for any default
	// dispatching.
	static int32_t HandleInput(
			android_app* app,
			AInputEvent* event);

	static bool HandleInputEvents_Motion(
			AInputEvent* event,
			int32_t source);
	static bool HandleInputKeyEvents_Key(
			AInputEvent* event,
			int32_t source);

	static void Backend_HandleWindowInitializeEvent();
	static void Backend_HandleWindowTerminateEvent();
}

using namespace DEngine;

static void Application::detail::Backend_HandleWindowInitializeEvent()
{
	auto& backendData = *detail::pBackendData;
	backendData.androidWindowReady = true;

	if (!detail::pAppData->windows.empty())
	{
		Log("ANDROID TESTING!!! Window restore event");
		detail::pAppData->windows.front().platformHandle = backendData.app->window;
		detail::UpdateWindowMinimized(backendData.app->window, false);
	}
}

static void Application::detail::Backend_HandleWindowTerminateEvent()
{
	auto& backendData = *detail::pBackendData;

	detail::UpdateWindowMinimized(backendData.app->window, true);
}

static Application::Orientation Application::detail::Backend_ToOrientation(int aconfiguration_orientation)
{
	switch (aconfiguration_orientation)
	{
		case ACONFIGURATION_ORIENTATION_PORT:
			return Orientation::Portrait;
		case ACONFIGURATION_ORIENTATION_LAND:
			return Orientation::Landscape;
		default:
			return Orientation::Invalid;
	}
}

static bool Application::detail::Backend_initJNI()
{
	return true;
}

static Application::Orientation Application::detail::Backend_GetUpdatedOrientation()
{
	auto const& backendData = *detail::pBackendData;
	int a = backendData.jniEnv->CallIntMethod(
			backendData.app->activity->clazz,
			backendData.jniGetCurrentOrientation);
	Orientation b = Backend_ToOrientation(a);
	if (b == Orientation::Invalid)
		throw std::runtime_error("DEngine - Application: Tried to get new updated device orientation, but it failed.");
	return b;
}

static void Application::detail::HandleCmd(android_app* app, int32_t cmd)
{
	auto& backendData = *detail::pBackendData;
	switch (cmd)
	{
	case APP_CMD_INIT_WINDOW:
		Log("ANDROID TESTING!!!! Init window event");
		Backend_HandleWindowInitializeEvent();
		break;
	case APP_CMD_TERM_WINDOW:
		Log("ANDROID TESTING!!!! Terminate window event");
		Backend_HandleWindowTerminateEvent();
		break;
	case APP_CMD_LOST_FOCUS:
		break;
	case APP_CMD_GAINED_FOCUS:
		break;
	case APP_CMD_CONFIG_CHANGED:
		backendData.configChangedThisTick = true;
		break;
	case APP_CMD_CONTENT_RECT_CHANGED:
		Log("ANDROID TESTING!!!! Content rect changed");
		break;
	case APP_CMD_WINDOW_RESIZED:
		break;
	case APP_CMD_START:
		backendData.androidAppReady = true;
		Log("ANDROID TESTING!!!! App start event");
		break;
	case APP_CMD_STOP:
		Log("ANDROID TESTING!!!! App stop event");
		break;
	case APP_CMD_PAUSE:
		Log("ANDROID TESTING!!!! App pause event");
		break;
	case APP_CMD_RESUME:
		Log("ANDROID TESTING!!!! App resume event");
		break;
	case APP_CMD_WINDOW_REDRAW_NEEDED:
		break;
	case APP_CMD_INPUT_CHANGED:
		break;
	case APP_CMD_DESTROY:
		Log("ANDROID TESTING!!!! App destroy event");
		std::exit(0);
		break;
	case APP_CMD_SAVE_STATE:
		break;
	}
}

static bool Application::detail::HandleInputEvents_Motion(AInputEvent* event, int32_t source)
{
	bool handled = false;

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
				detail::UpdateTouchInput(TouchEventType::Down, (u8)id, x, y);
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
					detail::UpdateTouchInput(TouchEventType::Moved, (u8)id, x, y);
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
				detail::UpdateTouchInput(TouchEventType::Up, (u8)id, x, y);
				handled = true;
				break;
			}

			default:
				break;
		}
	}

	return handled;
}

static bool Application::detail::HandleInputKeyEvents_Key(
		AInputEvent* event,
		int32_t source)
{
	bool handled = false;

	int32_t keyAction = AKeyEvent_getAction(event);
	if (keyAction != AKEY_EVENT_ACTION_DOWN && keyAction != AKEY_EVENT_ACTION_UP)
		return false;

	bool buttonValue = false;
	switch (keyAction)
	{
	case AKEY_EVENT_ACTION_DOWN:
		buttonValue = true;
		break;
	case AKEY_EVENT_ACTION_UP:
		buttonValue = false;
		break;
	default:
		break;
	}

	return handled;
}

// Return 1 if you have handled the event, 0 for any default
// dispatching.
static int32_t Application::detail::HandleInput(
		android_app* app,
		AInputEvent* event)
{
	bool handled = false;

	int32_t inputType = AInputEvent_getType(event);
	int32_t source = AInputEvent_getSource(event);

	switch (inputType)
	{
	case AINPUT_EVENT_TYPE_KEY:
		handled = detail::HandleInputKeyEvents_Key(event, source);
		break;
	case AINPUT_EVENT_TYPE_MOTION:
		handled = detail::HandleInputEvents_Motion(event, source);
		break;
	default:
		throw std::runtime_error("Error. Unexpected Android input event.");
	}

	return handled ? 1 : 0;
}

static void Application::detail::Backend_HandleConfigChanged_Deferred()
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;

	// AConfiguration_getOrientation is broken
	// So we use JNI.
	Orientation newOrientation = Backend_GetUpdatedOrientation();
	if (newOrientation == Orientation::Invalid)
		throw std::runtime_error("DEngine - App: Unable to query for updated Android orientation.");

	detail::UpdateOrientation(newOrientation);
	// If we had a reorientation and the window is non-square,
	// we need to resize the window.
	if (appData.orientationEvent && !appData.windows.empty())
	{
		Extent windowExtent = appData.windows.front().windowData.size;
		detail::UpdateWindowSize(backendData.app->window, { windowExtent.height, windowExtent.width });
	}
}

// Main entry point for our program.
// This gets ran by the new thread that our Android app starts.
void android_main(android_app* app)
{
	Application::detail::pBackendData = new Application::detail::BackendData;
	auto& backendData = *Application::detail::pBackendData;
	backendData.app = app;
	dengine_app_detail_main(0, nullptr);
}

bool Application::detail::Backend_Initialize()
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;

	// First we attach this thread to the JVM
	backendData.app->activity->vm->AttachCurrentThread(&backendData.jniEnv, nullptr);

	// I have no idea how this code works.
	// It does some crazy JNI bullshit to get some handles,
	// which in turn we need to grab Java function-pointer/handles
	// to our Java-side functions.
	//
	// I think this code can be shortened by reusing some values
	// inside the android_app struct. I don't actually know.
	jclass clazz = backendData.jniEnv->GetObjectClass(backendData.app->activity->clazz);
	jmethodID getApplication = backendData.jniEnv->GetMethodID(clazz,
			"getApplication",
			"()Landroid/app/Application;");
	jobject application = backendData.jniEnv->CallObjectMethod(backendData.app->activity->clazz, getApplication);
	jclass applicationClass = backendData.jniEnv->GetObjectClass(application);
	jmethodID getApplicationContext = backendData.jniEnv->GetMethodID(
			applicationClass,
			"getApplicationContext",
			"()Landroid/content/Context;");
	jobject context = backendData.jniEnv->CallObjectMethod(application, getApplicationContext);
	jclass contextClass = backendData.jniEnv->GetObjectClass(context);
	jmethodID getClassLoader = backendData.jniEnv->GetMethodID(
			contextClass,
			"getClassLoader",
			"()Ljava/lang/ClassLoader;");
	jobject classLoader = backendData.jniEnv->CallObjectMethod(context, getClassLoader);
	jclass classLoaderClass = backendData.jniEnv->GetObjectClass(classLoader);
	jmethodID loadClass = backendData.jniEnv->GetMethodID(
			classLoaderClass,
			"loadClass",
			"(Ljava/lang/String;)Ljava/lang/Class;");
	jstring activityName = backendData.jniEnv->NewStringUTF("didgy.dengine.editor.DEngineActivity");
	backendData.jniActivity = static_cast<jclass>(backendData.jniEnv->CallObjectMethod(
			classLoader,
			loadClass,
			activityName));
	backendData.jniGetCurrentOrientation = backendData.jniEnv->GetMethodID(
			backendData.jniActivity,
			"getCurrentOrientation",
			"()I");
	backendData.jniOpenSoftInput = backendData.jniEnv->GetMethodID(
			backendData.jniActivity,
			"openSoftInput",
			"()V");

	// Figure out current orientation
	appData.currentOrientation = Backend_GetUpdatedOrientation();

	// Set the callback to process system events
	backendData.app->onAppCmd = HandleCmd;
	while (!backendData.androidWindowReady)
	{
		// Keep polling events until the window is ready
		int fileDescriptors = 0; // What even is this?
		android_poll_source* source = nullptr;
		int pollResult = ALooper_pollAll(
				0,
				nullptr,
				&fileDescriptors,
				(void**)&source);
		if (pollResult >= 0)
		{
			if (source != nullptr)
			{
				source->process(backendData.app, source);
			}
		}
	}

	backendData.app->onInputEvent = HandleInput;

	return true;
}

void Application::detail::Backend_ProcessEvents()
{
	auto& backendData = *detail::pBackendData;


	int fileDescriptors = 0; // What even is this?
	android_poll_source* source = nullptr;
	int pollResult = ALooper_pollAll(
			0,
			nullptr,
			&fileDescriptors,
			(void**)&source);
	if (pollResult >= 0)
	{
		if (source != nullptr)
		{
			source->process(backendData.app, source);
		}
	}

	if (backendData.configChangedThisTick)
		Backend_HandleConfigChanged_Deferred();
	backendData.configChangedThisTick = false;

	{
		// Process our custom events
		std::lock_guard _{ backendData.customEventQueueLock};
		for (auto const& event : backendData.customEventQueue)
		{
			switch (event.type)
			{
			case CustomEvent::Type::CharInput:
				detail::PushCharInput(event.charInput);
				break;
			case CustomEvent::Type::CharRemove:
				detail::PushCharRemoveEvent();
				break;
			default:
				break;
			}
		}

		backendData.customEventQueue.clear();
	}
}

Application::WindowID Application::CreateWindow(
		char const* title,
		Extent extents)
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;

	detail::AppData::WindowNode newNode{};
	newNode.id = (App::WindowID)appData.windowIdTracker;
	appData.windowIdTracker++;
	newNode.platformHandle = backendData.app->window;

	int width = ANativeWindow_getWidth(backendData.app->window);
	int height = ANativeWindow_getHeight(backendData.app->window);
	newNode.windowData.size.width = (u32)width;
	newNode.windowData.size.height = (u32)height;

	appData.windows.push_back(newNode);

	return newNode.id;
}

Std::Opt<u64> Application::CreateVkSurface(
		WindowID window,
		uSize vkInstance,
		void const* vkAllocationCallbacks)
{
	PFN_vkGetInstanceProcAddr procAddr = nullptr;
	void* lib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (lib == nullptr)
		return Std::nullOpt;
	procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(lib, "vkGetInstanceProcAddr"));
	int closeReturnCode = dlclose(lib);
	lib = nullptr;
	if (procAddr == nullptr || closeReturnCode != 0)
		return Std::nullOpt;

	auto funcPtr = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
			procAddr(
					*reinterpret_cast<VkInstance*>(&vkInstance),
					"vkCreateAndroidSurfaceKHR"));
	if (funcPtr == nullptr)
		return Std::nullOpt;

	VkAndroidSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.window = detail::pBackendData->app->window;

	VkSurfaceKHR returnVal{};
	VkResult result = funcPtr(
			*reinterpret_cast<VkInstance*>(&vkInstance),
			&createInfo,
			reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks),
			&returnVal);
	if (result != VK_SUCCESS || returnVal == VkSurfaceKHR())
		return Std::nullOpt;

	u64 temp = *reinterpret_cast<u64*>(&returnVal);

	return { temp };
}

Std::StaticVector<char const*, 5> Application::RequiredVulkanInstanceExtensions()
{
	Std::StaticVector<char const*, 5> returnVal{};
	returnVal.PushBack("VK_KHR_surface");
	returnVal.PushBack("VK_KHR_android_surface");
	return returnVal;
}

void Application::detail::Backend_Log(char const* msg)
{
	__android_log_print(ANDROID_LOG_ERROR, "DEngine: ", "%s", msg);
}

static Application::Button Application::detail::Backend_AndroidKeyCodeToButton(int32_t in)
{
	switch (in)
	{
	case AKEYCODE_0:
		return Button::Zero;
	case AKEYCODE_1:
		return Button::One;
	case AKEYCODE_2:
		return Button::Two;
	case AKEYCODE_3:
		return Button::Three;
	case AKEYCODE_4:
		return Button::Four;
	case AKEYCODE_5:
		return Button::Five;
	case AKEYCODE_6:
		return Button::Six;
	case AKEYCODE_7:
		return Button::Seven;
	case AKEYCODE_8:
		return Button::Eight;
	case AKEYCODE_9:
		return Button::Nine;

	case AKEYCODE_DEL:
		return Button::Delete;
	case AKEYCODE_BACK:
		return Button::Back;
	default:
		return Button::Undefined;
	}
}

void Application::OpenSoftInput()
{
	const auto& backendData = *detail::pBackendData;
	backendData.jniEnv->CallVoidMethod(backendData.app->activity->clazz, backendData.jniOpenSoftInput);
}


extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_editor_DEngineActivity_nativeTestFunc(
	JNIEnv *env,
	jobject thiz,
	int utfValue)
{
		using namespace DEngine::Application;
		using namespace DEngine::Application::detail;
		auto& backendData = *Application::detail::pBackendData;

		std::lock_guard _(backendData.customEventQueueLock );

		CustomEvent event{};
		event.type = CustomEvent::Type::CharInput;
		event.charInput = utfValue;

		backendData.customEventQueue.push_back(event);
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_editor_DEngineActivity_charRemoveEventNative(
		JNIEnv *env,
		jobject thiz)
{
	// TODO: implement charRemoveEventNative()
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;
	auto& backendData = *Application::detail::pBackendData;

	std::lock_guard _(backendData.customEventQueueLock );

	CustomEvent event{};
	event.type = CustomEvent::Type::CharRemove;

	backendData.customEventQueue.push_back(event);
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_MainActivity_nativeTestFunc(
		JNIEnv *env,
		jobject thiz)
{
	// TODO: implement nativeTestFunc()

	int x = 0;
	if (x > 2)
	{

	}
}

void Application::LockCursor(bool state)
{

}


//
// File IO
//

namespace DEngine::Application::detail
{
	struct Backend_FileInputStreamData
	{
		AAsset* file = nullptr;
		// There is no AAsset_tell() so we need to remember the position manually.
		size_t pos = 0;
	};
}

DEngine::Application::FileInputStream::FileInputStream()
{
	static_assert(sizeof(detail::Backend_FileInputStreamData) <= sizeof(FileInputStream::m_buffer));

	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}

DEngine::Application::FileInputStream::FileInputStream(char const* path)
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	Open(path);
}

DEngine::Application::FileInputStream::FileInputStream(FileInputStream&& other) noexcept
{
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(detail::Backend_FileInputStreamData));
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));
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

	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(detail::Backend_FileInputStreamData));
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));

	return *this;
}


bool DEngine::Application::FileInputStream::Seek(i64 offset, SeekOrigin origin)
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));

	if (implData.file == nullptr)
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
	off64_t result = AAsset_seek64(implData.file, static_cast<off64_t>(offset), posixOrigin);
	if (result == -1)
		return false;

	implData.pos = static_cast<size_t>(result);
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	return true;
}

bool DEngine::Application::FileInputStream::Read(char* output, u64 size)
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file == nullptr)
		return false;

	int result = AAsset_read(implData.file, output, (size_t)size);
	if (result < 0)
		return false;

	implData.pos += size;
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	return true;
}

DEngine::Std::Opt<DEngine::u64> DEngine::Application::FileInputStream::Tell() const
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file == nullptr)
		return {};

	return (u64)implData.pos;
}

bool DEngine::Application::FileInputStream::IsOpen() const
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	return implData.file != nullptr;
}

bool DEngine::Application::FileInputStream::Open(char const* path)
{
	Close();

	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	auto const& backendData = *detail::pBackendData;
	implData.file = AAssetManager_open(backendData.app->activity->assetManager, path, AASSET_MODE_RANDOM);
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
	return implData.file != nullptr;
}

void DEngine::Application::FileInputStream::Close()
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file != nullptr)
		AAsset_close(implData.file);

	implData = detail::Backend_FileInputStreamData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}