#pragma once

#include <DEngine/Platform/Platform.hpp>
#include <DEngine/Platform/PlatformImpl.hpp>

#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/input.h>
#include <android/native_window.h>
#include <jni.h>

#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/Containers/FnScratchList.hpp>

#include <thread>
#include <vector>
#include <functional>

extern int dengine_impl_main(int argc, char const* const* argv);

namespace DEngine::Platform::impl::Backend
{
	constexpr bool logEvents = true;

    // Every window holds a pointer to an instance of this struct in their extra data
    // area.
    struct PerWindowData : public WindowPlatformBackendBase {
        ANativeWindow* aNativeWindow = nullptr;
        [[nodiscard]] void* GetRawHandle() override { return aNativeWindow; }
        [[nodiscard]] void const* GetRawHandle() const override { return aNativeWindow; }
    };

	struct BackendData;

	[[nodiscard]] inline BackendData& GetBackendData(Context& ctx) noexcept {
		return (BackendData&)ctx.GetImplData().GetBackendData();
	}
	[[nodiscard]] inline BackendData const& GetBackendData(Context const& ctx) noexcept {
		return (BackendData const&)ctx.GetImplData().GetBackendData();
	}
	[[nodiscard]] inline BackendData& GetBackendData(Context::Impl& implData) noexcept {
		return (BackendData&)implData.GetBackendData();
	}
	[[nodiscard]] inline BackendData const& GetBackendData(Context::Impl const& implData) noexcept {
		return (BackendData const&)implData.GetBackendData();
	}
	[[nodiscard]] inline BackendData& GetBackendData(long long in) noexcept {
		BackendData* ptr = nullptr;
		memcpy(&ptr, &in, sizeof(ptr));
		return *ptr;
	}

	void WriteToLogcat(LogSeverity severity, Std::Span<char const> const& msg);

	enum class LooperIdentifier {
		InputQueue,
		CustomEvent,
	};
	enum class CustomEventType {
		OnResume,
		OnPause,
		ConfiguationChanged,
		TextInput,
		EndTextInputSession,
		NativeWindowCreated,
		NativeWindowResized,
		NativeWindowDestroyed,
		InputQueueCreated,
		InputQueueDestroyed,
		ApplyWindowInsets,
		DeviceOrientation,
		Unknown,
	};

	using CustomEvent_CallbackFnT = Std::FnRef<void(CustomEventType)>;

	struct CustomEvent {
		CustomEventType type;
		std::function<void(Context::Impl&, BackendData&)> fn;
	};

	struct PollSource {
		Context::Impl* implData = nullptr;
		BackendData* backendData = nullptr;
		Std::Opt<CustomEvent_CallbackFnT> customEvent_CallbackFnOpt;
	};

	struct JniMethodIds {
		jmethodID _getWindowInsets_Id = nullptr;
		static constexpr char const* _getWindowInsets_Name = "getWindowInsets";
		static constexpr char const* getWindowInsets_Signature =
			"("
			"Ldidgy/dengine/DEngineActivity;"
			")I]";
		[[nodiscard]] Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> getWindowInsets(
			JNIEnv* jniEnv,
			jclass nativeToAndroidClass,
			jobject dengineActivity) const
		{
			DENGINE_IMPL_APPLICATION_ASSERT(_openSoftInput != nullptr);
			auto obj = (jintArray)jniEnv->CallStaticObjectMethod(
				nativeToAndroidClass,
				_getWindowInsets_Id,
				dengineActivity);

			Std::Array<int, 8> tempArray = {};
			jniEnv->GetIntArrayRegion(obj, 0, 8, tempArray.Data());
			jniEnv->DeleteLocalRef(obj);

			Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> output = {};
			auto& systemBarInsets = output[(int)WindowInsetSource::SystemBars];
			systemBarInsets.left = tempArray[0];
			systemBarInsets.right = tempArray[1];
			systemBarInsets.top = tempArray[2];
			systemBarInsets.bottom = tempArray[3];

			return output;
		}

		jmethodID _openSoftInput = nullptr;
		static constexpr char const* openSoftInput_Name = "openSoftInput";
		static constexpr char const* openSoftInput_Signature = "("
		   "Ldidgy/dengine/DEngineActivity;"
		   "Ljava/lang/String;"
		   "I" // selectionStart
		   "I" // selectionCount
		   "I"
		   ")V";
		void openSoftInput(
			JNIEnv* jniEnv,
			jclass nativeToAndroidClass,
			jobject dengineActivity,
			jstring text,
			jint selectionStart,
			jint selectionCount,
			SoftInputFilter inputFilter) const
		{
			DENGINE_IMPL_APPLICATION_ASSERT(_openSoftInput != nullptr);
			jniEnv->CallStaticVoidMethod(
				nativeToAndroidClass,
				_openSoftInput,
				dengineActivity,
				text,
				selectionStart,
				selectionCount,
				(jint)inputFilter);
		}
		jmethodID hideSoftInput = nullptr;

