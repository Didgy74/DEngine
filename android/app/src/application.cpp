#include <DEngine/impl/Application.hpp>
#include <DEngine/impl/AppAssert.hpp>

#include "BackendData.hpp"
#include "HandleCustomEvent.hpp"
#include "HandleInputEvent.hpp"

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Std/Containers/Defer.hpp>

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

	/*
	 * 	CUSTOM EVENTS
	 *
	 *	The following functions are our custom events. Some of them are in place of
	 *	the ones exposed by the NativeActivity interface because not all of those
	 *	work anymore because we override the active View in the Java code.
	 *	Also there are extra ones for the functionality we need that is not exposed in
	 *	ANativeActivity.
	 */

	// Pushes custom event to queue and increment the event file-descriptor.
	// Must be called from the Java thread.
	static void PushCustomEvent_Insert(
		BackendData& backendData,
		Std::Span<CustomEvent const> events)
	{
		std::lock_guard _{ backendData.customEventQueueLock };

		auto& eventQueue = backendData.customEventQueue;
		for (auto const& item : events) {
			eventQueue.push_back(item);
		}

		auto fdIncrement = (eventfd_t)events.Size();
		eventfd_write(backendData.customEventFd, fdIncrement);
	}
	static void PushCustomEvent_Insert(BackendData& backendData, CustomEvent const& event) {
		PushCustomEvent_Insert(backendData, { &event, 1 });
	}

	template<class Callable>
	static void PushCustomEvent_Insert2_Inner(
		BackendData& backendData,
		CustomEvent::Type eventType,
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
		CustomEvent::Type eventType,
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

			if (backendData.currentWindow.Has()) {

				auto windowId = backendData.currentWindow.Get();

				auto* windowNodePtr = implData.GetWindowNode(windowId);
				DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr != nullptr);
				auto& windowNode = *windowNodePtr;
				DENGINE_IMPL_APPLICATION_ASSERT(windowNode.platformHandle == nullptr);
				windowNode.platformHandle = window;

				BackendInterface::PushWindowMinimizeSignal(
					implData,
					backendData.currentWindow.Get(),
					false);
			}
		};
		PushCustomEvent_Insert2(
			backendData,
			CustomEvent::Type::NativeWindowCreated,
			job);
	}
	static void PushCustomEvent_NativeWindowResized(BackendData& backendData, ANativeWindow* window)
	{
		// We don't actually care
	}
	static void PushCustomEvent_NativeWindowDestroyed(BackendData& backendData, ANativeWindow* window)
	{
		auto job = [=](Context::Impl& implData, BackendData& backendData) {

			DENGINE_IMPL_APPLICATION_ASSERT(backendData.nativeWindow == window);
			backendData.nativeWindow = nullptr;

			if (backendData.currentWindow.Has()) {

				auto windowId = backendData.currentWindow.Get();

				auto* windowNodePtr = implData.GetWindowNode(windowId);
				DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr != nullptr);
				auto& windowNode = *windowNodePtr;
				DENGINE_IMPL_APPLICATION_ASSERT(windowNode.platformHandle != nullptr);
				windowNode.platformHandle = nullptr;

				BackendInterface::PushWindowMinimizeSignal(
						implData,
						backendData.currentWindow.Get(),
						true);
			}
		};
		PushCustomEvent_Insert2(
			backendData,
			CustomEvent::Type::NativeWindowDestroyed,
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
			CustomEvent::Type::InputQueueCreated,
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
			CustomEvent::Type::InputQueueDestroyed,
			job);
	}
	static void PushCustomEvent_DeviceOrientation(BackendData& backendData, int aOrient)
	{
		// For now we don't care
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

			if (backendData.currentWindow.HasValue())
			{
				auto windowWidth = (uint32_t)ANativeWindow_getWidth(backendData.nativeWindow);
				auto windowHeight = (uint32_t)ANativeWindow_getHeight(backendData.nativeWindow);
				BackendInterface::UpdateWindowSize(
					implData,
					backendData.currentWindow.Get(),
					{ windowWidth, windowHeight },
					posOffset.x,
					posOffset.y,
					extent);
			}
		};
		PushCustomEvent_Insert2(
			backendData,
			CustomEvent::Type::ContentRectChanged,
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
					backendData.currentWindow.Get(),
					job.oldStart,
					job.oldCount,
					replacementText);
			};

			PushCustomEvent_Insert2_Inner(
				backendData,
				CustomEvent::Type::TextInput,
				temp);
		}

		eventfd_t fdIncrement = inputJobs.Size();
		eventfd_write(backendData.customEventFd, fdIncrement);
	}

	void PushCustomEvent_EndTextInputSession(BackendData& backendData) {
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			if (backendData.currentWindow.Has()) {
				impl::BackendInterface::PushEndTextInputSessionEvent(
					implData,
					backendData.currentWindow.Get());
			}
		};
		PushCustomEvent_Insert2(
			backendData,
			CustomEvent::Type::EndTextInputSession,
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
		//DENGINE_IMPL_APPLICATION_ASSERT(detail::pBackendData);
	}

	static void onDestroy(ANativeActivity* activity)
	{
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

		// And we create our file descriptor for our custom events
		// We need to create this right away because
		// our first events (and need to signal this) will happen before the other thread
		// can initialize something like this.
		backendData.customEventFd = eventfd(0, EFD_SEMAPHORE);

		// Now we run our apps int main-equivalent on a new thread.
		// The remaining part of initialization is handled by that thread.
		auto lambda = []() {
			DENGINE_APP_MAIN_ENTRYPOINT(0, nullptr);
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
	jobject dengineActivity)
{
	using namespace DEngine;
	using namespace DEngine::Application;
	using namespace DEngine::Application::impl;

	double returnValue = {};
	auto* backendPtr = pBackendData;

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

namespace DEngine::Application::impl
{
	// Can only be run from the main native thread
	static void RunEventPolling(
		Context::Impl& implData,
		BackendData& backendData,
		bool waitForEvents,
		u64 timeoutNs,
		Std::Opt<CustomEvent_CallbackFnT> callback = Std::nullOpt)
	{
		DENGINE_IMPL_APPLICATION_ASSERT(
			std::this_thread::get_id() == backendData.gameThread.get_id());

		// Not implemented yet.
		if (waitForEvents && timeoutNs != 0)
			DENGINE_IMPL_APPLICATION_UNREACHABLE();

		// All our callbacks contain a pointer to this pollSource member.
		// We populate it with pointers back to our internal structures before polling
		// and then nullify it when we're done polling.
		// It also holds any additional arguments we want to send to our
		// event callbacks.
		auto& pollSource = backendData.pollSource;
		pollSource.backendData = &backendData;
		pollSource.implData = &implData;
		pollSource.customEvent_CallbackFnOpt = callback;
		Std::Defer cleanup = [&]() {
			pollSource = {};
		};

		// These polling functions have parameters we never use, made some quick lambdas
		// to wrap them.
		auto pollOnceWrapper = [&](int timeout) {
			return ALooper_pollOnce(timeout,nullptr,nullptr,nullptr);
		};
		auto pollAllWrapper = [&](int timeout) {
			return ALooper_pollAll(timeout,nullptr,nullptr,nullptr);
		};

		if (waitForEvents && timeoutNs == 0) {
			/*
			 * 	There's a problem when waiting indefinitely for
			 * 	the pollAll function where it will not return
			 * 	no matter how many events happen.
			 * 	But the pollOnce function has an issue where it will only
			 * 	trigger one event even if multiple were pushed
			 * 	simultaneously.
			 *
			 * 	So we first check if any events happened since the last poll,
			 *	if not then we start waiting indefinitely until any events happen.
			 */

			// First poll all and return ASAP
			// Argument 0 means to return as fast as possible
			// Ref: Android NDK ALooper
			int initialPollResult = pollAllWrapper(0);
			if (initialPollResult == ALOOPER_POLL_TIMEOUT) {
				// This means we found no events, now
				// we want to wait indefinitely, however in some edge cases the
				// poller might wake up prematurely. Such as when placing
				// breakpoints in this code. Hence the loop.
				// We only break out of the loop if the result
				// tells us we actually triggered an event.
				bool continuePolling = true;
				while (continuePolling) {
					// -1 (negative number) means to wait indefinitely
					// Ref: Android NDK ALooper
					int pollResult = pollOnceWrapper(-1);
					if (pollResult == ALOOPER_POLL_CALLBACK)
						continuePolling = false;
				}
				// The pollOnce call only catches a single event no matter what.
				// We call pollAll a final time in case multiple events were fired
				// simultaneously by Android.
				pollAllWrapper(0);
			}

		} else {
			// Just run all pending events and
			// return immediately.
			pollAllWrapper(0);
		}
	}

	static void WaitForInitialRequiredEvents(
		Context::Impl& implData,
		BackendData& backendData)
	{
		bool nativeWindowSet = false;
		bool visibleAreaSet = false;
		while (!nativeWindowSet || !visibleAreaSet)
		{
			auto callback =
				[&](CustomEvent::Type type) {
				switch (type) {
					case CustomEvent::Type::NativeWindowCreated:
						nativeWindowSet = true;
						break;
					case CustomEvent::Type::ContentRectChanged:
						visibleAreaSet = true;
						break;
					default:
						break;
				}
			};

			CustomEvent_CallbackFnT fnRef = callback;

			RunEventPolling(
				implData,
				backendData,
				true,
				0,
				fnRef);
		}
	}

	JniMethodIds LoadJavaMethodIds(JNIEnv* jniEnv, jobject mainActivity) {
		JniMethodIds returnValue = {};

		jclass classObject = jniEnv->GetObjectClass(mainActivity);

		returnValue.openSoftInput = jniEnv->GetMethodID(
				classObject,
				"nativeEvent_openSoftInput",
				"(Ljava/lang/String;I)V");
		returnValue.hideSoftInput = jniEnv->GetMethodID(
				classObject,
				"nativeEvent_hideSoftInput",
				"()V");

		return returnValue;
	}
}

void* Application::impl::Backend::Initialize(
	Context& ctx,
	Context::Impl& implData)
{
	auto& backendData = *pBackendData;

	// First we attach this thread to the JVM,
	// this gives us the JniEnv handle for this thread and
	// allows this thread to do Java/JNI stuff.
	JNIEnv* jniEnv = nullptr;
	jint attachThreadResult = backendData.nativeActivity->vm->AttachCurrentThread(
		&jniEnv,
		nullptr);
	if (attachThreadResult != JNI_OK)
	{
		// Attaching failed. Crash the program. Can't really handle this,
		// we should probably use a more elegant method of crashing though.
		std::abort();
	}
	backendData.gameThreadJniEnv = jniEnv;

	backendData.jniMethodIds = LoadJavaMethodIds(jniEnv, backendData.mainActivity);

	// Load the ALooper object for this thread.
	// We don't want to use the ALOOPER_PREPARE_ALLOW_NON_CALLBACKS flag,
	// all our events are callback based.
	backendData.gameThreadAndroidLooper = ALooper_prepare(0);

	// Add the event-file-descriptor for our custom events to this looper.
	ALooper_addFd(
		backendData.gameThreadAndroidLooper,
		backendData.customEventFd,
		(int)LooperIdentifier::CustomEvent,
		ALOOPER_EVENT_INPUT,
		&looperCallback_CustomEvent,
		&backendData.pollSource);

	// We need to wait for a few stuff, like the input queue and our native window
	// before we can really do anything. Probably not an ideal solution.
	WaitForInitialRequiredEvents(implData, backendData);

	return pBackendData;
}

void Application::impl::Backend::ProcessEvents(
	Context& ctx,
	Context::Impl& implData,
	void* pBackendDataIn,
	bool waitForEvents,
	u64 timeoutNs)
{
	auto& backendData = *static_cast<BackendData*>(pBackendDataIn);

	RunEventPolling(
		implData,
		backendData,
		waitForEvents,
		timeoutNs);
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

Std::StackVec<char const*, 5> DEngine::Application::GetRequiredVkInstanceExtensions() noexcept
{
	return {
		"VK_KHR_surface",
		"VK_KHR_android_surface"
	};
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

	// We have to be in the correct thread
	DENGINE_IMPL_APPLICATION_ASSERT(std::this_thread::get_id() == backendData.gameThread.get_id());

	std::vector<jchar> tempString;
	tempString.reserve(inputString.Size());
	for (uSize i = 0; i < inputString.Size(); i++) {
		tempString.push_back((jchar)inputString[i]);
	}
	jstring javaString = backendData.gameThreadJniEnv->NewString(tempString.data(), tempString.size());
	backendData.gameThreadJniEnv->CallVoidMethod(
		backendData.mainActivity,
		backendData.jniMethodIds.openSoftInput,
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
		backendData.jniMethodIds.hideSoftInput);
}