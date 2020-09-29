#include "detail_Application.hpp"

#include <android/configuration.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <jni.h>

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>

#include <dlfcn.h>

#include <string>
#include <thread>
#include <vector>
#include <mutex>

extern int dengine_app_detail_main(int argc, char** argv);

namespace DEngine::Application::detail
{
	struct BackendData;

	enum class LooperID
	{
		Input = 1
	};

	struct AndroidPollSource {
		LooperID looperId = LooperID::Input;

		BackendData* backendData = nullptr;
	};

	struct CustomEvent
	{
		enum class Type
		{
			CharInput,
			CharRemove,
			CharEnter,
		};
		Type type;
		union
		{
			u32 charInput;
		};
	};

	struct BackendData
	{
		ANativeActivity* activity = nullptr;

		AndroidPollSource inputPollSource{};

		std::thread gameThread;
		std::mutex lock;
		ANativeWindow* nativeWindow = nullptr;
		u32 nativeWindowVisibleOffsetX = 0;
		u32 nativeWindowVisibleOffsetY = 0;
		Extent nativeWindowVisibleExtent{};
		bool windowVisibleRectInitialized = false;
		AInputQueue* inputQueue = nullptr;

		jobject mainActivity = nullptr;
		// JNI Envs are per-thread. This is for the game thread, it is created when attaching
		// the thread to the JavaVM.
		JNIEnv* gameThreadJniEnv = nullptr;
		jmethodID jniOpenSoftInput = nullptr;
		jmethodID jniHideSoftInput = nullptr;
		std::vector<CustomEvent> customEventQueue;
	};

	BackendData* pBackendData = nullptr;

	static bool Backend_initJNI();

	static Orientation Backend_ToOrientation(int aconfiguration_orientation);
	static Orientation Backend_GetUpdatedOrientation();

	static Button Backend_AndroidKeyCodeToButton(int32_t in);

	static void Backend_HandleConfigChanged_Deferred();

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

namespace DEngine::Application::detail
{
	static void onStart(ANativeActivity* activity)
	{

	}

	static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window)
	{
		auto& backendData = *detail::pBackendData;
		std::lock_guard _{ backendData.lock };

		backendData.nativeWindow = window;
	}

	static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
	{
		auto& backendData = *detail::pBackendData;
		std::lock_guard _{ backendData.lock };

		backendData.inputQueue = queue;
	}
}

extern "C"
JNIEXPORT void JNICALL ANativeActivity_onCreate(
	ANativeActivity* activity,
	void* savedState,
	size_t savedStateSize)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	Application::detail::pBackendData = new Application::detail::BackendData();
	auto& backendData = *Application::detail::pBackendData;

	backendData.activity = activity;

	activity->callbacks->onStart = &Application::detail::onStart;
	activity->callbacks->onNativeWindowCreated = &Application::detail::onNativeWindowCreated;
	activity->callbacks->onInputQueueCreated = &Application::detail::onInputQueueCreated;
}

extern "C"
JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeInit(
	JNIEnv* env,
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	auto& backendData = *Application::detail::pBackendData;

	backendData.mainActivity = dengineActivity;

	backendData.inputPollSource.backendData = &backendData;

	backendData.customEventQueue.reserve(25);

	auto lambda = []()
	{
		DENGINE_APP_MAIN_ENTRYPOINT(0, nullptr);
	};

	backendData.gameThread = std::thread(lambda);
	auto threadName = "MainGameThread";
	pthread_setname_np(backendData.gameThread.native_handle(), threadName);
}

extern "C"
JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnCharInput(
	JNIEnv* env,
	jobject dengineActivity,
	jint utfValue)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	auto& backendData = *Application::detail::pBackendData;
	std::lock_guard _{ backendData.lock };

	CustomEvent newEvent{};
	newEvent.type = CustomEvent::Type::CharInput;
	newEvent.charInput = utfValue;
	backendData.customEventQueue.push_back(newEvent);
}

