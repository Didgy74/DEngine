#pragma once

#include <DEngine/impl/Application.hpp>

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <jni.h>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>
#include <DEngine/Std/BumpAllocator.hpp>

#include <thread>
#include <vector>

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
	[[nodiscard]] inline BackendData& GetBackendData(jdouble in) noexcept {
		BackendData* ptr = nullptr;
		memcpy(&ptr, &in, sizeof(ptr));
		return *ptr;
	}

	enum class LooperIdentifier {
		InputQueue,
		CustomEvent,
	};

	struct CustomEvent
	{
		enum class Type
		{
			OnResume,
			OnPause,
			TextInput,
			EndTextInputSession,
			NativeWindowCreated,
			NativeWindowResized,
			NativeWindowDestroyed,
			InputQueueCreated,
			InputQueueDestroyed,
			ContentRectChanged,
			DeviceOrientation,
		};
		Type type;

		template<Type>
		struct Data {};
		template<>
		struct Data<Type::NativeWindowCreated> {
			ANativeWindow* nativeWindow;
		};
		template<>
		struct Data<Type::NativeWindowResized> {
			ANativeWindow* nativeWindow;
		};
		template<>
		struct Data<Type::NativeWindowDestroyed> {
			ANativeWindow* nativeWindow;
		};
		template<>
		struct Data<Type::InputQueueCreated> {
			AInputQueue* inputQueue;
		};
		template<>
		struct Data<Type::InputQueueDestroyed> {
			AInputQueue* inputQueue;
		};
		template<>
		struct Data<Type::ContentRectChanged> {
			u32 offsetX;
			u32 offsetY;
			u32 width;
			u32 height;
		};
		template<>
		struct Data<Type::DeviceOrientation> {
			Orientation newOrientation;
		};
		template<>
		struct Data<Type::TextInput> {
			u32 start;
			uSize count;
			uSize textInputStartIndex;
			uSize textInputLength;
		};

		union Data_T
		{
			Data<Type::NativeWindowCreated> nativeWindowCreated;
			Data<Type::NativeWindowResized> nativeWindowResized;
			Data<Type::NativeWindowDestroyed> nativeWindowDestroyed;
			Data<Type::InputQueueCreated> inputQueueCreated;
			Data<Type::InputQueueDestroyed> inputQueueDestroyed;
			Data<Type::ContentRectChanged> contentRectChanged;
			Data<Type::DeviceOrientation> deviceOrientation;
			Data<Type::TextInput> textInput;
		};
		Data_T data;
	};

	using CustomEvent_CallbackFnT = Std::FnRef<void(CustomEvent::Type)>;

	struct CustomEvent2 {
		CustomEvent::Type type;
		Std::FnRef<void(Context::Impl&, BackendData&)> fn;
	};

	struct PollSource {
		Context::Impl* implData = nullptr;
		BackendData* backendData = nullptr;
		Std::Opt<CustomEvent_CallbackFnT> customEvent_CallbackFnOpt;
	};

	struct JniMethodIds {
		jmethodID openSoftInput = nullptr;
		jmethodID hideSoftInput = nullptr;
	};

	struct BackendData
	{
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

		Std::Opt<WindowID> currentWindow;
		ALooper* gameThreadAndroidLooper = nullptr;
		ANativeWindow* nativeWindow = nullptr;
		AInputQueue* inputQueue = nullptr;
		Math::Vec2UInt visibleAreaOffset;
		Extent visibleAreaExtent;
		Orientation currentOrientation;

		JavaVM* globalJavaVm = nullptr;
		// This is initialized to be a global Java ref
		jobject mainActivity = nullptr;
		AAssetManager* assetManager = nullptr;
		// JNI Envs are per-thread. This is for the game thread, it is created when attaching
		// the thread to the JavaVM.
		JNIEnv* gameThreadJniEnv = nullptr;
		JniMethodIds jniMethodIds = {};

		std::mutex customEventQueueLock;
		std::vector<CustomEvent> customEventQueue;
		std::vector<u32> customEvent_textInputs;
		std::vector<CustomEvent2> customEventQueue2;
		// ONLY USE WITH customEventQueue member!!!
		Std::FrameAlloc queuedEvents_InnerBuffer = Std::FrameAlloc::PreAllocate(1024).Get();

	};

	extern BackendData* pBackendData;
}