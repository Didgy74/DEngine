#include <DEngine/impl/Application.hpp>

#include "HandleCustomEvent.hpp"

#include "BackendData.hpp"
#include "HandleInputEvent.hpp"

#include <DEngine/impl/AppAssert.hpp>
#include <DEngine/Std/Utility.hpp>

#include <android/log.h>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>

#include <sys/eventfd.h>

namespace DEngine::Application::impl
{
	void ProcessCustomEvent(
		Context::Impl& implData,
		BackendData& backendData,
		Std::Opt<CustomEvent_CallbackFnT> const& customCallback)
	{
		auto lock = std::lock_guard{ backendData.customEventQueueLock };

		DENGINE_IMPL_APPLICATION_ASSERT(!backendData.customEventQueue.empty());
		auto event = Std::Move(backendData.customEventQueue.front());
		backendData.customEventQueue.erase(backendData.customEventQueue.begin());

		if (customCallback.Has()) {
			customCallback.Get().Invoke(event.type);
		}
		event.fn(implData, backendData);

		if (backendData.customEventQueue.empty()) {
			// Clear up some resources
			backendData.customEvent_textInputs.clear();
		}
	}
}

using namespace DEngine;
using namespace DEngine::Application;
using namespace DEngine::Application::impl;

int Application::impl::looperCallback_CustomEvent(int fd, int events, void* data)
{
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

namespace DEngine::Application::impl {
	/*
 * 	CUSTOM EVENTS
 *
 *	The following functions are our custom events. Some of them are in place of
 *	the ones exposed by the NativeActivity interface because not all of those
 *	work anymore because we override the active View in the Java code.
 *	Also there are extra ones for the functionality we need that is not exposed in
 *	ANativeActivity.
 */

	template<class Callable>
	static void PushCustomEvent_Insert_Inner(
		BackendData& backendData,
		CustomEventType eventType,
		Callable&& in)
	{
		backendData.customEventQueue.push_back({ eventType, Std::Move(in) });
	}
	enum class PushCustomEvent_UseLock { Yes, No };
	template<class Callable> requires (!Std::Trait::isRef<Callable>)
	static void PushCustomEvent_Insert(
		BackendData& backendData,
		CustomEventType eventType,
		Callable&& in,
		PushCustomEvent_UseLock useLock = PushCustomEvent_UseLock::Yes)
	{
		if (useLock == PushCustomEvent_UseLock::Yes)
			backendData.customEventQueueLock.lock();


		PushCustomEvent_Insert_Inner(backendData, eventType, Std::Move(in));
		eventfd_write(backendData.customEventFd, 1);


		if (useLock == PushCustomEvent_UseLock::Yes)
			backendData.customEventQueueLock.unlock();
	}

	static void PushCustomEvent_NativeWindowCreated(BackendData& backendData, ANativeWindow* window) {
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			DENGINE_IMPL_APPLICATION_ASSERT(backendData.nativeWindow == nullptr);
			backendData.nativeWindow = window;

			if (backendData.currWindowId.Has()) {
				auto windowId = backendData.currWindowId.Get();

				auto* windowNodePtr = implData.GetWindowNode(windowId);
				DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr != nullptr);
				auto& windowNode = *windowNodePtr;
				DENGINE_IMPL_APPLICATION_ASSERT(windowNode.platformHandle == nullptr);
				windowNode.platformHandle = window;

				BackendInterface::PushWindowMinimizeSignal(implData, windowId, false);
			}
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::NativeWindowCreated,
			Std::Move(job));
	}
	static void PushCustomEvent_NativeWindowResized(BackendData& backendData, ANativeWindow* window)
	{
		// We don't actually care (yet?)
	}
	static void PushCustomEvent_NativeWindowDestroyed(BackendData& backendData, ANativeWindow* window)
	{
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			//DENGINE_IMPL_APPLICATION_ASSERT(backendData.nativeWindow == window);
			ANativeWindow_release(backendData.nativeWindow);
			backendData.nativeWindow = nullptr;

			if (backendData.currWindowId.Has()) {
				auto windowId = backendData.currWindowId.Get();

				auto* windowNodePtr = implData.GetWindowNode(windowId);
				DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr != nullptr);
				auto& windowNode = *windowNodePtr;
				DENGINE_IMPL_APPLICATION_ASSERT(windowNode.platformHandle != nullptr);
				windowNode.platformHandle = nullptr;

				BackendInterface::PushWindowMinimizeSignal(implData, windowId,true);
			}
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::NativeWindowDestroyed,
			Std::Move(job));
	}
	static void PushCustomEvent_InputQueueCreated(BackendData& backendData, AInputQueue* queue)
	{
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			DENGINE_IMPL_APPLICATION_ASSERT(backendData.gameThreadAndroidLooper != nullptr);

			backendData.inputQueue = queue;

			AInputQueue_attachLooper(
			backendData.inputQueue,
			backendData.gameThreadAndroidLooper,
			(int)LooperIdentifier::InputQueue,
			&looperCallback_InputEvent,
			&backendData.pollSource);
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::InputQueueCreated,
			Std::Move(job));
	}
	static void PushCustomEvent_InputQueueDestroyed(BackendData& backendData, AInputQueue* queue)
	{
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			DENGINE_IMPL_APPLICATION_ASSERT(backendData.gameThreadAndroidLooper != nullptr);
			AInputQueue_detachLooper(queue);
			backendData.inputQueue = nullptr;
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::InputQueueDestroyed,
			Std::Move(job));
	}
	static void PushCustomEvent_ConfigurationChanged(BackendData& backendData)
	{
		auto job = [](Context::Impl& implData, BackendData& backendData) {
			AConfiguration_fromAssetManager(backendData.currAConfig, backendData.assetManager);
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::ConfiguationChanged,
			Std::Move(job));
	}
	static void PushCustomEvent_DeviceOrientation(BackendData& backendData, int aOrient)
	{

	}
	void PushCustomEvent_ContentRectChanged(
		BackendData& backendData,
		u32 posX,
		u32 posY,
		u32 width,
		u32 height)
	{
		Math::Vec2UInt posOffset = { posX, posY };
		Extent extent = { width, height };
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			backendData.visibleAreaOffset = posOffset;
			backendData.visibleAreaExtent = extent;

			if (backendData.currWindowId.HasValue())
			{
				// I don't really know what to do if the native window handle is null
				DENGINE_IMPL_APPLICATION_ASSERT(backendData.nativeWindow);

				auto windowWidth = (uint32_t)ANativeWindow_getWidth(backendData.nativeWindow);
				auto windowHeight = (uint32_t)ANativeWindow_getHeight(backendData.nativeWindow);
				BackendInterface::UpdateWindowSize(
					implData,
					backendData.currWindowId.Get(),
					{ windowWidth, windowHeight },
					posOffset.x,
					posOffset.y,
					extent);
			}
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::ContentRectChanged,
			Std::Move(job));
	}

	struct TextInputJob {
		uSize oldStart;
		uSize oldCount;
		uSize textOffset;
		uSize textCount;
	};
	// This takes an array of events because multiple input jobs can happen
	// at the same timestamp and be submitted together
	void PushCustomEvent_TextInput(
		BackendData& backendData,
		Std::Span<TextInputJob const> inputJobs,
		Std::Span<u32 const> inputText)
	{
		// Acquire the lock early because we need to know the text-buffer size.
		std::lock_guard _{ backendData.customEventQueueLock };

		// We need to know the size of the text input buffer,
		// and then append our new content to it afterwards.
		auto& textBuffer = backendData.customEvent_textInputs;
		auto oldTextBufferLen = textBuffer.size();
		auto newLen = oldTextBufferLen + inputText.Size();
		textBuffer.resize(newLen);
		for (int i = 0; i < inputText.Size(); i++) {
			// Can't be zero value character
			DENGINE_IMPL_APPLICATION_ASSERT(inputText[i] != 0);
			textBuffer[i + oldTextBufferLen] = inputText[i];
		}

		for (auto const job : inputJobs) {
			auto textInputStartIndex = oldTextBufferLen + job.textOffset;
			auto temp = [job, textInputStartIndex](Context::Impl& implData, BackendData& backendData)
			{
				auto& textBuffer = backendData.customEvent_textInputs;
				DENGINE_IMPL_APPLICATION_ASSERT(textInputStartIndex + job.textCount <= textBuffer.size());
				Std::Span replacementText = {
					textBuffer.data() + textInputStartIndex,
					job.textCount };
				DENGINE_IMPL_APPLICATION_ASSERT(job.oldCount != 0 || !replacementText.Empty());
				impl::BackendInterface::PushTextInputEvent(
					implData,
					backendData.currWindowId.Get(),
					job.oldStart,
					job.oldCount,
					replacementText);
			};

			// We already locked the queue in this function scope, we don't want to lock it again.
			PushCustomEvent_Insert(
				backendData,
				CustomEventType::TextInput,
				Std::Move(temp),
				PushCustomEvent_UseLock::No);
		}

		//eventfd_t fdIncrement = inputJobs.Size();
		//eventfd_write(backendData.customEventFd, fdIncrement);
	}

	void PushCustomEvent_EndTextInputSession(BackendData& backendData) {
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			if (backendData.currWindowId.Has()) {
				impl::BackendInterface::PushEndTextInputSessionEvent(
					implData,
					backendData.currWindowId.Get());
			}
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::EndTextInputSession,
			Std::Move(job));
	}

	void PushCustomEvent_OnResume(BackendData& backendData) {
		// We moved this stuff into the nativeWindow-events instead.
		/*
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			if (backendData.currentWindow.Has()) {
				BackendInterface::PushWindowMinimizeSignal(
					implData,
					backendData.currentWindow.Get(),
					false);
			}
		};
		PushCustomEvent_Insert2(
			backendData,
			CustomEvent::Type::OnResume,
			job);
		 */
	}
	void PushCustomEvent_OnPause(BackendData& backendData) {
		// We moved this stuff into the nativeWindow-events instead.
		/*
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			if (backendData.currentWindow.Has()) {
				BackendInterface::PushWindowMinimizeSignal(
					implData,
					backendData.currentWindow.Get(),
					true);
			}
		};
		PushCustomEvent_Insert2(
			backendData,
			CustomEvent::Type::OnPause,
			job);
		*/
	}

	static void LogEvent(char const* msg)
	{
		__android_log_print(ANDROID_LOG_DEBUG, "DEngine - Event", "%s", msg);
	}

	static void onStart(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("Start");
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onPause(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("On Pause");
		PushCustomEvent_OnPause(GetBackendData(activity));
	}

	static void onResume(ANativeActivity* activity) {
		if constexpr (logEvents)
			LogEvent("On Resume");
		PushCustomEvent_OnResume(GetBackendData(activity));
	}

	static void onStop(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("On Stop");
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onDestroy(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("On Destroy");
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onNativeWindowCreated(BackendData& backendData, ANativeWindow* window) {
		if constexpr (logEvents)
			LogEvent("Native window created");
		PushCustomEvent_NativeWindowCreated(backendData, window);
	}
	static void onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window) {
		if constexpr (logEvents)
			LogEvent("Native window resized");
		auto& backendData = GetBackendData(activity);
		PushCustomEvent_NativeWindowResized(backendData, window);
	}
	static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
		if constexpr (logEvents)
			LogEvent("Native window destroyed");
		auto& backendData = GetBackendData(activity);
		PushCustomEvent_NativeWindowDestroyed(backendData, window);
	}

	static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
		if constexpr (logEvents)
			LogEvent("Input queue created");
		auto& backendData = GetBackendData(activity);
		PushCustomEvent_InputQueueCreated(backendData, queue);
	}
	static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
		if constexpr (logEvents)
			LogEvent("Input queue destroyed");
		auto& backendData = GetBackendData(activity);
		PushCustomEvent_InputQueueDestroyed(backendData, queue);
	}
	static void onConfigurationChanged(ANativeActivity* activity) {
		if constexpr (logEvents)
			LogEvent("Configuration changed");
		auto& backendData = GetBackendData(activity);
		PushCustomEvent_ConfigurationChanged(backendData);
	}
	static void onContentRectChanged(ANativeActivity* activity, ARect const* rect) {
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

	[[nodiscard]] BackendData* InitBackendData(
		JNIEnv* env,
        jobject app,
		jclass appClass,
		jobject activity,
		jobject assetManager,
		float fontScale)
	{
		// Create our backendData, store it in the global pointer
		auto backendPtr = new BackendData();
		auto& backendData = *backendPtr;

		auto javaVmResult = env->GetJavaVM(&backendData.globalJavaVm);
		if (javaVmResult != 0) {
			// TODO: Error
		}

		backendData.appHandle = env->NewGlobalRef(app);
		backendData.appClass = env->GetObjectClass(app);
		backendData.mainActivity = env->NewGlobalRef(activity);
		backendData.assetManager = AAssetManager_fromJava(env, assetManager);
		backendData.currAConfig = AConfiguration_new();
		AConfiguration_fromAssetManager(backendData.currAConfig, backendData.assetManager);

		backendData.fontScale = fontScale;

		// And we create our file descriptor for our custom events
		// We need to create this right away because
		// our first events (and need to signal this) will happen before the other thread
		// can initialize something like this.
		backendData.customEventFd = eventfd(0, EFD_SEMAPHORE);

		// Now we run our apps int main-equivalent on a new thread.
		// The remaining part of initialization is handled by that thread.
		// From this point onwards, this new thread is what owns the backendData.
		backendData.gameThread = std::thread([]() {
			DENGINE_MAIN_ENTRYPOINT(0, nullptr);
		});

		return backendPtr;
	}

	// Called by the main Java thread during startup,
	// this is the very first C code that is run in our
	// project.
	void OnCreate(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("Create");

		// Set all the callbacks exposed by the
		// ANativeActivity interface.
		activity->callbacks->onStart = [](ANativeActivity* nativeActivity) {

		};
		activity->callbacks->onPause = [](ANativeActivity* nativeActivity) {

		};
		activity->callbacks->onResume = &onResume;
		activity->callbacks->onStop = &onStop;
		activity->callbacks->onDestroy = &onDestroy;
		activity->callbacks->onNativeWindowCreated = [](
			ANativeActivity* nativeActivity,
			ANativeWindow* newWindow)
		{
			PushCustomEvent_NativeWindowCreated(
				*(BackendData*)nativeActivity->instance,
				newWindow);
		};
		activity->callbacks->onNativeWindowDestroyed = &onNativeWindowDestroyed;
		activity->callbacks->onNativeWindowResized = &onNativeWindowResized;
		activity->callbacks->onInputQueueCreated = &onInputQueueCreated;
		activity->callbacks->onInputQueueDestroyed = &onInputQueueDestroyed;
		activity->callbacks->onConfigurationChanged = &onConfigurationChanged;
		activity->callbacks->onContentRectChanged = &onContentRectChanged;
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
	using namespace DEngine::Application::impl;

	//OnCreate(activity);
}

// Called at the end of onCreate in Java.
extern "C"
JNIEXPORT jlong
Java_didgy_dengine_NativeInterface_init(
	JNIEnv* env,
	jclass clazz,
	jobject app,
	jclass appClass,
	jobject activity,
	jobject assetManager,
	jfloat fontScale)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	auto backendPtr = InitBackendData(
		env,
        app,
		appClass,
		activity,
		assetManager,
		fontScale);
	pBackendDataInit = backendPtr;

	jlong returnValue = {};

	// We need to make sure we can fit this pointer in the return value.
	static_assert(sizeof(returnValue) >= sizeof(backendPtr));
	memcpy( &returnValue, &backendPtr, sizeof(backendPtr));
	return returnValue;
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_NativeInterface_onTextInput(
	JNIEnv *env,
	jclass clazz,
	jlong backendPtr,
	jintArray infos_start,
	jintArray infos_count,
	jintArray infos_textOffset,
	jintArray infos_textCount,
	jstring allStrings)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	auto& backendData = GetBackendData(backendPtr);

	// Copy the string
	jsize textLen = env->GetStringLength(allStrings);
	std::vector<jchar> tempJniString;
	tempJniString.resize(textLen);
	env->GetStringRegion(allStrings, 0, textLen, tempJniString.data());
	std::vector<u32> outString(textLen);
	for (int i = 0; i < textLen; i++) {
		DENGINE_IMPL_APPLICATION_ASSERT(tempJniString[i] != 0);
		outString[i] = (u32)tempJniString[i];
	}

	jsize infoLen = env->GetArrayLength(infos_start);
	DENGINE_IMPL_APPLICATION_ASSERT(infoLen > 0);
	auto convert = [=](jintArray infos) {
		std::vector<int> tempInts(infoLen);
		env->GetIntArrayRegion(infos, 0, infoLen, tempInts.data());
		return tempInts;
	};
	auto starts = convert(infos_start);
	auto counts = convert(infos_count);
	auto textOffsets = convert(infos_textOffset);
	auto textCount = convert(infos_textCount);

	std::vector<TextInputJob> inputJobs;
	inputJobs.reserve(starts.size());

	for (int i = 0; i < starts.size(); i++) {
		TextInputJob newJob = {};
		newJob.oldStart = starts[i];
		newJob.oldCount = counts[i];
		newJob.textOffset = textOffsets[i];
		newJob.textCount = textCount[i];
		DENGINE_IMPL_APPLICATION_ASSERT(
			newJob.oldCount != 0 || newJob.textCount != 0);
		DENGINE_IMPL_APPLICATION_ASSERT(
			newJob.textOffset + newJob.textCount <= outString.size());
		inputJobs.push_back(newJob);
	}

	PushCustomEvent_TextInput(
		backendData,
		{ inputJobs.data(), inputJobs.size() },
		{ outString.data(), outString.size() });
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_NativeInterface_sendEventEndTextInputSession(
		JNIEnv* env,
		jclass object,
		jlong backendDataPtr)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	PushCustomEvent_EndTextInputSession(GetBackendData(backendDataPtr));
}

