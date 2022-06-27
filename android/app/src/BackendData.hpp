#pragma once

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <jni.h>

#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>

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
			TextInput,
			EndTextInputSession,
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
		struct Data<Type::NativeWindowCreated> {
			ANativeWindow* nativeWindow;
		};
		template<>
		struct Data<Type::InputQueueCreated> {
			AInputQueue* inputQueue;
		};
		template<>
		struct Data<Type::VisibleAreaChanged> {
			u32 offsetX;
			u32 offsetY;
			u32 width;
			u32 height;
		};
		template<>
		struct Data<Type::NewOrientation> {
			uint8_t newOrientation;
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
			Data<Type::NativeWindowDestroyed> nativeWindowDestroyed;
			Data<Type::InputQueueCreated> inputQueueCreated;
			Data<Type::VisibleAreaChanged> visibleAreaChanged;
			Data<Type::NewOrientation> newOrientation;
			Data<Type::TextInput> textInput;
		};
		Data_T data;
	};

	using CustomEvent_CallbackFnT = Std::FnRef<void(CustomEvent const&)>;

	struct PollSource {
		Context::Impl* implData = nullptr;
		BackendData* backendData = nullptr;
		Std::Opt<CustomEvent_CallbackFnT> customEvent_CallbackFnOpt;
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

		ANativeActivity* nativeActivity = nullptr;
		PollSource pollSource = {};

		std::thread gameThread;

		Std::Opt<WindowID> currentWindow;
		ANativeWindow* nativeWindow = nullptr;
		AInputQueue* inputQueue = nullptr;
		Math::Vec2UInt visibleAreaOffset;
		Extent visibleAreaExtent;
		Orientation currentOrientation;

		JavaVM* globalJavaVm = nullptr;
		jobject mainActivity = nullptr;
		AAssetManager* assetManager = nullptr;
		// JNI Envs are per-thread. This is for the game thread, it is created when attaching
		// the thread to the JavaVM.
		JNIEnv* gameThreadJniEnv = nullptr;
		jmethodID jniOpenSoftInput = nullptr;
		jmethodID jniHideSoftInput = nullptr;
		std::mutex customEventQueueLock;
		std::vector<CustomEvent> customEventQueue;
		std::vector<u32> customEvent_textInputs;
	};

	extern BackendData* pBackendData;
}