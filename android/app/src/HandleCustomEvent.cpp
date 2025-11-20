#include <DEngine/Platform/PlatformImpl.hpp>

#include "HandleCustomEvent.hpp"

#include "BackendData.hpp"
#include "HandleInputEvent.hpp"

#include <DEngine/Platform/PlatformAssert.hpp>

#include <android/log.h>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>

#include <sys/eventfd.h>

#include <condition_variable>
#include <mutex>

namespace DEngine::Platform::impl::Backend
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
using namespace DEngine::Platform;
using namespace DEngine::Platform::impl;
namespace Backend = DEngine::Platform::impl::Backend;

int Backend::looperCallback_CustomEvent(int fd, int events, void* data)
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

namespace DEngine::Platform::impl::Backend {
    /*
     * 	CUSTOM EVENTS
     *
     *	The following functions are our custom events.
     *	All of these run on the game-thread whenever it polls for events on the ALooper object.
     *
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
		auto job = [window](Context::Impl& implData, BackendData& backendData) {
			DENGINE_IMPL_APPLICATION_ASSERT(backendData.nativeWindow == nullptr);
			backendData.nativeWindow = window;

			if (backendData.currWindowId.Has()) {
				auto windowId = backendData.currWindowId.Get();

				auto* windowNodePtr = implData.GetWindowNode(windowId);
				DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr != nullptr);
				auto& windowNode = *windowNodePtr;
                auto* windowNodeBackendPtr = (PerWindowData*)windowNode.backendData.Get();
                DENGINE_IMPL_APPLICATION_ASSERT(windowNodeBackendPtr != nullptr);
                auto& windowNodeBackend = *windowNodeBackendPtr;
				DENGINE_IMPL_APPLICATION_ASSERT(windowNodeBackend.aNativeWindow == nullptr);
				windowNodeBackend.aNativeWindow = window;

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
		auto job = [window](Context::Impl& implData, BackendData& backendData) {
			//DENGINE_IMPL_APPLICATION_ASSERT(backendData.nativeWindow == window);
			ANativeWindow_release(backendData.nativeWindow);
			backendData.nativeWindow = nullptr;

			if (backendData.currWindowId.Has()) {
				auto windowId = backendData.currWindowId.Get();

				auto* windowNodePtr = implData.GetWindowNode(windowId);
				DENGINE_IMPL_APPLICATION_ASSERT(windowNodePtr != nullptr);
				auto& windowNode = *windowNodePtr;
                auto* windowNodeBackendPtr = (PerWindowData*)windowNode.backendData.Get();
                DENGINE_IMPL_APPLICATION_ASSERT(windowNodeBackendPtr != nullptr);
                auto& windowNodeBackend = *windowNodeBackendPtr;
				DENGINE_IMPL_APPLICATION_ASSERT(windowNodeBackend.aNativeWindow != nullptr);
                windowNodeBackend.aNativeWindow = nullptr;

				BackendInterface::PushWindowMinimizeSignal(implData, windowId,true);
			}
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::NativeWindowDestroyed,
			Std::Move(job));
	}
	template<class Callable>
	static void RunOnGameThread_Blocking(
		BackendData& backendData,
		CustomEventType eventType,
		Callable&& work)
	{
		std::mutex doneMutex;
		std::condition_variable doneCondVar;
		bool done = false;

		auto job = [&](Context::Impl& implData, BackendData& backendData) {
			work(implData, backendData);
			{
				std::lock_guard lock { doneMutex };
				done = true;
			}
			doneCondVar.notify_one();
		};
		PushCustomEvent_Insert(backendData, eventType, Std::Move(job));

		std::unique_lock lock { doneMutex };
		doneCondVar.wait(lock, [&]{ return done; });
	}

	// Attach is asynchronous: there's no use-after-free risk on creation because the
	// Java InputQueue stays alive until onInputQueueDestroyed, and the custom-event
	// queue's FIFO ordering guarantees this attach runs before any later detach.
	static void PushCustomEvent_InputQueueCreated(BackendData& backendData, AInputQueue* queue)
	{
		auto job = [queue](Context::Impl& implData, BackendData& backendData) {
			DENGINE_IMPL_APPLICATION_ASSERT(backendData.gameThreadAndroidLooper != nullptr);
			DENGINE_IMPL_APPLICATION_ASSERT(backendData.inputQueue == nullptr);

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
	// Detach is synchronous so the queue is guaranteed off our looper before the
	// framework disposes it. Detaches whatever we currently hold rather than a
	// passed-in handle, so it's robust even if the matching attach never ran.
	static void PushCustomEvent_InputQueueDestroyed(BackendData& backendData)
	{
		RunOnGameThread_Blocking(
			backendData,
			CustomEventType::InputQueueDestroyed,
			[](Context::Impl& implData, BackendData& backendData) {
				DENGINE_IMPL_APPLICATION_ASSERT(backendData.gameThreadAndroidLooper != nullptr);
				if (backendData.inputQueue != nullptr) {
					AInputQueue_detachLooper(backendData.inputQueue);
					backendData.inputQueue = nullptr;
				}
			});
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
	void PushCustomEvent_onApplyWindowInsets(
		BackendData& backendData,
		Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> incomingInsets)
	{
		auto job = [=](Context::Impl& implData, BackendData& backendData) {
			// This lambda runs on main game thread during event polling

			//backendData.visibleAreaOffset = posOffset;
			//backendData.visibleAreaExtent = extent;
			backendData.windowInsets = incomingInsets;

			if (backendData.currWindowId.HasValue()) {
				// I don't really know what to do if the native window handle is null
				DENGINE_IMPL_APPLICATION_ASSERT(backendData.nativeWindow);

				auto windowWidth = (uint32_t)ANativeWindow_getWidth(backendData.nativeWindow);
				auto windowHeight = (uint32_t)ANativeWindow_getHeight(backendData.nativeWindow);
				BackendInterface::UpdateWindowSize(
					implData,
					backendData.currWindowId.Get(),
					{ windowWidth, windowHeight },
					incomingInsets);
			}
		};
		PushCustomEvent_Insert(
			backendData,
			CustomEventType::ApplyWindowInsets,
			Std::Move(job));
	}

	struct TextInputJob {
		enum class JobType {
			Invalid,
			ReplaceText,
			SetSelection
		};
		JobType jobType = JobType::Invalid;

		struct ReplaceTextJob {
			uSize oldStart;
			uSize oldCount;
			uSize textOffset;
			uSize textCount;
		};
		ReplaceTextJob replaceTextJob = {};
		struct SetSelectionJob {
			uSize start;
			uSize count;
		};
		SetSelectionJob setSelectionJob = {};
	};
	// This takes an array of events because multiple input jobs can happen
	// at the same timestamp and be submitted together
	void PushCustomEvent_TextInput(
		BackendData& backendData,
		Std::Span<TextInputJob const> inputJobs,
		Std::Span<u32 const> inputText)
	{
		// Acquire the lock early because we need to know the text-buffer size.
		// Also we need to lock because we want to batch these commands and the backend-interface
		// doesn't support sending all the commands with a single event.
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
			if (job.jobType == TextInputJob::JobType::SetSelection) {
				auto temp = [job](Context::Impl& implData, BackendData& backendData) {
					impl::BackendInterface::PushTextSelectionEvent(
						implData,
						backendData.currWindowId.Get(),
						job.setSelectionJob.start,
						job.setSelectionJob.count);
				};
				PushCustomEvent_Insert(
					backendData,
					CustomEventType::TextInput,
					Std::Move(temp),
					PushCustomEvent_UseLock::No);

			} else if (job.jobType == TextInputJob::JobType::ReplaceText) {
				auto textInputStartIndex = oldTextBufferLen + job.replaceTextJob.textOffset;
				auto temp = [job, textInputStartIndex](Context::Impl& implData, BackendData& backendData) {
					auto& textBuffer = backendData.customEvent_textInputs;
					DENGINE_IMPL_APPLICATION_ASSERT(textInputStartIndex + job.replaceTextJob.textCount <= textBuffer.size());
					Std::Span replacementText = {
						textBuffer.data() + textInputStartIndex,
						job.replaceTextJob.textCount };
					DENGINE_IMPL_APPLICATION_ASSERT(job.replaceTextJob.oldCount != 0 || !replacementText.Empty());
					impl::BackendInterface::PushTextInputEvent(
						implData,
						backendData.currWindowId.Get(),
						job.replaceTextJob.oldStart,
						job.replaceTextJob.oldCount,
						replacementText);
				};

				// We already locked the queue in this function scope, we don't want to lock it again.
				PushCustomEvent_Insert(
					backendData,
					CustomEventType::TextInput,
					Std::Move(temp),
					PushCustomEvent_UseLock::No);
			} else {
				DENGINE_IMPL_APPLICATION_UNREACHABLE();
			}
		}
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

	static void LogEvent(char const* msg)
	{
		__android_log_print(ANDROID_LOG_DEBUG, "DEngine - Event", "%s", msg);
	}

	static void onNativeWindowCreated(BackendData& backendData, ANativeWindow* window) {
		if constexpr (logEvents)
			LogEvent("Native window created");
		PushCustomEvent_NativeWindowCreated(backendData, window);
	}

	[[nodiscard]] BackendData* InitBackendData(
		JNIEnv* env,
        jobject app,
		jclass appClass,
		jclass nativeToAndroidClass,
		jobject activity,
		jobject assetManager,
		float fontScale,
		int touchScrollSlopPx,
		int scrollBarWidthPx)
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
		backendData.nativeToAndroidClass = (jclass)env->NewGlobalRef((jobject)nativeToAndroidClass);
		backendData.mainActivity = env->NewGlobalRef(activity);
		backendData.assetManager = AAssetManager_fromJava(env, assetManager);
		backendData.currAConfig = AConfiguration_new();
		AConfiguration_fromAssetManager(backendData.currAConfig, backendData.assetManager);

		backendData.fontScale = fontScale;
		backendData.touchScrollSlopPx = touchScrollSlopPx;
		backendData.scrollBarWidthPx = scrollBarWidthPx;

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
}

// Called at the end of onCreate in Java.
extern "C"
JNIEXPORT jlong
Java_didgy_dengine_ToNative_init(
	JNIEnv* env,
	jclass clazz,
	jobject app,
	jclass appClass,
	jclass nativeToAndroidClass,
	jobject activity,
	jobject assetManager,
	jfloat fontScale,
	jint touchScrollSlopPx,
	jint scrollBarWidthPx)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;

	auto backendPtr = Backend::InitBackendData(
		env,
        app,
		appClass,
		nativeToAndroidClass,
		activity,
		assetManager,
		fontScale,
		touchScrollSlopPx,
		scrollBarWidthPx);
	Backend::pBackendDataInit = backendPtr;

	jlong returnValue = {};

	// We need to make sure we can fit this pointer in the return value.
	static_assert(sizeof(returnValue) >= sizeof(backendPtr));
	memcpy( &returnValue, &backendPtr, sizeof(backendPtr));
	return returnValue;
}

static std::vector<u32> jstringToStdVectorU32(JNIEnv* env, jstring inputString)
{
	// Copy the string
	jsize textLen = env->GetStringLength(inputString);

	std::vector<jchar> tempJniString;
	tempJniString.resize(textLen);
	env->GetStringRegion(inputString, 0, textLen, tempJniString.data());

	std::vector<u32> outString(textLen);
	for (int i = 0; i < textLen; i++) {
		DENGINE_IMPL_APPLICATION_ASSERT(tempJniString[i] != 0);
		outString[i] = (u32)tempJniString[i];
	}
	return outString;
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_ToNative_onTextInput(
	JNIEnv* env,
	jclass clazz,
	jlong backendPtr,
	jintArray info_eventType,
	jintArray info_textReplaceOldStart,
	jintArray info_textReplaceOldCount,
	jintArray info_textReplaceTextOffset,
	jintArray info_textReplaceTextCount,
	jintArray info_setSelectionSelStart,
	jintArray info_setSelectionSelCount,
	jstring allStrings)
{
	using namespace DEngine;
	using namespace DEngine::Platform;

	auto& backendData = Backend::GetBackendData(backendPtr);

	// Copy the string
	auto outString = jstringToStdVectorU32(env, allStrings);

	jsize infoLen = env->GetArrayLength(info_eventType);
	DENGINE_IMPL_APPLICATION_ASSERT(infoLen > 0);
	auto convert = [=](jintArray infos) {
		std::vector<jint> tempInts(infoLen);
		env->GetIntArrayRegion(infos, 0, infoLen, tempInts.data());
		return tempInts;
	};
	auto eventTypes = convert(info_eventType);
	auto textReplaceOldStarts = convert(info_textReplaceOldStart);
	auto textReplaceOldCounts = convert(info_textReplaceOldCount);
	auto textReplaceTextOffsets = convert(info_textReplaceTextOffset);
	auto textReplaceTextCounts = convert(info_textReplaceTextCount);
	auto setSelectionStarts = convert(info_setSelectionSelStart);
	auto setSelectionCounts = convert(info_setSelectionSelCount);

	std::vector<Backend::TextInputJob> inputJobs;
	inputJobs.reserve(infoLen);

	for (int i = 0; i < infoLen; i++) {
		if (eventTypes[i] == 0) {
			Backend::TextInputJob newJob = {};
			newJob.jobType = Backend::TextInputJob::JobType::ReplaceText;
			newJob.replaceTextJob.oldStart = textReplaceOldStarts[i];
			newJob.replaceTextJob.oldCount = textReplaceOldCounts[i];
			newJob.replaceTextJob.textOffset = textReplaceTextOffsets[i];
			newJob.replaceTextJob.textCount = textReplaceTextCounts[i];
			DENGINE_IMPL_APPLICATION_ASSERT(
				!(newJob.replaceTextJob.oldCount == 0 && newJob.replaceTextJob.textCount == 0));
			DENGINE_IMPL_APPLICATION_ASSERT(
				newJob.replaceTextJob.textOffset + newJob.replaceTextJob.textCount <= outString.size());
			inputJobs.push_back(newJob);
		} else if (eventTypes[i] == 1) {
			Backend::TextInputJob newJob = {};
			newJob.jobType = Backend::TextInputJob::JobType::SetSelection;
			newJob.setSelectionJob.start = setSelectionStarts[i];
			newJob.setSelectionJob.count = setSelectionCounts[i];
			inputJobs.push_back(newJob);
		} else {
			DENGINE_IMPL_APPLICATION_UNREACHABLE();
		}
	}

	PushCustomEvent_TextInput(
		backendData,
		{ inputJobs.data(), inputJobs.size() },
		{ outString.data(), outString.size() });
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_ToNative_sendEventEndTextInputSession(
	JNIEnv* env,
	jclass object,
	jlong backendDataPtr)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;

	PushCustomEvent_EndTextInputSession(Backend::GetBackendData(backendDataPtr));
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL
Java_didgy_dengine_ToNative_onApplyWindowInsets(
	JNIEnv* env,
	jclass clazz,
	jlong backendPtr,
	jint systemBarLeft,
	jint systemBarRight,
	jint systemBarTop,
	jint systemBarBottom,
	jint imeLeft,
	jint imeRight,
	jint imeTop,
	jint imeBottom)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;

	Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> insets = {};
	auto& systemBarInsets = insets[(int)WindowInsetSource::SystemBars];
	systemBarInsets.left = systemBarLeft;
	systemBarInsets.right = systemBarRight;
	systemBarInsets.top = systemBarTop;
	systemBarInsets.bottom = systemBarBottom;

	auto& imeInsets = insets[(int)WindowInsetSource::TextInputSystem];
	imeInsets.left = imeLeft;
	imeInsets.right = imeRight;
	imeInsets.top = imeTop;
	imeInsets.bottom = imeBottom;

	PushCustomEvent_onApplyWindowInsets(
		Backend::GetBackendData(backendPtr),
		insets);
}

/*
 * A bug in the NDK makes AConfiguration_getOrientation never have the updated value,
 * so we have this workaround path.
 *
 * Possibly no configuration changes are recorded from this bug, didn't really test enough.
 * Could possibly be that AConfiguration_fromAssetManager doesn't work in general.
 */
extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_ToNative_onNewOrientation(
	JNIEnv* env,
	jclass thiz,
	jlong backendPtr,
	jint newOrientation)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;

