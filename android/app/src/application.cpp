#include <DEngine/detail/Application.hpp>
#include <DEngine/detail/AppAssert.hpp>

#include <DEngine/Std/Containers/Variant.hpp>
#include <DEngine/Std/Utility.hpp>

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
			NativeWindowCreated,
			NativeWindowDestroyed,
			InputQueueCreated,
			VisibleAreaChanged,
			NewOrientation,
		};

		template<Type>
		struct Data {};
		template<>
		struct Data<Type::CharInput>
		{
			u32 charInput;
		};
		template<>
		struct Data<Type::NativeWindowCreated>
		{
			ANativeWindow* nativeWindow;
		};
		template<>
		struct Data<Type::InputQueueCreated>
		{
			AInputQueue* inputQueue;
		};
		template<>
		struct Data<Type::VisibleAreaChanged>
		{
			i32 offsetX;
			i32 offsetY;
			u32 width;
			u32 height;
		};
		template<>
		struct Data<Type::NewOrientation>
		{
			uint8_t newOrientation;
		};

		using VariantType = Std::Variant<
			Data<Type::CharInput>,
			Data<Type::CharRemove>,
			Data<Type::CharEnter>,
			Data<Type::NativeWindowCreated>,
			Data<Type::NativeWindowDestroyed>,
			Data<Type::InputQueueCreated>,
			Data<Type::VisibleAreaChanged>,
			Data<Type::NewOrientation>>;
		VariantType data = Data<Type::CharInput>{}; // Just set a default value
	};

	struct BackendData
	{
		ANativeActivity* activity = nullptr;
		AndroidPollSource inputPollSource{};

		std::thread gameThread;

		Std::Opt<WindowID> currentWindow;
		ANativeWindow* nativeWindow = nullptr;
		AInputQueue* inputQueue = nullptr;
		Math::Vec2Int visibleAreaOffset;
		Extent visibleAreaExtent;
		Orientation currentOrientation;

		jobject mainActivity = nullptr;
		// JNI Envs are per-thread. This is for the game thread, it is created when attaching
		// the thread to the JavaVM.
		JNIEnv* gameThreadJniEnv = nullptr;
		jmethodID jniOpenSoftInput = nullptr;
		jmethodID jniHideSoftInput = nullptr;
		std::mutex customEventQueueLock;
		std::vector<CustomEvent> customEventQueue;
	};

	BackendData* pBackendData = nullptr;

	[[nodiscard]] static bool HandleInputEvents_Motion(
			AInputEvent* event,
			int32_t source);
}

using namespace DEngine;

namespace DEngine::Application::detail
{
	[[nodiscard]] static Orientation ToOrientation(uint8_t aconfigOrientation)
	{
		switch (aconfigOrientation)
		{
			case ACONFIGURATION_ORIENTATION_LAND:
				return Orientation::Landscape;
			case ACONFIGURATION_ORIENTATION_PORT:
				return Orientation::Portrait;
			default:
				break;
		}
		return Orientation::Invalid;
	}

	static void onStart(ANativeActivity* activity)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onPause(ANativeActivity* activity)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onResume(ANativeActivity* activity)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onStop(ANativeActivity* activity)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onDestroy(ANativeActivity* activity)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
		auto& backendData = *detail::pBackendData;