extern "C"
JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnCharEnter(
	JNIEnv* env,
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	auto& backendData = *Application::detail::pBackendData;
	std::lock_guard _{ backendData.lock };

	CustomEvent newEvent{};
	newEvent.type = CustomEvent::Type::CharEnter;
	backendData.customEventQueue.push_back(newEvent);
}

extern "C"
JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnCharRemove(
	JNIEnv* env,
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	auto& backendData = *Application::detail::pBackendData;
	std::lock_guard _{ backendData.lock };

	CustomEvent newEvent{};
	newEvent.type = CustomEvent::Type::CharRemove;
	backendData.customEventQueue.push_back(newEvent);
}

extern "C"
JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeUpdateWindowContentRect(
	JNIEnv* env,
	jobject dengineActivity,
	jint top,
	jint bottom,
	jint left,
	jint right)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	auto& backendData = *Application::detail::pBackendData;
	std::lock_guard _{ backendData.lock };
	backendData.nativeWindowVisibleOffsetY = (u32)top;
	backendData.nativeWindowVisibleOffsetX = (u32)left;
	//std::swap(backendData.nativeWindowVisibleOffsetX, backendData.nativeWindowVisibleOffsetY);
	backendData.nativeWindowVisibleExtent.width = right - left;
	backendData.nativeWindowVisibleExtent.height = bottom - top;
	//std::swap(backendData.nativeWindowVisibleExtent.width, backendData.nativeWindowVisibleExtent.height);
	backendData.windowVisibleRectInitialized = true;
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
	return {};
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
	}
}

namespace DEngine::Application::detail
{
	int testCallback(int fd, int events, void *data)
	{
		AndroidPollSource& androidPollSource = *(AndroidPollSource*)data;
		auto& backendData = *androidPollSource.backendData;
		if ((unsigned int)events & ALOOPER_EVENT_INPUT)
		{
			while (true)
			{
				AInputEvent* event = nullptr;
				int32_t eventIndex = AInputQueue_getEvent(backendData.inputQueue, &event);
				if (eventIndex < 0)
					break;

				if (AInputQueue_preDispatchEvent(backendData.inputQueue, event) != 0) {
					continue;
				}
				int handled = 0;

				int32_t eventType = AInputEvent_getType(event);
				int32_t eventSource = AInputEvent_getSource(event);

				if (eventType == AINPUT_EVENT_TYPE_MOTION)
				{
					handled = HandleInputEvents_Motion(event, eventSource);
				}

				AInputQueue_finishEvent(backendData.inputQueue, event, handled);
			}
		}

		return 1;
	}
}

