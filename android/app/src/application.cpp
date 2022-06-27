#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

#include "BackendData.hpp"
#include <DEngine/Std/Utility.hpp>

#include <android/configuration.h>
#include <android/log.h>

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>

#include <dlfcn.h>
#include <sys/eventfd.h>

extern int dengine_app_detail_main(int argc, char** argv);

using namespace DEngine;
using namespace DEngine::Application;

Application::impl::BackendData* Application::impl::pBackendData = nullptr;

namespace DEngine::Application::impl
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

	static void PushCustomEvent_WindowCreated(BackendData& backendData, ANativeWindow* window)
	{
		CustomEvent event = {};
		event.type = CustomEvent::Type::NativeWindowCreated;
		CustomEvent::Data<CustomEvent::Type::NativeWindowCreated> data = {};
		data.nativeWindow = window;
		event.data.nativeWindowCreated = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);

		eventfd_t fdIncrement = 1;
		eventfd_write(backendData.customEventFd, fdIncrement);
	}
	static void PushCustomEvent_WindowDestroyed(BackendData& backendData, ANativeWindow* window)
	{
		CustomEvent event = {};
		event.type = CustomEvent::Type::NativeWindowDestroyed;
		CustomEvent::Data<CustomEvent::Type::NativeWindowDestroyed> data = {};
		event.data.nativeWindowDestroyed = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);
	}
	static void PushCustomEvent_InputQueueCreated(BackendData& backendData, AInputQueue* queue)
	{
		CustomEvent event = {};
		event.type = CustomEvent::Type::InputQueueCreated;
		CustomEvent::Data<CustomEvent::Type::InputQueueCreated> data = {};
		data.inputQueue = queue;
		event.data.inputQueueCreated = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);

		eventfd_t fdIncrement = 1;
		eventfd_write(backendData.customEventFd, fdIncrement);
	}
	static void PushCustomEvent_ConfigurationChanged(BackendData& backendData)
	{
		/*
		CustomEvent event = {};
		event.type = CustomEvent::Type::NewOrientation;
		CustomEvent::Data<CustomEvent::Type::NewOrientation> data = {};
		event.data.newOrientation = data;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);
		*/
	}
	void PushCustomEvent_ContentRectChanged(
		BackendData& backendData,
		int posX,
		int posY,
		int width,
		int height)
	{
		CustomEvent event = {};
		event.type = CustomEvent::Type::VisibleAreaChanged;
		event.data.visibleAreaChanged = {};
		auto& data = event.data.visibleAreaChanged;
		data.offsetX = (i32)posX;
		data.offsetY = (i32)posY;
		data.width = (u32)width;
		data.height = (u32)height;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);
	}

	void PushCustomEvent_TextInput(
		BackendData& backendData,
		int start,
		int count,
		Std::Span<u32 const> text)
	{
		std::lock_guard _{ backendData.customEventQueueLock };

		auto& textInputs = backendData.customEvent_textInputs;
		auto oldLen = textInputs.size();
		auto newLen = oldLen + text.Size();
		textInputs.resize(newLen);
		for (int i = oldLen; i < newLen; i++)
			textInputs[i] = text[i - oldLen];

		CustomEvent event = {};
		event.type = CustomEvent::Type::TextInput;
		event.data.textInput = {};
		auto& data = event.data.textInput;
		data.start = (uSize)start;
		data.count = (uSize)count;
		data.textInputStartIndex = oldLen;
		data.textInputLength = oldLen - newLen;

		backendData.customEventQueue.push_back(event);
	}

	void PushCustomEvent_EndTextInputSession(
		BackendData& backendData)
	{
		CustomEvent event = {};
		event.type = CustomEvent::Type::EndTextInputSession;

		std::lock_guard _{ backendData.customEventQueueLock };
		backendData.customEventQueue.push_back(event);
	}

	static void LogEvent(char const* msg)
	{
		__android_log_print(ANDROID_LOG_DEBUG, "DEngine - Event:", "%s", msg);
	}

	static void onStart(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("Start");
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onPause(ANativeActivity* activity)
	{
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onResume(ANativeActivity* activity)
	{
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onStop(ANativeActivity* activity)
	{
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onDestroy(ANativeActivity* activity)
	{
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window)
	{
		if constexpr (logEvents)
			LogEvent("Native window created");
		auto& backendData = GetBackendData(activity);
		PushCustomEvent_WindowCreated(backendData, window);
	}
	static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window)
	{
		if constexpr (logEvents)
			LogEvent("Native window destroyed");
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
		//auto& backendData = *detail::pBackendData;
	}
	static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue)
	{
		if constexpr (logEvents)
			LogEvent("Input queue created");
		auto& backendData = GetBackendData(activity);
		PushCustomEvent_InputQueueCreated(backendData, queue);
	}

	static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue)
	{
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}
	static void onConfigurationChanged(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("Configuration changed");
		auto& backendData = GetBackendData(activity);
	}
	static void onContentRectChanged(ANativeActivity* activity, ARect const* rect)
	{
		if constexpr (logEvents)
			LogEvent("Content rect changed");

		auto& backendData = GetBackendData(activity);
		auto posX = rect->left;
		auto posY = rect->top;
		auto width = rect->right - rect->left;
		auto height = rect->bottom - rect->top;
		PushCustomEvent_ContentRectChanged(
			backendData,
			posX,
			posY,
			width,
			height);
	}

	// Called by the main Java thread during startup
	void OnCreate(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("Create");

		activity->callbacks->onStart = &onStart;
		activity->callbacks->onPause = &onPause;
		activity->callbacks->onResume = &onResume;
		activity->callbacks->onStop = &onStop;
		activity->callbacks->onDestroy = &onDestroy;
		activity->callbacks->onNativeWindowCreated = &onNativeWindowCreated;
		activity->callbacks->onNativeWindowDestroyed = &onNativeWindowDestroyed;
		activity->callbacks->onInputQueueCreated = &onInputQueueCreated;
		activity->callbacks->onInputQueueDestroyed = &onInputQueueDestroyed;
		activity->callbacks->onConfigurationChanged = &onConfigurationChanged;
		activity->callbacks->onContentRectChanged = &onContentRectChanged;

		// Create our backendData
		pBackendData = new BackendData();
		auto& backendData = *pBackendData;

		activity->instance = &backendData;

		backendData.nativeActivity = activity;
		backendData.globalJavaVm = activity->vm;

		JNIEnv* env = nullptr;
		auto getEnvResult = backendData.globalJavaVm->functions->GetEnv(
			backendData.globalJavaVm,
			(void**)&env,
			JNI_VERSION_1_6);
		DENGINE_IMPL_APPLICATION_ASSERT(getEnvResult == JNI_OK);
		backendData.mainActivity = env->NewGlobalRef(activity->clazz);

		backendData.assetManager = activity->assetManager;

		// And we create our file descriptor for our custom events
		backendData.customEventFd = eventfd(0, EFD_SEMAPHORE);

		auto lambda = []() {
			DENGINE_APP_MAIN_ENTRYPOINT(0, nullptr);
		};

		backendData.gameThread = std::thread(lambda);
	}
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL ANativeActivity_onCreate(
	ANativeActivity* activity,
	void* savedState,
	size_t savedStateSize)
{
	DEngine::Application::impl::OnCreate(activity);
}

// Called at the end of onCreate in Java.
extern "C"
[[maybe_unused]] JNIEXPORT double JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeInit(
	JNIEnv* env,
	jobject dengineActivity)


	{
	double returnValue = {};
	auto* backendPtr = DEngine::Application::impl::pBackendData;
	memcpy(&returnValue, &backendPtr, sizeof(backendPtr));

	return returnValue;
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeOnTextInput(
	JNIEnv* env,
	jobject dengineActivity,
	jdouble backendDataPtr,
	jint start,
	jint count,
	jstring text,
	jint textLen)
{
	auto& backendData = DEngine::Application::impl::GetBackendData(backendDataPtr);

	std::vector<DEngine::u32> temp;
	temp.resize(textLen);
	jchar const* jniText = env->GetStringChars(text, nullptr);

	for (int i = 0; i < textLen; i++)
		temp[i] = (DEngine::u32)jniText[i];

	DEngine::Application::impl::PushCustomEvent_TextInput(
		backendData,
		start,
		count,
		{ temp.data(), temp.size() });

	env->ReleaseStringChars(text, jniText);
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_editor_DEngineActivity_nativeSendEventEndTextInputSession(
	JNIEnv* env,
	jobject dengineActivity,
	jdouble backendDataPtr)
{
	DEngine::Application::impl::PushCustomEvent_EndTextInputSession(
		DEngine::Application::impl::GetBackendData(backendDataPtr));
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
	jdouble backendPtr,
	jint posX,
	jint posY,
	jint width,
	jint height)
{
	PushCustomEvent_ContentRectChanged(
		DEngine::Application::impl::GetBackendData(backendPtr),
		posX,
		posY,
		width,
		height);
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
}

namespace DEngine::Application::impl
{
	// This function is called from the AInputQueue function when we poll the ALooper
	static bool HandleInputEvents_Key(
		Context::Impl& implData,
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


				//detail::UpdateGamepadButton(
					//appData,
					//gamepadButton,
					//pressed);
			}
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion_Cursor(
		Context::Impl& implData,
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

			implData.cursorOpt = CursorData{};
			auto& cursorData = implData.cursorOpt.Value();
			cursorData.position = { (i32)x, (i32)y };

			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_HOVER_MOVE || action == AMOTION_EVENT_ACTION_MOVE)
		{
			f32 const x = AMotionEvent_getX(event, index);
			f32 const y = AMotionEvent_getY(event, index);
			BackendInterface::UpdateCursorPosition(
				implData,
				backendData.currentWindow.Value(),
				{ (i32)x, (i32)y });
			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_DOWN)
		{
			//detail::UpdateButton(appData, Button::LeftMouse, true);
			handled = true;
		}
		else if (action == AMOTION_EVENT_ACTION_UP)
		{
			//detail::UpdateButton(appData, Button::LeftMouse, false);
			handled = true;
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion_Touch(
		Context::Impl& implData,
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
				BackendInterface::UpdateTouch(
					implData,
					backendData.currentWindow.Get(),
					TouchEventType::Down,
					(u8)id,
					x,
					y);
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
					BackendInterface::UpdateTouch(
						implData,
						backendData.currentWindow.Get(),
						TouchEventType::Moved,
						(u8)id,
						x,
						y);
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
				BackendInterface::UpdateTouch(
					implData,
					backendData.currentWindow.Get(),
					TouchEventType::Up,
					(u8)id,
					x,
					y);
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
		Context::Impl& implData,
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

			/*
			detail::UpdateGamepadAxis(
				appData,
				GamepadAxis::LeftX,
				leftStickX);
			 */

			handled = true;
		}

		return handled;
	}

	// This function is called from the AInputQueue function when we poll the ALooper
	// Should return true if the event has been handled by the app.
	static bool HandleInputEvents_Motion(
		Context::Impl& appData,
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

namespace DEngine::Application::impl
{
	// Callback for AInputQueue_attachLooper.
	// This needs to return 1 if we want to keep receiving
	// callbacks, or 0 if we want to unregister this callback from the looper.
	int testCallback(int fd, int events, void *data)
	{
		auto& androidPollSource = *static_cast<PollSource*>(data);
		auto& implData = *androidPollSource.implData;
		auto& backendData = *androidPollSource.backendData;

		//if (events == (int)LooperID::Input)
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
					handled = HandleInputEvents_Motion(
						implData,
						backendData,
						event,
						eventSourceFlags);
				}
				/*
				if (!handled && eventType == AINPUT_EVENT_TYPE_KEY)
				{
					handled = HandleInputEvents_Key(
						implData,
						backendData,
						event,
						eventSourceFlags);
				}
				*/

				AInputQueue_finishEvent(backendData.inputQueue, event, handled);
			}
		}

		// 1 Means we want this callback to keep being invoked.
		return 1;
	}
}

namespace DEngine::Application::impl
{
	static void HandleEvent_NativeWindowCreated(
		Context::Impl& implData,
		BackendData& backendData,
		ANativeWindow* window)
	{
		backendData.nativeWindow = window;
	}

	static void HandleEvent_NativeWindowDestroyed(
		Context::Impl& implData)
	{
		auto& backendData = GetBackendData(implData);

		backendData.nativeWindow = nullptr;
	}

	static void HandleEvent_InputQueueCreated(
		BackendData& backendData,
		AInputQueue* queue)
	{
		backendData.inputQueue = queue;
	}

	static void HandleEvent_VisibleAreaChanged(
		Context::Impl& implData,
		BackendData& backendData,
		Math::Vec2UInt offset,
		Extent extent)
	{
		backendData.visibleAreaOffset = offset;
		backendData.visibleAreaExtent = extent;

		if (backendData.currentWindow.HasValue())
		{
			auto windowNodePtr = implData.GetWindowNode(backendData.currentWindow.Get());

			auto windowWidth = (uint32_t)ANativeWindow_getWidth(backendData.nativeWindow);
			auto windowHeight = (uint32_t)ANativeWindow_getHeight(backendData.nativeWindow);

			/*
			detail::UpdateWindowSize(
				*windowNodePtr,
				{ windowWidth, windowHeight },
				offset,
				extent);
			 */
		}
	}

	static void HandleEvent_Reorientation(uint8_t newOrientation)
	{

	}

	static void HandleCustomEvent_TextInput(
		Context::Impl& implData,
		BackendData& backendData,
		u32 start,
		u32 count,
		Std::Span<u32 const> replacementString)
	{
		impl::BackendInterface::PushTextInputEvent(
			implData,
			backendData.currentWindow.Get(),
			start,
			count,
			replacementString);
	}

	static void HandleCustomEvent_EndTextInputSession(
		Context::Impl& implData,
		BackendData& backendData)
	{
		impl::BackendInterface::PushEndTextInputSessionEvent(
			implData,
			backendData.currentWindow.Get());
	}

	using ProcessCustomEvents_CallableT = void(*)(CustomEvent);
	template<typename Callable = ProcessCustomEvents_CallableT>
	static void ProcessCustomEvents_Internal(
		Context::Impl& implData,
		BackendData& backendData,
		Std::Opt<Callable> callable)
	{
		std::lock_guard queueLock{ backendData.customEventQueueLock };
		for (auto const& event : backendData.customEventQueue)
		{
			if (callable.HasValue())
				callable.Value()(event);
			auto typeIndex = event.type;
			switch (typeIndex)
			{
				case CustomEvent::Type::NativeWindowCreated: {
					auto const& data = event.data.nativeWindowCreated;
					HandleEvent_NativeWindowCreated(
						implData,
						backendData,
						data.nativeWindow);
					break;
				}

				case CustomEvent::Type::NativeWindowDestroyed: {
					HandleEvent_NativeWindowDestroyed(implData);
					break;
				}

				case CustomEvent::Type::InputQueueCreated: {
					auto const& data = event.data.inputQueueCreated;
					HandleEvent_InputQueueCreated(backendData, data.inputQueue);
					break;
				}

				case CustomEvent::Type::VisibleAreaChanged: {
					auto const& data = event.data.visibleAreaChanged;
					HandleEvent_VisibleAreaChanged(
						implData,
						backendData,
						{ data.offsetX, data.offsetY },
						{ data.width, data.height });
					break;
				}

				case CustomEvent::Type::NewOrientation: {
					auto const& data = event.data.newOrientation;
					HandleEvent_Reorientation(data.newOrientation);
					break;
				}

				case CustomEvent::Type::TextInput: {
					auto const& data = event.data.textInput;
					Std::Span replacementText = {
						backendData.customEvent_textInputs.data(),
						backendData.customEvent_textInputs.size() };
					HandleCustomEvent_TextInput(
						implData,
						backendData,
						data.start,
						data.count,
						replacementText);
					break;
				}

				case CustomEvent::Type::EndTextInputSession: {
					HandleCustomEvent_EndTextInputSession(implData, backendData);
					break;
				}

				default:
					DENGINE_IMPL_APPLICATION_UNREACHABLE();
					break;
			}
		}
		backendData.customEventQueue.clear();
		backendData.customEvent_textInputs.clear();
	}

	template<typename Callable>
	static void ProcessCustomEvents(
		Context::Impl& implData,
		BackendData& backendData,
		Callable callable)
	{
		ProcessCustomEvents_Internal(
			implData,
			backendData,
			Std::Opt{ callable });
	}

	static void ProcessCustomEvents(
		Context::Impl& implData,
		BackendData& backendData)
	{
		ProcessCustomEvents_Internal<ProcessCustomEvents_CallableT>(
			implData,
			backendData,
			Std::nullOpt);
	}

	void ProcessCustomEvent(
		Context::Impl& implData,
		BackendData& backendData,
		Std::Opt<CustomEvent_CallbackFnT> const& customCallback)
	{
		std::lock_guard scopedLock { backendData.customEventQueueLock };
		DENGINE_IMPL_APPLICATION_ASSERT(!backendData.customEventQueue.empty());

		// Extract the oldest element, the first one in the list.
		auto event = Std::Move(backendData.customEventQueue.front());
		backendData.customEventQueue.erase(backendData.customEventQueue.begin());

		switch (event.type)
		{
			case CustomEvent::Type::NativeWindowCreated: {
				auto const& data = event.data.nativeWindowCreated;
				HandleEvent_NativeWindowCreated(
					implData,
					backendData,
					data.nativeWindow);
				break;
			}

			case CustomEvent::Type::NativeWindowDestroyed: {
				HandleEvent_NativeWindowDestroyed(implData);
				break;
			}

			case CustomEvent::Type::InputQueueCreated: {
				auto const& data = event.data.inputQueueCreated;
				HandleEvent_InputQueueCreated(backendData, data.inputQueue);
				break;
			}

			case CustomEvent::Type::VisibleAreaChanged: {
				auto const& data = event.data.visibleAreaChanged;
				HandleEvent_VisibleAreaChanged(
					implData,
					backendData,
					{ data.offsetX, data.offsetY },
					{ data.width, data.height });
				break;
			}

			case CustomEvent::Type::NewOrientation: {
				auto const& data = event.data.newOrientation;
				HandleEvent_Reorientation(data.newOrientation);
				break;
			}

			case CustomEvent::Type::TextInput: {
				auto const& data = event.data.textInput;
				Std::Span replacementText = {
					backendData.customEvent_textInputs.data(),
					backendData.customEvent_textInputs.size() };
				HandleCustomEvent_TextInput(
					implData,
					backendData,
					data.start,
					data.count,
					replacementText);
				break;
			}

			case CustomEvent::Type::EndTextInputSession: {
				HandleCustomEvent_EndTextInputSession(implData, backendData);
				break;
			}

			default:
				DENGINE_IMPL_APPLICATION_UNREACHABLE();
				break;
		}

		if (customCallback.Has())
			customCallback.Get()(event);
	}

	int looperCallback_CustomEvent(int fd, int events, void* data) {
		DENGINE_IMPL_APPLICATION_ASSERT(events == ALOOPER_EVENT_INPUT);

		auto& pollSource = *(PollSource*)data;
		DENGINE_IMPL_APPLICATION_ASSERT(pollSource.implData);
		DENGINE_IMPL_APPLICATION_ASSERT(pollSource.backendData);

		auto& implData = *pollSource.implData;
		auto& backendData = *pollSource.backendData;

		ProcessCustomEvent(implData, backendData, pollSource.customEvent_CallbackFnOpt);

		eventfd_t value = 0;
		eventfd_read(fd, &value);

		return 1; // Returning 1 means we want to continue having this event registered.
	}

	static void WaitForInitialRequiredEvents(
		Context::Impl& implData,
		BackendData& backendData)
	{
		bool nativeWindowSet = false;
		bool inputQueueSet = false;
		bool visibleAreaSet = false;
		while (!inputQueueSet || !nativeWindowSet || !visibleAreaSet)
		{
			auto& pollSource = backendData.pollSource;
			pollSource.backendData = &backendData;
			pollSource.implData = &implData;
			pollSource.customEvent_CallbackFnOpt =
				[&inputQueueSet, &nativeWindowSet, &visibleAreaSet](CustomEvent const &event)
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
			};


			int pollResult = ALooper_pollAll(
				0,
				nullptr,
				nullptr,
				nullptr);
			if (pollResult == ALOOPER_POLL_TIMEOUT) {
				ALooper_pollOnce(
					-1,
					nullptr,
					nullptr,
					nullptr);
			}

			pollSource.backendData = nullptr;
			pollSource.implData = nullptr;
			pollSource.customEvent_CallbackFnOpt = Std::nullOpt;
		}
	}
}

void* Application::impl::Backend::Initialize(
	Context& ctx,
	Context::Impl& implData)
{
	auto& backendData = *pBackendData;

	// First we attach this thread to the JVM
	jint attachThreadResult = backendData.nativeActivity->vm->AttachCurrentThread(
		&backendData.gameThreadJniEnv,
		nullptr);
	if (attachThreadResult != JNI_OK)
	{
		// Attaching failed. Crash the program.
		std::abort();
	}

	jclass clazz = backendData.gameThreadJniEnv->GetObjectClass(backendData.mainActivity);
	backendData.jniOpenSoftInput = backendData.gameThreadJniEnv->GetMethodID(
		clazz,
		"nativeEvent_openSoftInput",
		"(Ljava/lang/String;I)V");
	backendData.jniHideSoftInput = backendData.gameThreadJniEnv->GetMethodID(
		clazz,
		"nativeEvent_hideSoftInput",
		"()V");

	ALooper* looper = ALooper_prepare(0); // ALOOPER_PREPARE_ALLOW_NON_CALLBACKS (?)
	ALooper_addFd(
		looper,
		backendData.customEventFd,
		(int)LooperIdentifier::CustomEvent,
		ALOOPER_EVENT_INPUT,
		&looperCallback_CustomEvent,
		&backendData.pollSource);

	WaitForInitialRequiredEvents(implData, backendData);

	AInputQueue_attachLooper(
		backendData.inputQueue,
		looper,
		(int)LooperIdentifier::InputQueue,
		&testCallback,
		&backendData.pollSource);
	return pBackendData;
}

namespace DEngine::Application::impl
{
	static int BuildPollingTimeout(bool waitForEvents, u64 timeoutNs)
	{
		if (waitForEvents) {
			if (timeoutNs != 0) {
				// Not implemented
				DENGINE_IMPL_APPLICATION_UNREACHABLE();
			} else {
				// Negative numbers will wait indefinitely.
				return -1;
			}
		}
		else {
			// The polling will exit immediately if no event is found.
			return 0;
		}
	}
}

void Application::impl::Backend::ProcessEvents(
	Context& ctx,
	Context::Impl& implData,
	void* pBackendDataIn,
	bool waitForEvents,
	u64 timeoutNs)
{
	auto& backendData = *static_cast<BackendData*>(pBackendDataIn);

	int pollingTimeout = 0;

	int eventCount = 0; // What even is this?
	int fileDescriptor = 0;
	void* source = nullptr;

	backendData.pollSource.implData = &implData;
	backendData.pollSource.backendData = &backendData;

	if (waitForEvents) {
		bool continueSpinning = true;
		while (continueSpinning) {
			int pollResult = ALooper_pollOnce(
				3000,
				&fileDescriptor,
				&eventCount,
				&source);
			if (pollResult == ALOOPER_POLL_CALLBACK) {
				continueSpinning = false;
			} else if (pollResult == ALOOPER_POLL_TIMEOUT) {
				continueSpinning = true;
			}
		}
	} else {
		int pollResult = ALooper_pollAll(
			0, // Returns immediately
			&fileDescriptor,
			&eventCount,
			&source);
	}

	backendData.pollSource.implData = nullptr;
	backendData.pollSource.backendData = nullptr;

	// Process our custom events
	ProcessCustomEvents(implData, backendData);
}

auto Application::impl::Backend::NewWindow(
	Context& ctx,
	Context::Impl& implData,
	void* pBackendDataIn,
	WindowID windowId,
	Std::Span<char const> const& title,
	Extent extent) -> Std::Opt<NewWindow_ReturnT>
{
	auto& backendData = *static_cast<BackendData*>(pBackendDataIn);

	// We don't support multiple windows.
	if (backendData.currentWindow.Has() || backendData.nativeWindow == nullptr)
		return Std::nullOpt;

	backendData.currentWindow = windowId;

	NewWindow_ReturnT returnValue = {};
	returnValue.platformHandle = backendData.nativeWindow;

	auto& windowData = returnValue.windowData;

	int width = ANativeWindow_getWidth(backendData.nativeWindow);
	int height = ANativeWindow_getHeight(backendData.nativeWindow);
	windowData.extent.width = (u32)width;
	windowData.extent.height = (u32)height;
	windowData.visibleOffset = backendData.visibleAreaOffset;
	windowData.visibleExtent = backendData.visibleAreaExtent;

	return returnValue;
}

void Application::impl::Backend::DestroyWindow(
	Context::Impl& implData,
	void* backendDataIn,
	Context::Impl::WindowNode const& windowNode)
{
	auto& backendData = *static_cast<BackendData*>(backendDataIn);
	bool invalidDestroy =
		!backendData.currentWindow.Has() ||
		backendData.currentWindow.Get() != windowNode.id;
	if (invalidDestroy)
		throw std::runtime_error("Cannot destroy this window.");

	backendData.currentWindow = Std::nullOpt;
}

Application::Context::CreateVkSurface_ReturnT Application::impl::Backend::CreateVkSurface(
	Context::Impl& implData,
	void* pBackendDataIn,
	void* platformHandle,
	uSize vkInstanceIn,
	void const* vkAllocationCallbacks) noexcept
{
	auto& backendData = *(BackendData*)pBackendDataIn;

	Context::CreateVkSurface_ReturnT returnValue = {};

	// If we have no active window, we can't make a surface.
	if (!backendData.nativeWindow || platformHandle != backendData.nativeWindow)
	{
		returnValue.vkResult = VK_ERROR_UNKNOWN;
		return returnValue;
	}


	// Load the function pointer
	if (backendData.vkGetInstanceProcAddrFn == nullptr)
	{
		void* lib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
		if (lib == nullptr)
		{
			returnValue.vkResult = VK_ERROR_UNKNOWN;
			return returnValue;
		}

		auto procAddr = dlsym(lib, "vkGetInstanceProcAddr");
		backendData.vkGetInstanceProcAddrFn = procAddr;
		int closeReturnCode = dlclose(lib);
		if (procAddr == nullptr || closeReturnCode != 0)
		{
			returnValue.vkResult = VK_ERROR_UNKNOWN;
			return returnValue;
		}
	}

	VkInstance instance = {};
	static_assert(sizeof(instance) == sizeof(vkInstanceIn));
	memcpy(&instance, &vkInstanceIn, sizeof(instance));

	auto procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(backendData.vkGetInstanceProcAddrFn);

	// Load the function pointer
	auto funcPtr = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
		procAddr(
			instance,
			"vkCreateAndroidSurfaceKHR"));
	if (funcPtr == nullptr)
	{
		returnValue.vkResult = VK_ERROR_UNKNOWN;
		return returnValue;
	}

	VkAndroidSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.window = backendData.nativeWindow;

	VkSurfaceKHR surface = {};
	auto result = funcPtr(
		instance,
		&createInfo,
		static_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks),
		&surface);

	// Memcpy the values into the return struct
	static_assert(sizeof(result) == sizeof(returnValue.vkResult));
	memcpy(&returnValue.vkResult, &result, returnValue.vkResult);
	static_assert(sizeof(surface) == sizeof(returnValue.vkSurface));
	memcpy(&returnValue.vkSurface, &surface, sizeof(returnValue.vkSurface));

	return returnValue;
}