		CustomEvent newEvent = {};
		CustomEvent::Data<CustomEvent::Type::NativeWindowCreated> data = {};
		data.nativeWindow = window;
		newEvent.data = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(newEvent);
	}

	static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
		auto& backendData = *detail::pBackendData;

		CustomEvent newEvent = {};
		CustomEvent::Data<CustomEvent::Type::NativeWindowDestroyed> data = {};
		newEvent.data = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(newEvent);
	}

	static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
		auto& backendData = *detail::pBackendData;

		CustomEvent newEvent = {};
		CustomEvent::Data<CustomEvent::Type::InputQueueCreated> data = {};
		data.inputQueue = queue;
		newEvent.data = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(newEvent);
	}

	static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onConfigurationChanged(ANativeActivity* activity)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(detail::pBackendData);

		auto& backendData = *detail::pBackendData;

		CustomEvent newEvent = {};
		CustomEvent::Data<CustomEvent::Type::NewOrientation> data = {};
		newEvent.data = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(newEvent);
	}
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL ANativeActivity_onCreate(
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
	activity->callbacks->onPause = &Application::detail::onPause;
	activity->callbacks->onResume = &Application::detail::onResume;
	activity->callbacks->onStop = &Application::detail::onStop;
	activity->callbacks->onDestroy = &Application::detail::onDestroy;
	activity->callbacks->onNativeWindowCreated = &Application::detail::onNativeWindowCreated;
	activity->callbacks->onNativeWindowDestroyed = &Application::detail::onNativeWindowDestroyed;
	activity->callbacks->onInputQueueCreated = &Application::detail::onInputQueueCreated;
	activity->callbacks->onInputQueueDestroyed = &Application::detail::onInputQueueDestroyed;
	activity->callbacks->onConfigurationChanged = &Application::detail::onConfigurationChanged;
}

// Called at the end of onCreate in Java.
extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeInit(
	JNIEnv* env,
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	DENGINE_DETAIL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	backendData.mainActivity = dengineActivity;

	backendData.inputPollSource.backendData = &backendData;

	backendData.customEventQueue.reserve(25);

	// Load current orientation
	AConfiguration* tempConfig = AConfiguration_new();
	AConfiguration_fromAssetManager(tempConfig, backendData.activity->assetManager);
	int32_t orientation = AConfiguration_getOrientation(tempConfig);
	backendData.currentOrientation = ToOrientation(orientation);
	AConfiguration_delete(tempConfig);


	auto lambda = []()
	{
		DENGINE_APP_MAIN_ENTRYPOINT(0, nullptr);
	};

	backendData.gameThread = std::thread(lambda);
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnCharInput(
	JNIEnv* env,
	jobject dengineActivity,
	jint utfValue)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	auto& backendData = *Application::detail::pBackendData;

	CustomEvent newEvent = {};
	CustomEvent::Data<CustomEvent::Type::CharInput> data = {};
	data.charInput = utfValue;
	newEvent.data = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(newEvent);
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnCharEnter(
	JNIEnv* env,
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	DENGINE_DETAIL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	CustomEvent newEvent = {};
	CustomEvent::Data<CustomEvent::Type::CharEnter> data = {};
	newEvent.data = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(newEvent);
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnCharRemove(
	JNIEnv* env,
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	DENGINE_DETAIL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	CustomEvent newEvent = {};
	CustomEvent::Data<CustomEvent::Type::CharRemove> data = {};
	newEvent.data = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(newEvent);
}

// We do not use ANativeActivity's View in this implementation,
// due to not being able to access it Java side and therefore cannot
// activate the soft input on it. That means ANativeActivity's callback for
// onContentRectChanged doesn't actually work.
// We need our own custom event for the View that we actually use.
extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnContentRectChanged(
	JNIEnv* env,
	jobject dengineActivity,
	jint posX,
	jint posY,
	jint width,
	jint height)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	DENGINE_DETAIL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	CustomEvent newEvent = {};
	CustomEvent::Data<CustomEvent::Type::VisibleAreaChanged> data = {};
	data.offsetX = (i32)posX;
	data.offsetY = (i32)posY;
	data.width = (u32)width;
	data.height = (u32)height;
	newEvent.data = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(newEvent);
}

/*
 * A bug in the NDK makes AConfiguration_getOrientation never have the updated value,
 * so we have this workaround path.
 *
 * Possibly no configuration changes are recorded from this bug, didn't really test enough.
 * Could possibly be that AConfiguration_fromAssetManager doesn't work in general.
 */
extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnNewOrientation(
	JNIEnv* env,
	jobject thiz,
	jint newOrientation)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	DENGINE_DETAIL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	CustomEvent newEvent = {};
	CustomEvent::Data<CustomEvent::Type::NewOrientation> data = {};
	data.newOrientation = (uint8_t)newOrientation;
	newEvent.data = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(newEvent);
}

