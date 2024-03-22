#pragma once

#include <DEngine/impl/Application.hpp>

#include <android/asset_manager.h>
#include <android/configuration.h>
#include <android/input.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <jni.h>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Std/Containers/FnScratchList.hpp>

#include <thread>
#include <vector>
#include <functional>

extern int dengine_impl_main(int argc, char** argv);

namespace DEngine::Application::impl
{
	constexpr bool logEvents = true;

	struct BackendData;

	[[nodiscard]] inline BackendData& GetBackendData(Context& ctx) noexcept {
		return *(BackendData*)ctx.GetImplData().backendData;
	}
	[[nodiscard]] inline BackendData const& GetBackendData(Context const& ctx) noexcept {
		return *(BackendData const*)ctx.GetImplData().backendData;
	}
	[[nodiscard]] inline BackendData& GetBackendData(Context::Impl& implData) noexcept {
		return *(BackendData*)implData.backendData;
	}
	[[nodiscard]] inline BackendData const& GetBackendData(Context::Impl const& implData) noexcept {
		return *(BackendData const*)implData.backendData;
	}
	[[nodiscard]] inline BackendData& GetBackendData(ANativeActivity* activity) noexcept {
		return *(BackendData*)activity->instance;
	}
	[[nodiscard]] inline BackendData const& GetBackendData(ANativeActivity const* activity) noexcept {
		return *(BackendData const*)activity->instance;
	}
	[[nodiscard]] inline BackendData& GetBackendData(long long in) noexcept {
		BackendData* ptr = nullptr;
		memcpy(&ptr, &in, sizeof(ptr));
		return *ptr;
	}

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
		ContentRectChanged,
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
		jmethodID updateAccessibility = nullptr;
		jmethodID openSoftInput = nullptr;
		jmethodID hideSoftInput = nullptr;
	};

	struct BackendData {
		// Decided against using this because the function is one-per-vkinstance
		//PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurface = nullptr;
		// Cached to improve speed during the CreateVkSurface function.
		void* vkGetInstanceProcAddrFn = nullptr;

		// The Linux event file descriptor used for our custom events that
		// are not exposed by regular Android NDK.
		// Custom events will signal this file descriptor, and we will
		// poll the events with the ALooper.
		int customEventFd = 0;

		// I think this is owned by the
		// Java thread, by default we should not touch its members directly?
		ANativeActivity* nativeActivity = nullptr;
		PollSource pollSource = {};

		std::thread gameThread;

		// This is the ID of our current window
		Std::Opt<WindowID> currWindowId;
		// This is the native handle to our current window
		// associated with the currWindowId
		ANativeWindow* nativeWindow = nullptr;
		ALooper* gameThreadAndroidLooper = nullptr;

		AInputQueue* inputQueue = nullptr;
		Math::Vec2UInt visibleAreaOffset;
		Extent visibleAreaExtent;
		Orientation currentOrientation;
		float fontScale = 1.f;


		JavaVM* globalJavaVm = nullptr;
		// This is our main DEngineApp
		jobject appHandle = nullptr;
		// This is our main DEngineApp class object
		jclass appClass = nullptr;

		// This is initialized to be a global Java ref
		jobject mainActivity = nullptr;
		AAssetManager* assetManager = nullptr;
		// This should generally never be nullptr?
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