	PushCustomEvent_DeviceOrientation(
			Backend::GetBackendData(backendPtr),
			newOrientation);
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_ToNative_onFontScale(
	JNIEnv* env,
	jclass thiz,
	jlong backendPtr,
	jlong windowId,
	jfloat newScale)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;
}

extern "C"
[[maybe_unused]] JNIEXPORT void JNICALL Java_didgy_dengine_ToNative_onNativeWindowCreated(
	JNIEnv* env,
	jclass thiz,
	jlong backendPtr,
	jobject surface)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;

	auto& backendData = Backend::GetBackendData(backendPtr);

	auto* nativeWindowHandle = ANativeWindow_fromSurface(env, surface);
	PushCustomEvent_NativeWindowCreated(backendData, nativeWindowHandle);
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_ToNative_onNativeWindowDestroyed(
	JNIEnv *env,
	jclass clazz,
	jlong backendPtr,
	jobject surface)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;

	auto& backendData = Backend::GetBackendData(backendPtr);

	auto* nativeWindowHandle = ANativeWindow_fromSurface(env, surface);
	PushCustomEvent_NativeWindowDestroyed(backendData, nativeWindowHandle);
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_ToNative_onInputQueueCreated(
	JNIEnv *env,
	jclass clazz,
	jlong backend_ptr,
	jobject activity,
	jlong windowId,
	jobject queueLocalRef)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;

	auto& backendData = Backend::GetBackendData(backend_ptr);

	// Resolve the native queue here on the Java thread (we have the JNIEnv). The
	// queue stays alive until onInputQueueDestroyed, so the pointer is safe to hand
	// to the game thread, which does the actual attach to its looper.
	AInputQueue* queue = AInputQueue_fromJava(env, queueLocalRef);
	Backend::PushCustomEvent_InputQueueCreated(backendData, queue);
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_ToNative_onInputQueueDestroyed(
	JNIEnv *env,
	jclass clazz,
	jlong backend_ptr,
	jobject activity,
	jlong windowId,
	jobject queueLocalRef)
{
	using namespace DEngine;
	using namespace DEngine::Platform;
	using namespace DEngine::Platform::impl;

	auto& backendData = Backend::GetBackendData(backend_ptr);

	// Blocks until the game thread has detached the queue from its looper, because
	// the framework disposes the queue the instant this function returns.
	Backend::PushCustomEvent_InputQueueDestroyed(backendData);
}
