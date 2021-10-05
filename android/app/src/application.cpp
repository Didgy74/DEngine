#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

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

		AppData* appData = nullptr;
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
		Type type;

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

		union Data_T
		{
			Data<Type::CharInput> charInput;
			Data<Type::CharRemove> charRemove;
			Data<Type::CharEnter> charEnter;
			Data<Type::NativeWindowCreated> nativeWindowCreated;
			Data<Type::NativeWindowDestroyed> nativeWindowDestroyed;
			Data<Type::InputQueueCreated> inputQueueCreated;
			Data<Type::VisibleAreaChanged> visibleAreaChanged;
			Data<Type::NewOrientation> newOrientation;
		};
		Data_T data;
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

	[[nodiscard]] static GamepadKey ToGamepadButton(int32_t androidKeyCode) noexcept
	{
		switch (androidKeyCode)
		{
			case AKEYCODE_BUTTON_A:
				return GamepadKey::A;
		}
		return GamepadKey::Invalid;
	}

	static void onStart(ANativeActivity* activity)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onPause(ANativeActivity* activity)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onResume(ANativeActivity* activity)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onStop(ANativeActivity* activity)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onDestroy(ANativeActivity* activity)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
		auto& backendData = *detail::pBackendData;

		CustomEvent event = {};
		event.type = CustomEvent::Type::NativeWindowCreated;
		CustomEvent::Data<CustomEvent::Type::NativeWindowCreated> data = {};
		data.nativeWindow = window;
		event.data.nativeWindowCreated = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);
	}

	static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
		auto& backendData = *detail::pBackendData;

		CustomEvent event = {};
		event.type = CustomEvent::Type::NativeWindowDestroyed;
		CustomEvent::Data<CustomEvent::Type::NativeWindowDestroyed> data = {};
		event.data.nativeWindowDestroyed = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);
	}

	static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
		auto& backendData = *detail::pBackendData;

		CustomEvent event = {};
		event.type = CustomEvent::Type::InputQueueCreated;
		CustomEvent::Data<CustomEvent::Type::InputQueueCreated> data = {};
		data.inputQueue = queue;
		event.data.inputQueueCreated = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);
	}

	static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onConfigurationChanged(ANativeActivity* activity)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);

		auto& backendData = *detail::pBackendData;

		CustomEvent event = {};
		event.type = CustomEvent::Type::NewOrientation;
		CustomEvent::Data<CustomEvent::Type::NewOrientation> data = {};
		event.data.newOrientation = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);
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

	DENGINE_IMPL_APPLICATION_ASSERT(Application::detail::pBackendData);
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

	CustomEvent event = {};
	event.type = CustomEvent::Type::CharInput;
	CustomEvent::Data<CustomEvent::Type::CharInput> data = {};
	data.charInput = utfValue;
	event.data.charInput = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(event);
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnCharEnter(
	JNIEnv* env,
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	DENGINE_IMPL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	CustomEvent event = {};
	event.type = CustomEvent::Type::CharEnter;
	CustomEvent::Data<CustomEvent::Type::CharEnter> data = {};
	event.data.charEnter = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(event);
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnCharRemove(
	JNIEnv* env,
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	DENGINE_IMPL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	CustomEvent event = {};
	event.type = CustomEvent::Type::CharRemove;
	CustomEvent::Data<CustomEvent::Type::CharRemove> data = {};
	event.data.charRemove = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(event);
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

	DENGINE_IMPL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	CustomEvent event = {};
	event.type = CustomEvent::Type::VisibleAreaChanged;
	CustomEvent::Data<CustomEvent::Type::VisibleAreaChanged> data = {};
	data.offsetX = (i32)posX;
	data.offsetY = (i32)posY;
	data.width = (u32)width;
	data.height = (u32)height;
	event.data.visibleAreaChanged = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(event);
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

	DENGINE_IMPL_APPLICATION_ASSERT(Application::detail::pBackendData);
	auto& backendData = *Application::detail::pBackendData;

	CustomEvent event = {};
	event.type = CustomEvent::Type::NewOrientation;
	CustomEvent::Data<CustomEvent::Type::NewOrientation> data = {};
	data.newOrientation = (uint8_t)newOrientation;
	event.data.newOrientation = data;

	std::lock_guard _{ backendData.customEventQueueLock };
	backendData.customEventQueue.push_back(event);
}

namespace DEngine::Application::detail
{
	// This function is called from the AInputQueue function when we poll the ALooper
	static bool HandleInputEvents_Key(
		AppData& appData,
		BackendData& backendData,
		AInputEvent* event,
		int32_t sourceFlags)
	{
		bool handled = false;

		auto const keyCode = AKeyEvent_getKeyCode(event);

		auto const gamepadButton = ToGamepadButton(keyCode);

		if (gamepadButton != GamepadKey::Invalid)
		{
			handled = true;

			auto const action = AKeyEvent_getAction(event);

			if (action != AKEY_EVENT_ACTION_MULTIPLE)
			{
				auto const pressed = action == AKEY_EVENT_ACTION_DOWN;

				detail::UpdateGamepadButton(
					appData,
					gamepadButton,
					pressed);
			}
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion_Cursor(
		AppData& appData,
		BackendData& backendData,
		AInputEvent* event,
		i32 sourceFlags,
		i32 action,
		i32 index)
	{
		if ((sourceFlags & AINPUT_SOURCE_MOUSE) != AINPUT_SOURCE_MOUSE)
			return false;

		auto handled = false;

		if (action == AMOTION_EVENT_ACTION_HOVER_ENTER)
		{
			// The cursor started existing
			f32 const x = AMotionEvent_getX(event, index);
			f32 const y = AMotionEvent_getY(event, index);

			appData.cursorOpt = CursorData{};
			auto& cursorData = appData.cursorOpt.Value();
			cursorData.position = { (i32)x, (i32)y };

			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_HOVER_MOVE || action == AMOTION_EVENT_ACTION_MOVE)
		{
			f32 const x = AMotionEvent_getX(event, index);
			f32 const y = AMotionEvent_getY(event, index);
			detail::UpdateCursor(
				appData,
				backendData.currentWindow.Value(),
				{ (i32)x, (i32)y });
			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_DOWN)
		{
			detail::UpdateButton(appData, Button::LeftMouse, true);
			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_UP)
		{
			detail::UpdateButton(appData, Button::LeftMouse, false);
			handled = true;
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion_Touch(
		AppData& appData,
		BackendData& backendData,
		AInputEvent* event,
		i32 sourceFlags,
		i32 action,
		i32 index)
	{
		if ((sourceFlags & AINPUT_SOURCE_TOUCHSCREEN) == 0)
			return false;

		auto handled = false;

		switch (action)
		{
			case AMOTION_EVENT_ACTION_DOWN:
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
			{
				auto const x = AMotionEvent_getX(event, index);
				auto const y = AMotionEvent_getY(event, index);
				auto const id = AMotionEvent_getPointerId(event, index);
				detail::UpdateTouchInput(appData, TouchEventType::Down, (u8)id, x, y);
				handled = true;
				break;
			}

			case AMOTION_EVENT_ACTION_MOVE:
			{
				auto const count = AMotionEvent_getPointerCount(event);
				for (auto i = 0; i < count; i++)
				{
					auto const x = AMotionEvent_getX(event, i);
					auto const y = AMotionEvent_getY(event, i);
					auto const  id = AMotionEvent_getPointerId(event, i);
					detail::UpdateTouchInput(appData, TouchEventType::Moved, (u8)id, x, y);
				}
				handled = true;
				break;
			}

			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_POINTER_UP:
			{
				auto const  x = AMotionEvent_getX(event, index);
				auto const  y = AMotionEvent_getY(event, index);
				auto const  id = AMotionEvent_getPointerId(event, index);
				detail::UpdateTouchInput(appData, TouchEventType::Up, (u8)id, x, y);
				handled = true;
				break;
			}

			default:
				break;
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion_Joystick(
		AppData& appData,
		BackendData& backendData,
		AInputEvent* event,
		i32 sourceFlags,
		i32 action,
		i32 index)
	{
		if ((sourceFlags & AINPUT_SOURCE_JOYSTICK) == 0)
			return false;

		auto handled = false;

		if (action == AMOTION_EVENT_ACTION_MOVE)
		{
			auto const leftStickX = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);

			detail::UpdateGamepadAxis(
				appData,
				GamepadAxis::LeftX,
				leftStickX);

			handled = true;
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion(
		AppData& appData,
		BackendData& backendData,
		AInputEvent* event,
		int32_t sourceFlags)
	{
		bool handled;

		auto const actionIndexCombo = AMotionEvent_getAction(event);
		auto const action = actionIndexCombo & AMOTION_EVENT_ACTION_MASK;
		auto const index =
			(actionIndexCombo & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
			AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

		handled = HandleInputEvents_Motion_Cursor(
			appData,
			backendData,
			event,
			sourceFlags,
			action,
			index);
		if (handled)
			return true;

		handled = HandleInputEvents_Motion_Touch(
			appData,
			backendData,
			event,
			sourceFlags,
			action,
			index);
		if (handled)
			return true;

		handled = HandleInputEvents_Motion_Joystick(
			appData,
			backendData,
			event,
			sourceFlags,
			action,
			index);
		if (handled)
			return true;

		return handled;
	}
}

namespace DEngine::Application::detail
{
	// Callback for AInputQueue_attachLooper.
	// This needs to return 1 if we want to keep receiving
	// callbacks, or 0 if we want to unregister this callback from the looper.
	int testCallback(int fd, int events, void *data)
	{
		auto& androidPollSource = *static_cast<AndroidPollSource*>(data);
		auto& appData = *androidPollSource.appData;
		auto& backendData = *androidPollSource.backendData;

		if ((unsigned int)events & ALOOPER_EVENT_INPUT)
		{
			while (true)
			{
				AInputEvent* event = nullptr;
				auto const eventIndex = AInputQueue_getEvent(backendData.inputQueue, &event);
				if (eventIndex < 0)
					break;

				if (AInputQueue_preDispatchEvent(backendData.inputQueue, event) != 0)
					continue;

				bool handled = false;

				auto const eventType = AInputEvent_getType(event);
				auto const eventSourceFlags = AInputEvent_getSource(event);

				if (!handled && eventType == AINPUT_EVENT_TYPE_MOTION)
				{
					handled = HandleInputEvents_Motion(appData, backendData, event, eventSourceFlags);
				}
				if (!handled && eventType == AINPUT_EVENT_TYPE_KEY)
				{
					handled = HandleInputEvents_Key(appData, backendData, event, eventSourceFlags);
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
			AppData::WindowNode* windowNode = detail::GetWindowNode(appData, backendData.currentWindow.Value());
			DENGINE_IMPL_APPLICATION_ASSERT(windowNode);
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
			AppData::WindowNode* windowNode = detail::GetWindowNode(appData, backendData.currentWindow.Value());
			DENGINE_IMPL_APPLICATION_ASSERT(windowNode);
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
		auto& appData = *detail::pAppData;

		auto& backendData = *detail::pBackendData;
		backendData.visibleAreaOffset = offset;
		backendData.visibleAreaExtent = extent;

		if (backendData.currentWindow.HasValue())
		{
			auto windowNodePtr = detail::GetWindowNode(appData, backendData.currentWindow.Value());

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
		DENGINE_IMPL_APPLICATION_ASSERT(pBackendData);
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
			auto typeIndex = event.type;
			switch (typeIndex)
			{
				case CustomEvent::Type::NativeWindowCreated:
				{
					auto const& data = event.data.nativeWindowCreated;
					HandleEvent_NativeWindowCreated(data.nativeWindow);
					break;
				}

				case CustomEvent::Type::NativeWindowDestroyed:
				{
					HandleEvent_NativeWindowDestroyed();
					break;
				}

				case CustomEvent::Type::InputQueueCreated:
				{
					auto const& data = event.data.inputQueueCreated;
					HandleEvent_InputQueueCreated(data.inputQueue);
					break;
				}

				case CustomEvent::Type::VisibleAreaChanged:
				{
					auto const& data = event.data.visibleAreaChanged;
					HandleEvent_VisibleAreaChanged(
						{ data.offsetX, data.offsetY },
						{ data.width, data.height });
					break;
				}

				case CustomEvent::Type::CharInput:
				{
					auto const& data = event.data.charInput;
					detail::PushCharInput(data.charInput);
					break;
				}

				case CustomEvent::Type::CharRemove:
				{
					detail::PushCharRemoveEvent();
					break;
				}

				case CustomEvent::Type::CharEnter:
				{
					detail::PushCharEnterEvent();
					break;
				}

				case CustomEvent::Type::NewOrientation:
				{
					auto const& data = event.data.newOrientation;
					HandleEvent_Reorientation(data.newOrientation);
					break;
				}

				default:
					DENGINE_IMPL_APPLICATION_UNREACHABLE();
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

	backendData.inputPollSource.appData = &appData;

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
				auto typeIndex = event.type;
				switch (typeIndex)
				{
					case CustomEvent::Type::InputQueueCreated:
						inputQueueSet = true;
						break;
					case CustomEvent::Type::NativeWindowCreated:
						nativeWindowSet = true;
						break;
					case CustomEvent::Type::VisibleAreaChanged:
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
	newNode.windowData.extent.width = (u32)width;
	newNode.windowData.extent.height = (u32)height;
	newNode.windowData.visibleOffset = backendData.visibleAreaOffset;
	newNode.windowData.visibleExtent = backendData.visibleAreaExtent;
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