bool Application::detail::Backend_Initialize()
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;

	// First we attach this thread to the JVM
	jint attachThreadResult = backendData.activity->vm->AttachCurrentThread(
		&backendData.gameThreadJniEnv,
		nullptr);
	if (attachThreadResult != JNI_OK)
	{
		// Attaching failed. Crash the program.
		std::abort();
	}
	// I have no idea how this code works.
	// It does some crazy JNI bullshit to get some handles,
	// which in turn we need to grab Java function-pointer/handles
	// to our Java-side functions.
	//
	// I think this code can be shortened by reusing some values
	// inside the android_app struct. I don't actually know.

	jclass clazz = backendData.gameThreadJniEnv->GetObjectClass(backendData.activity->clazz);
	jmethodID getApplication = backendData.gameThreadJniEnv->GetMethodID(
		clazz,
		"getApplication",
		"()Landroid/app/Application;");
	jobject application = backendData.gameThreadJniEnv->CallObjectMethod(backendData.activity->clazz, getApplication);
	jclass applicationClass = backendData.gameThreadJniEnv->GetObjectClass(application);
	jmethodID getApplicationContext = backendData.gameThreadJniEnv->GetMethodID(
		applicationClass,
		"getApplicationContext",
		"()Landroid/content/Context;");
	jobject context = backendData.gameThreadJniEnv->CallObjectMethod(application, getApplicationContext);
	jclass contextClass = backendData.gameThreadJniEnv->GetObjectClass(context);
	jmethodID getClassLoader = backendData.gameThreadJniEnv->GetMethodID(
		contextClass,
		"getClassLoader",
		"()Ljava/lang/ClassLoader;");
	jobject classLoader = backendData.gameThreadJniEnv->CallObjectMethod(context, getClassLoader);
	jclass classLoaderClass = backendData.gameThreadJniEnv->GetObjectClass(classLoader);
	jmethodID loadClass = backendData.gameThreadJniEnv->GetMethodID(
			classLoaderClass,
			"loadClass",
			"(Ljava/lang/String;)Ljava/lang/Class;");


	jstring activityName = backendData.gameThreadJniEnv->NewStringUTF("didgy.dengine.editor.DEngineActivity");
	jclass activity = static_cast<jclass>(backendData.gameThreadJniEnv->CallObjectMethod(
			classLoader,
			loadClass,
			activityName));

	backendData.jniOpenSoftInput = backendData.gameThreadJniEnv->GetMethodID(
		activity,
			"openSoftInput",
			"(Ljava/lang/String;)V");
	backendData.jniHideSoftInput = backendData.gameThreadJniEnv->GetMethodID(
		activity,
		"hideSoftInput",
		"()V");

	// Figure out current orientation
	//appData.currentOrientation = Backend_GetUpdatedOrientation();


	while (true)
	{
		std::lock_guard _{ backendData.lock };
		if (backendData.nativeWindow && backendData.windowVisibleRectInitialized && backendData.inputQueue)
			break;
	}

	ALooper* looper = ALooper_prepare(0); // ALOOPER_PREPARE_ALLOW_NON_CALLBACKS ?
	AInputQueue_attachLooper(
		backendData.inputQueue,
		looper,
		(int)LooperID::Input,
		&testCallback,
		&backendData.inputPollSource);
	return true;
}

void Application::detail::Backend_ProcessEvents()
{
	auto& backendData = *detail::pBackendData;

	int eventCount = 0; // What even is this?
	int fileDescriptor = 0;
	AndroidPollSource* source = nullptr;
	int pollResult = ALooper_pollAll(
		0,
		&fileDescriptor,
		&eventCount,
		(void**)&source);
	if (pollResult >= 0)
	{
		auto looperId = (LooperID)fileDescriptor; // This is wrong.
		if (source != nullptr)
		{

		}
	}

	// Process our custom events
	{
		std::lock_guard _{ backendData.lock };
		for (auto const event : backendData.customEventQueue)
		{
			switch (event.type)
			{
				case CustomEvent::Type::CharInput:
					detail::PushCharInput(event.charInput);
					break;
				case CustomEvent::Type::CharRemove:
					detail::PushCharRemoveEvent();
					break;
				case CustomEvent::Type::CharEnter:
					detail::PushCharEnterEvent();
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

	int width = ANativeWindow_getWidth(backendData.nativeWindow);
	int height = ANativeWindow_getHeight(backendData.nativeWindow);
	newNode.windowData.size.width = (u32)width;
	newNode.windowData.size.height = (u32)height;
	newNode.windowData.visiblePosition = { (i32)backendData.nativeWindowVisibleOffsetX, (i32)backendData.nativeWindowVisibleOffsetY };
	newNode.windowData.visibleSize = backendData.nativeWindowVisibleExtent;

	appData.windows.push_back(newNode);

	return WindowID(0);
}

Std::Opt<u64> Application::CreateVkSurface(
		WindowID window,
		uSize vkInstance,
		void const* vkAllocationCallbacks)
{
	PFN_vkGetInstanceProcAddr procAddr = nullptr;
	void* lib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (lib == nullptr)
		return {};
	procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(lib, "vkGetInstanceProcAddr"));
	int closeReturnCode = dlclose(lib);
	lib = nullptr;
	if (procAddr == nullptr || closeReturnCode != 0)
		return {};

	auto funcPtr = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
			procAddr(
					*reinterpret_cast<VkInstance*>(&vkInstance),
					"vkCreateAndroidSurfaceKHR"));
	if (funcPtr == nullptr)
		return {};


	VkAndroidSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.window = detail::pBackendData->nativeWindow;

	VkSurfaceKHR returnVal{};
	VkResult result = funcPtr(
			*reinterpret_cast<VkInstance*>(&vkInstance),
			&createInfo,
			reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks),
			&returnVal);
	if (result != VK_SUCCESS || returnVal == VkSurfaceKHR())
		return Std::nullOpt;

	u64 temp = *reinterpret_cast<u64*>(&returnVal);

	return Std::Opt{ temp };
}