		jmethodID _updateInputConnection = nullptr;
		static constexpr char const* updateInputConnection_Name = "updateInputConnection";
		static constexpr char const* updateInputConnection_Signature =
			"("
			"Ldidgy/dengine/DEngineApp;"
			"Ldidgy/dengine/DEngineActivity;"
			"J" // windowId
			"I" // selectionStart
			"I" // selectionCount
			"[B" // textData
			")Z"; // boolean return
		bool UpdateInputConnection(
			JNIEnv& jniEnv,
			jclass nativeToAndroidClass,
			jobject dengineApp,
			jobject activity,
			jlong windowId,
			jint selectionStart,
			jint selectionCount,
			jbyteArray textData) const
		{
			DENGINE_IMPL_APPLICATION_ASSERT(_updateInputConnection != nullptr);
			return jniEnv.CallStaticBooleanMethod(
				nativeToAndroidClass,
				_updateInputConnection,
				dengineApp,
				activity,
				windowId,
				selectionStart,
				selectionCount,
				textData);
		}


		jmethodID _accessibilityUpdate = nullptr;
		static constexpr char const* accessibilityUpdate_Name = "accessibilityUpdate";
		static constexpr char const* accessibilityUpdate_Signature =
			"("
			"Ldidgy/dengine/DEngineApp;"
			"Ldidgy/dengine/DEngineActivity;"
			"J" // windowId
			"[J" // widgetIds
			"[I" // posX
			"[I" // posY
			"[I" // width
			"[I" // height
			"[Z" // clickable
			"[I" // textStart
			"[I" // textCount
			"[B" // textData
			")V"; // Void return
		void AccessibilityUpdate(
			JNIEnv* jniEnv,
			jclass nativeToAndroidClass,
			jobject dengineApp,
			jobject activity,
			jlong windowId,
			jlongArray widgetIds,
			jintArray posX,
			jintArray posY,
			jintArray width,
			jintArray height,
			jbooleanArray clickable,
			jintArray textStart,
			jintArray textCount,
			jbyteArray textData) const
		{
			DENGINE_IMPL_APPLICATION_ASSERT(_accessibilityUpdate != nullptr);
			jniEnv->CallStaticVoidMethod(
				nativeToAndroidClass,
				_accessibilityUpdate,
				dengineApp,
				activity,
				windowId,
				widgetIds,
				posX,
				posY,
				width,
				height,
				clickable,
				textStart,
				textCount,
				textData);
		}

	};

    struct BackendData : public Context::Impl::PlatformBackendBase {
		// Decided against using this because the function is one-per-vkinstance
		//PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurface = nullptr;
		// Cached to improve speed during the CreateVkSurface function.
		void* vkGetInstanceProcAddrFn = nullptr;

		// The Linux event file descriptor used for our custom events that
		// are not exposed by regular Android NDK.
		// Custom events will signal this file descriptor, and we will
		// poll the events with the ALooper.
		int customEventFd = 0;

		PollSource pollSource = {};

		std::thread gameThread;

		// This is the ID of our current window
		// TODO: Needs to be reworked in the future if we are to support multiple windows.
		Std::Opt<WindowID> currWindowId;


		// This is the native handle to our current window
		// associated with the currWindowId
		ANativeWindow* nativeWindow = nullptr;
		ALooper* gameThreadAndroidLooper = nullptr;

		AInputQueue* inputQueue = nullptr;

		// TODO: These values are per window. And they are only ever read/written to by the native
		// game thread. The Java thread will post jobs to update these. In the future this
		// should be reworked to support multiple windows.
		//Math::Vec2UInt visibleAreaOffset;
		//Extent visibleAreaExtent;
		Std::Array<WindowInsets, (int)WindowInsetSource::COUNT> windowInsets = {};
		Orientation currentOrientation;
		float fontScale = 1.f;
		int touchScrollSlopPx = 0;
		int scrollBarWidthPx = 0;


		JavaVM* globalJavaVm = nullptr;
		// This is our main DEngineApp
		jobject appHandle = nullptr;
		// This is our main DEngineApp class object
		jclass appClass = nullptr;
		// Initialized at startup and never changed.
		// This holds the class-object for the NativeToAndroid class.
		jclass nativeToAndroidClass = nullptr;

		// This is initialized to be a global Java ref
		// TODO: Might need to be reworked for multi-window support.
		jobject mainActivity = nullptr;
		AAssetManager* assetManager = nullptr;

		// This should generally never be nullptr
		AConfiguration* currAConfig = nullptr;
		// JNI Envs are per-thread. This is for the game thread, it is created when attaching
		// the thread to the JavaVM.
		JNIEnv* gameThreadJniEnv = nullptr;
		JniMethodIds jniMethodIds = {};

		std::vector<u32> customEvent_textInputs;
		std::mutex customEventQueueLock;
		std::vector<CustomEvent> customEventQueue;
	};

	// This should only be used for init.
	extern BackendData* pBackendDataInit;
}