static bool Application::detail::HandleInputEvents_Motion(AInputEvent* event, int32_t source)
{
	bool handled = false;

	if (source == AINPUT_SOURCE_MOUSE)
	{
		auto action = decltype(AMOTION_EVENT_ACTION_DOWN)(AMotionEvent_getAction(event));
		int32_t index = int32_t((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
		if (action == AMOTION_EVENT_ACTION_HOVER_ENTER)
		{
			// The cursor started existing
			f32 x = AMotionEvent_getX(event, index);
			f32 y = AMotionEvent_getY(event, index);

			pAppData->cursorOpt = CursorData{};
			pAppData->cursorOpt.Value().position = { (int32_t)x, (int32_t)y };
			pAppData->cursorOpt.Value().positionDelta = {};
			pAppData->cursorOpt.Value().scrollDeltaY = 0;
		}
		else if (action == AMOTION_EVENT_ACTION_HOVER_MOVE || action == AMOTION_EVENT_ACTION_MOVE)
		{
			f32 x = AMotionEvent_getX(event, index);
			f32 y = AMotionEvent_getY(event, index);
			AppData::WindowNode* windowNode = detail::GetWindowNode(detail::pBackendData->currentWindow.Value());
			DENGINE_DETAIL_APPLICATION_ASSERT(windowNode);
			detail::UpdateCursor(*windowNode, { (int32_t)x, (int32_t)y });
			handled = true;
			return handled;
		}
		else if (action == AMOTION_EVENT_ACTION_DOWN)
		{
			AppData::WindowNode* windowNode = detail::GetWindowNode(detail::pBackendData->currentWindow.Value());
			DENGINE_DETAIL_APPLICATION_ASSERT(windowNode);
			detail::UpdateButton(Button::LeftMouse, true);
			handled = true;
			return handled;
		}
		else if (action == AMOTION_EVENT_ACTION_UP)
		{
			AppData::WindowNode* windowNode = detail::GetWindowNode(detail::pBackendData->currentWindow.Value());
			DENGINE_DETAIL_APPLICATION_ASSERT(windowNode);
			detail::UpdateButton(Button::LeftMouse, false);
			handled = true;
			return handled;
		}

	}

	auto pointer = decltype(AMOTION_EVENT_ACTION_DOWN)(AMotionEvent_getAction(event));

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

namespace DEngine::Application::detail
{
	static void HandleEvent_NativeWindowCreated(
		ANativeWindow* window)
	{
		auto& backendData = *detail::pBackendData;
		auto& appData = *detail::pAppData;

		backendData.nativeWindow = window;
		// Need to maximize the window
		if (backendData.currentWindow.HasValue())
		{
			AppData::WindowNode* windowNode = detail::GetWindowNode(backendData.currentWindow.Value());
			DENGINE_DETAIL_APPLICATION_ASSERT(windowNode);
			detail::UpdateWindowMinimized(*windowNode, false);
			windowNode->platformHandle = backendData.nativeWindow;
		}
	}

	static void HandleEvent_NativeWindowDestroyed()
	{
		auto& backendData = *detail::pBackendData;
		auto& appData = *detail::pAppData;

		// We need to minimize the window
		if (backendData.currentWindow.HasValue())
		{
			AppData::WindowNode* windowNode = detail::GetWindowNode(backendData.currentWindow.Value());
			DENGINE_DETAIL_APPLICATION_ASSERT(windowNode);
			detail::UpdateWindowMinimized(*windowNode, true);
			windowNode->platformHandle = nullptr;
		}

		backendData.nativeWindow = nullptr;
	}

	static void HandleEvent_InputQueueCreated(AInputQueue* queue)
	{
		auto& backendData = *detail::pBackendData;
		auto& appData = *detail::pAppData;

		backendData.inputQueue = queue;
	}

	static void HandleEvent_VisibleAreaChanged(
		Math::Vec2Int offset,
		Extent extent)
	{
		auto& backendData = *detail::pBackendData;
		backendData.visibleAreaOffset = offset;
		backendData.visibleAreaExtent = extent;

		if (backendData.currentWindow.HasValue())
		{
			auto windowNodePtr = detail::GetWindowNode(backendData.currentWindow.Value());

			auto windowWidth = (uint32_t)ANativeWindow_getWidth(backendData.nativeWindow);
			auto windowHeight = (uint32_t)ANativeWindow_getHeight(backendData.nativeWindow);

			detail::UpdateWindowSize(
				*windowNodePtr,
				{ windowWidth, windowHeight },
				offset,
				extent);
		}

	}

	static void HandleEvent_Reorientation(uint8_t newOrientation)
	{
		DENGINE_DETAIL_APPLICATION_ASSERT(pBackendData);
		auto& backendData = *pBackendData;



	}

	using ProcessCustomEvents_CallableT = void(*)(CustomEvent);
	template<typename Callable = ProcessCustomEvents_CallableT>
	static void ProcessCustomEvents_Internal(
		Std::Opt<Callable> callable)
	{
		auto& backendData = *detail::pBackendData;
		std::lock_guard queueLock{ backendData.customEventQueueLock };
		for (auto const& event : backendData.customEventQueue)
		{
			if (callable.HasValue())
				callable.Value()(event);
			auto typeIndex = event.data.GetIndex();
			switch (typeIndex)
			{
				case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::NativeWindowCreated>>:
				{
					auto const& data = event.data.Get<CustomEvent::Data<CustomEvent::Type::NativeWindowCreated>>();
					HandleEvent_NativeWindowCreated(data.nativeWindow);
					break;
				}

				case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::NativeWindowDestroyed>>:
				{
					HandleEvent_NativeWindowDestroyed();
					break;
				}

				case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::InputQueueCreated>>:
				{
					auto const& data = event.data.Get<CustomEvent::Data<CustomEvent::Type::InputQueueCreated>>();
					HandleEvent_InputQueueCreated(data.inputQueue);
					break;
				}

				case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::VisibleAreaChanged>>:
				{
					auto const& data = event.data.Get<CustomEvent::Data<CustomEvent::Type::VisibleAreaChanged>>();
					HandleEvent_VisibleAreaChanged(
						{ data.offsetX, data.offsetY },
						{ data.width, data.height });
					break;
				}

				case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::CharInput>>:
				{
					auto const& data = event.data.Get<CustomEvent::Data<CustomEvent::Type::CharInput>>();
					detail::PushCharInput(data.charInput);
					break;
				}

				case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::CharRemove>>:
				{
					detail::PushCharRemoveEvent();
					break;
				}

				case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::CharEnter>>:
				{
					detail::PushCharEnterEvent();
					break;
				}

				case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::NewOrientation>>:
				{
					auto const& data = event.data.Get<CustomEvent::Data<CustomEvent::Type::NewOrientation>>();
					HandleEvent_Reorientation(data.newOrientation);
					break;
				}

				default:
					DENGINE_IMPL_UNREACHABLE();
					break;
			}
		}
		backendData.customEventQueue.clear();
	}

	template<typename Callable>
	static void ProcessCustomEvents(Callable callable)
	{
		ProcessCustomEvents_Internal(Std::Opt{ callable });
	}

	static void ProcessCustomEvents()
	{
		ProcessCustomEvents_Internal<ProcessCustomEvents_CallableT>(Std::nullOpt);
	}
}

bool Application::detail::Backend_Initialize() noexcept
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

	auto threadName = "MainGameThread";
	int result = pthread_setname_np(backendData.gameThread.native_handle(), threadName);
	if (result != 0)
	{
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
		"(Ljava/lang/String;I)V");
	backendData.jniHideSoftInput = backendData.gameThreadJniEnv->GetMethodID(
		activity,
		"hideSoftInput",
		"()V");

	// We want to wait until we have received a input queue, a native window and set
	// the visible area stuff.
	bool nativeWindowSet = false;
	bool inputQueueSet = false;
	bool visibleAreaSet = false;
	while (!nativeWindowSet || !inputQueueSet || !visibleAreaSet)
	{
		ProcessCustomEvents(
			[&nativeWindowSet, &inputQueueSet, &visibleAreaSet](CustomEvent const& event)
			{
				auto typeIndex = event.data.GetIndex();
				switch (typeIndex)
				{
					case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::InputQueueCreated>>:
						inputQueueSet = true;
						break;
					case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::NativeWindowCreated>>:
						nativeWindowSet = true;
						break;
					case decltype(event.data)::indexOf<CustomEvent::Data<CustomEvent::Type::VisibleAreaChanged>>:
						visibleAreaSet = true;
						break;
					default:
						break;
				}
			});
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
	// The following if-test might be pointless in this implementation,
	// due to it being callback based and so this return value is meaningless.
	if (pollResult >= 0)
	{
		auto looperId = (LooperID)fileDescriptor; // This is wrong.
		if (source != nullptr)
		{

		}
	}

	// Process our custom events
	ProcessCustomEvents();
}

Application::WindowID Application::CreateWindow(
		char const* title,
		Extent extents)
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;

	if (backendData.currentWindow.HasValue())
		std::abort();
	else
		backendData.currentWindow = Std::Opt{ (App::WindowID)appData.windowIdTracker };

	detail::AppData::WindowNode newNode{};
	newNode.id = (App::WindowID)appData.windowIdTracker;
	appData.windowIdTracker++;

	int width = ANativeWindow_getWidth(backendData.nativeWindow);
	int height = ANativeWindow_getHeight(backendData.nativeWindow);
	newNode.windowData.size.width = (u32)width;
	newNode.windowData.size.height = (u32)height;
	newNode.windowData.visiblePosition = backendData.visibleAreaOffset;
	newNode.windowData.visibleSize = backendData.visibleAreaExtent;
	newNode.platformHandle = backendData.nativeWindow;

	appData.windows.push_back(newNode);

	return newNode.id;
}

void Application::detail::Backend_DestroyWindow(AppData::WindowNode& windowNode)
{
	auto& backendData = *detail::pBackendData;
	backendData.currentWindow = Std::nullOpt;
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

Std::StackVec<char const*, 5> Application::RequiredVulkanInstanceExtensions() noexcept
{
	Std::StackVec<char const*, 5> returnVal{};
	returnVal.PushBack("VK_KHR_surface");
	returnVal.PushBack("VK_KHR_android_surface");
	return returnVal;
}

void Application::SetCursor(WindowID id, CursorType cursorType) noexcept
{

}

void Application::detail::Backend_Log(char const* msg)
{
	__android_log_print(ANDROID_LOG_ERROR, "DEngine: ", "%s", msg);
}

void Application::OpenSoftInput(Std::Str inputString, SoftInputFilter inputFilter)
{
	auto const& backendData = *detail::pBackendData;
	std::basic_string<jchar> tempString;
	tempString.reserve(inputString.Size());
	for (uSize i = 0; i < inputString.Size(); i++)
	{
		tempString.push_back((jchar)inputString[i]);
	}

	jstring javaString = backendData.gameThreadJniEnv->NewString(tempString.data(), tempString.length());

	backendData.gameThreadJniEnv->CallVoidMethod(
		backendData.activity->clazz,
		backendData.jniOpenSoftInput,
		javaString,
		(jint)inputFilter);

	//backendData.gameThreadJniEnv->ReleaseStringChars(javaString, tempString.data());
}

void Application::UpdateCharInputContext(Std::Str inputString)
{

}

void Application::HideSoftInput()
{
	auto const& backendData = *detail::pBackendData;

	backendData.gameThreadJniEnv->CallVoidMethod(backendData.activity->clazz, backendData.jniHideSoftInput);
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