// We do not use ANativeActivity's View in this implementation,
// due to not being able to access it Java side and therefore cannot
// activate the soft input on it. That means ANativeActivity's callback for
// onContentRectChanged doesn't actually work.
// We need our own custom event for the View that we actually use.
extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL
Java_didgy_dengine_NativeInterface_onContentRectChanged(
	JNIEnv* env,
	jclass clazz,
	jlong backendPtr,
	jint posX,
	jint posY,
	jint width,
	jint height)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	PushCustomEvent_ContentRectChanged(
		GetBackendData(backendPtr),
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
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_NativeInterface_onNewOrientation(
		JNIEnv* env,
		jclass thiz,
		jlong backendPtr,
		jint newOrientation)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	PushCustomEvent_DeviceOrientation(
			GetBackendData(backendPtr),
			newOrientation);
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_NativeInterface_onFontScale(
		JNIEnv* env,
		jclass thiz,
		jlong backendPtr,
		jint windowId,
		jfloat newScale)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_NativeInterface_onNativeWindowCreated(
	JNIEnv* env,
	jclass thiz,
	jlong backendPtr,
	jobject surface)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	auto& backendData = GetBackendData(backendPtr);

	auto* nativeWindowHandle = ANativeWindow_fromSurface(env, surface);
	PushCustomEvent_NativeWindowCreated(backendData, nativeWindowHandle);
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_NativeInterface_onNativeWindowDestroyed(
	JNIEnv *env,
	jclass clazz,
	jlong backendPtr,
	jobject surface)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	auto& backendData = GetBackendData(backendPtr);

	auto* nativeWindowHandle = ANativeWindow_fromSurface(env, surface);
	PushCustomEvent_NativeWindowDestroyed(backendData, nativeWindowHandle);
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_NativeInterface_onTouch(
	JNIEnv *env,
	jclass clazz,
	jlong backendPtr,
	jint pointerId,
	jfloat x,
	jfloat y,
	jint action)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	auto toTouchEventType = [](jint action) -> TouchEventType {
		switch (action) {
			case AMOTION_EVENT_ACTION_DOWN: return TouchEventType::Down;
			case AMOTION_EVENT_ACTION_UP: return TouchEventType::Up;
			case AMOTION_EVENT_ACTION_MOVE: return TouchEventType::Moved;
			default:
				return TouchEventType::Unchanged;
		}
	};
	auto touchEventType = toTouchEventType(action);
	DENGINE_IMPL_APPLICATION_ASSERT(touchEventType != TouchEventType::Unchanged);

	auto& backendData = GetBackendData(backendPtr);

	PushCustomEvent_Insert(
		backendData,
		CustomEventType::Unknown,
		[=](Context::Impl& implData, BackendData& backendData) {
			BackendInterface::UpdateTouch(
				implData,
				backendData.currWindowId.Get(),
				touchEventType,
				(u8)pointerId,
				x,
				y);
		});
}
