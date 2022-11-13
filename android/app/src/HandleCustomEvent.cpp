#include <DEngine/impl/Application.hpp>

#include "HandleCustomEvent.hpp"

#include "BackendData.hpp"
#include "HandleInputEvent.hpp"

#include <DEngine/impl/AppAssert.hpp>
#include <DEngine/Std/Utility.hpp>

#include <android/log.h>

#include <sys/eventfd.h>

namespace DEngine::Application::impl
{
	void ProcessCustomEvent(
		Context::Impl& implData,
		BackendData& backendData,
		Std::Opt<CustomEvent_CallbackFnT> const& customCallback)
	{
		auto lock = std::lock_guard{ backendData.customEventQueueLock };

		DENGINE_IMPL_APPLICATION_ASSERT(!backendData.customEventQueue2.empty());
		auto event = Std::Move(backendData.customEventQueue2.front());
		backendData.customEventQueue2.erase(backendData.customEventQueue2.begin());

		if (customCallback.Has()) {
			customCallback.Get().Invoke(event.type);
		}
		event.fn(implData, backendData);

		// If we ran out of events now, we can safely reset the underlying allocator
		// the event lambdas rely on.
		if (backendData.customEventQueue2.empty()) {
			backendData.queuedEvents_InnerBuffer.Reset(false);
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
	static void PushCustomEvent_Insert2_Inner(
			BackendData& backendData,
			CustomEventType eventType,
			Callable const& in)
	{
		auto* tempPtr = (Callable*)backendData.queuedEvents_InnerBuffer.Alloc(sizeof(Callable), alignof(Callable));
		DENGINE_IMPL_APPLICATION_ASSERT(tempPtr != nullptr);
		new(tempPtr) Callable(in);

		backendData.customEventQueue2.push_back({ eventType, *tempPtr });
	}
	template<class Callable>
	static void PushCustomEvent_Insert2(
			BackendData& backendData,
			CustomEventType eventType,
			Callable const& in)
	{
		std::lock_guard _{ backendData.customEventQueueLock };

		PushCustomEvent_Insert2_Inner(backendData, eventType, in);

		eventfd_write(backendData.customEventFd, 1);
	}

	static void PushCustomEvent_NativeWindowCreated(BackendData& backendData, ANativeWindow* window)
	{
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
		PushCustomEvent_Insert2(
				backendData,
				CustomEventType::NativeWindowCreated,
				job);
	}
	static void PushCustomEvent_NativeWindowResized(BackendData& backendData, ANativeWindow* window)
	{
		// We don't actually care (yet?)
	}
	static void PushCustomEvent_NativeWindowDestroyed(BackendData& backendData, ANativeWindow* window)
	{
		auto job = [=](Context::Impl& implData, BackendData& backendData) {

			DENGINE_IMPL_APPLICATION_ASSERT(backendData.nativeWindow == window);
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
		PushCustomEvent_Insert2(
				backendData,
				CustomEventType::NativeWindowDestroyed,
				job);
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

		PushCustomEvent_Insert2(
				backendData,
				CustomEventType::InputQueueCreated,
				job);
	}
	static void PushCustomEvent_InputQueueDestroyed(BackendData& backendData, AInputQueue* queue)
	{
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			DENGINE_IMPL_APPLICATION_ASSERT(backendData.gameThreadAndroidLooper != nullptr);
			AInputQueue_detachLooper(queue);
			backendData.inputQueue = nullptr;
		};
		PushCustomEvent_Insert2(
			backendData,
			CustomEventType::InputQueueDestroyed,
			job);
	}
	static void PushCustomEvent_ConfigurationChanged(BackendData& backendData)
	{
		auto job = [](Context::Impl& implData, BackendData& backendData) {
			AConfiguration_fromAssetManager(backendData.currAConfig, backendData.assetManager);
		};
		PushCustomEvent_Insert2(
			backendData,
			CustomEventType::ConfiguationChanged,
			job);
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
		PushCustomEvent_Insert2(
				backendData,
				CustomEventType::ContentRectChanged,
				job);
	}

	struct TextInputJob {
		uSize oldStart;
		uSize oldCount;
		uSize textOffset;
		uSize textCount;
	};
	constexpr int textInputJobMemberCount = 4;
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

		for (auto const& job : inputJobs) {
			auto textInputStartIndex = oldTextBufferLen + job.textOffset;
			auto temp = [=](Context::Impl& implData, BackendData& backendData)
			{
				auto& textBuffer = backendData.customEvent_textInputs;
				DENGINE_IMPL_APPLICATION_ASSERT(
						textInputStartIndex + job.textCount <= textBuffer.size());
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

			PushCustomEvent_Insert2_Inner(
					backendData,
					CustomEventType::TextInput,
					temp);
		}

		eventfd_t fdIncrement = inputJobs.Size();
		eventfd_write(backendData.customEventFd, fdIncrement);
	}

	void PushCustomEvent_EndTextInputSession(BackendData& backendData) {
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			if (backendData.currWindowId.Has()) {
				impl::BackendInterface::PushEndTextInputSessionEvent(
						implData,
						backendData.currWindowId.Get());
			}
		};
		PushCustomEvent_Insert2(
				backendData,
				CustomEventType::EndTextInputSession,
				job);
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

	static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
		if constexpr (logEvents)
			LogEvent("Native window created");
		auto& backendData = GetBackendData(activity);
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

	// Called by the main Java thread during startup,
	// this is the very first C code that is run in our
	// project.
	void OnCreate(ANativeActivity* activity)
	{
		if constexpr (logEvents)
			LogEvent("Create");

		// Set all the callbacks exposed by the
		// ANativeActivity interface.
		activity->callbacks->onStart = &onStart;
		activity->callbacks->onPause = &onPause;
		activity->callbacks->onResume = &onResume;
		activity->callbacks->onStop = &onStop;
		activity->callbacks->onDestroy = &onDestroy;
		activity->callbacks->onNativeWindowCreated = &onNativeWindowCreated;
		activity->callbacks->onNativeWindowDestroyed = &onNativeWindowDestroyed;
		activity->callbacks->onNativeWindowResized = &onNativeWindowResized;
		activity->callbacks->onInputQueueCreated = &onInputQueueCreated;
		activity->callbacks->onInputQueueDestroyed = &onInputQueueDestroyed;
		activity->callbacks->onConfigurationChanged = &onConfigurationChanged;
		activity->callbacks->onContentRectChanged = &onContentRectChanged;

		// Create our backendData, store it in the global pointer
		pBackendData = new BackendData();
		auto& backendData = *pBackendData;

		activity->instance = &backendData;

		backendData.nativeActivity = activity;
		backendData.globalJavaVm = activity->vm;

		// We want the object handle of our main activity, so we can load
		// method-IDs for it later.
		// We know our Java Activity is the one being used, it just inherits from
		// ANativeActivity.
		backendData.mainActivity = activity->env->NewGlobalRef(activity->clazz);
		backendData.assetManager = activity->assetManager;
		backendData.currAConfig = AConfiguration_new();
		AConfiguration_fromAssetManager(backendData.currAConfig, backendData.assetManager);

		// And we create our file descriptor for our custom events
		// We need to create this right away because
		// our first events (and need to signal this) will happen before the other thread
		// can initialize something like this.
		backendData.customEventFd = eventfd(0, EFD_SEMAPHORE);

		// Now we run our apps int main-equivalent on a new thread.
		// The remaining part of initialization is handled by that thread.
		auto lambda = []() {
			DENGINE_MAIN_ENTRYPOINT(0, nullptr);
		};

		// From this point onwards, this new thread is what owns the backendData.
		backendData.gameThread = std::thread(lambda);
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

	OnCreate(activity);
}

// Called at the end of onCreate in Java.
extern "C"
[[maybe_unused]] JNIEXPORT jdouble JNICALL Java_didgy_dengine_DEngineActivity_nativeInit(
		JNIEnv* env,
		jobject dengineActivity,
		jfloat fontScale)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	double returnValue = {};
	auto* backendPtr = pBackendData;

	auto& backendData = *backendPtr;
	backendData.fontScale = fontScale;

	// We need to make sure we can fit this pointer in the return value.
	static_assert(sizeof(returnValue) >= sizeof(backendPtr));
	memcpy(&returnValue, &backendPtr, sizeof(backendPtr));
	return returnValue;
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_DEngineActivity_nativeOnTextInput(
		JNIEnv* env,
		jobject dengineActivity,
		jdouble backendDataPtr,
		jintArray infos,
		jstring text)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	auto& backendData = GetBackendData(backendDataPtr);

	// Copy the
	jsize textLen = env->GetStringLength(text);
	std::vector<jchar> tempJniString;
	tempJniString.resize(textLen);
	env->GetStringRegion(text, 0, textLen, tempJniString.data());
	std::vector<u32> outString(textLen);
	for (int i = 0; i < textLen; i++) {
		DENGINE_IMPL_APPLICATION_ASSERT(tempJniString[i] != 0);
		outString[i] = (u32)tempJniString[i];
	}


	// Copy ints over to the temporary vector.
	jsize infoLen = env->GetArrayLength(infos);
	DENGINE_IMPL_APPLICATION_ASSERT(infoLen > 0);
	DENGINE_IMPL_APPLICATION_ASSERT(infoLen % textInputJobMemberCount == 0);
	std::vector<jint> tempInts;
	tempInts.resize(infoLen);
	env->GetIntArrayRegion(infos, 0, infoLen, tempInts.data());
	std::vector<TextInputJob> inputJobs(infoLen / textInputJobMemberCount);
	for (int i = 0; i < infoLen; i += 4) {
		auto* rawInts = tempInts.data() + i;
		auto& textInputJob = inputJobs[i / textInputJobMemberCount];
		textInputJob.oldStart = rawInts[0];
		textInputJob.oldCount = rawInts[1];
		textInputJob.textOffset = rawInts[2];
		textInputJob.textCount = rawInts[3];

		DENGINE_IMPL_APPLICATION_ASSERT(
				textInputJob.oldCount != 0 || textInputJob.textCount != 0);
		DENGINE_IMPL_APPLICATION_ASSERT(
				textInputJob.textOffset + textInputJob.textCount <= outString.size());
	}

	PushCustomEvent_TextInput(
			backendData,
			{ inputJobs.data(), inputJobs.size() },
			{ outString.data(), outString.size() });
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_DEngineActivity_nativeSendEventEndTextInputSession(
		JNIEnv* env,
		jobject dengineActivity,
		jdouble backendDataPtr)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	PushCustomEvent_EndTextInputSession(
			GetBackendData(backendDataPtr));
}

// We do not use ANativeActivity's View in this implementation,
// due to not being able to access it Java side and therefore cannot
// activate the soft input on it. That means ANativeActivity's callback for
// onContentRectChanged doesn't actually work.
// We need our own custom event for the View that we actually use.
extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_DEngineActivity_nativeOnContentRectChanged(
		JNIEnv* env,
		jobject dengineActivity,
		jdouble backendPtr,
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
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_DEngineActivity_nativeOnNewOrientation(
		JNIEnv* env,
		jobject thiz,
		jdouble backendPtr,
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
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_DEngineActivity_nativeOnFontScale(
		JNIEnv* env,
		jobject thiz,
		jdouble backendPtr,
		jint windowId,
		jfloat newScale)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;
}