Std::StackVec<char const*, 5> DEngine::Application::RequiredVulkanInstanceExtensions() noexcept
{
	Std::StackVec<char const*, 5> returnVal{};
	returnVal.PushBack("VK_KHR_surface");
	returnVal.PushBack("VK_KHR_android_surface");
	return returnVal;
}

void Application::impl::Backend::Destroy(void* data)
{

}

void Application::impl::Backend::Log(
	Context::Impl& implData,
	LogSeverity severity,
	Std::Span<char const> const& msg)
{
	std::string outString;
	outString.reserve(msg.Size());
	for (auto const& item : msg)
		outString.push_back(item);

	int prio = 0;
	switch (severity) {
		case LogSeverity::Debug:
			prio = ANDROID_LOG_DEBUG;
			break;
		case LogSeverity::Error:
			prio = ANDROID_LOG_ERROR;
			break;
		case LogSeverity::Warning:
			prio = ANDROID_LOG_WARN;
			break;
		default:
			DENGINE_IMPL_APPLICATION_UNREACHABLE();
			break;
	}

	__android_log_print(prio, "DEngine: ", "%s", outString.c_str());
}

bool Application::impl::Backend::StartTextInputSession(
	Context::Impl& implData,
	void* pBackendDataIn,
	SoftInputFilter inputFilter,
	Std::Span<char const> const& inputString)
{
	auto& backendData = *(BackendData*)pBackendDataIn;

	std::vector<jchar> tempString;
	tempString.reserve(inputString.Size());
	for (uSize i = 0; i < inputString.Size(); i++) {
		tempString.push_back((jchar)inputString[i]);
	}
	jstring javaString = backendData.gameThreadJniEnv->NewString(tempString.data(), tempString.size());
	backendData.gameThreadJniEnv->CallVoidMethod(
		backendData.mainActivity,
		backendData.jniOpenSoftInput,
		javaString,
		(jint)inputFilter);

	backendData.gameThreadJniEnv->functions->DeleteLocalRef(
		backendData.gameThreadJniEnv,
		javaString);

	return true;
}

void Application::impl::Backend::StopTextInputSession(
	Context::Impl& implData,
	void* backendDataIn)
{
	auto& backendData = *(BackendData*)backendDataIn;

	backendData.gameThreadJniEnv->CallVoidMethod(
		backendData.mainActivity,
		backendData.jniHideSoftInput);
}