Std::StackVec<char const*, 5> Application::RequiredVulkanInstanceExtensions()
{
	Std::StackVec<char const*, 5> returnVal{};
	returnVal.PushBack("VK_KHR_surface");
	returnVal.PushBack("VK_KHR_android_surface");
	return returnVal;
}

void Application::SetCursor(WindowID id, CursorType cursorType)
{

}

void Application::detail::Backend_Log(char const* msg)
{
	__android_log_print(ANDROID_LOG_ERROR, "DEngine: ", "%s", msg);
}



void Application::OpenSoftInput(std::string_view text)
{
	auto const& backendData = *detail::pBackendData;
	std::basic_string<jchar> tempString;
	tempString.reserve(text.size());
	for (auto item : text)
	{
		tempString.push_back(item);
	}

	jstring javaString = backendData.gameThreadJniEnv->NewString(tempString.data(), tempString.length());

	backendData.gameThreadJniEnv->CallVoidMethod(backendData.activity->clazz, backendData.jniOpenSoftInput, javaString);

	//backendData.gameThreadJniEnv->ReleaseStringChars(javaString, tempString.data());
}

void Application::HideSoftInput()
{
	auto const& backendData = *detail::pBackendData;

	backendData.gameThreadJniEnv->CallVoidMethod(backendData.activity->clazz, backendData.jniHideSoftInput);
}

void Application::LockCursor(bool state)
{

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

Application::FileInputStream::FileInputStream()
{
	static_assert(sizeof(detail::Backend_FileInputStreamData) <= sizeof(FileInputStream::m_buffer));

	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}

Application::FileInputStream::FileInputStream(char const* path)
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	Open(path);
}

Application::FileInputStream::FileInputStream(FileInputStream&& other) noexcept
{
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(detail::Backend_FileInputStreamData));
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));
}

Application::FileInputStream::~FileInputStream()
{
	Close();
}

Application::FileInputStream& DEngine::Application::FileInputStream::operator=(FileInputStream&& other) noexcept
{
	if (this == &other)
		return *this;

	Close();

	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(detail::Backend_FileInputStreamData));
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));

	return *this;
}


bool Application::FileInputStream::Seek(i64 offset, SeekOrigin origin)
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

bool Application::FileInputStream::Read(char* output, u64 size)
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

Std::Opt<u64> Application::FileInputStream::Tell() const
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file == nullptr)
		return {};

	return Std::Opt{ (u64)implData.pos };
}

bool Application::FileInputStream::IsOpen() const
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	return implData.file != nullptr;
}

bool Application::FileInputStream::Open(char const* path)
{
	Close();

	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	auto const& backendData = *detail::pBackendData;
	implData.file = AAssetManager_open(backendData.activity->assetManager, path, AASSET_MODE_RANDOM);
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
	return implData.file != nullptr;
}

void Application::FileInputStream::Close()
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file != nullptr)
		AAsset_close(implData.file);

	implData = detail::Backend_FileInputStreamData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}