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

using namespace DEngine;
using namespace DEngine::Application;

Application::impl::BackendData* Application::impl::pBackendDataInit = nullptr;

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
			return ALooper_pollOnce(
				timeout,
				nullptr,
				nullptr,
				nullptr);
		};
		auto pollAllWrapper = [&](int timeout) {
			return ALooper_pollAll(
				timeout,
				nullptr,
				nullptr,
				nullptr);
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
		while (!nativeWindowSet || !visibleAreaSet) {

			CustomEvent_CallbackFnT fnRef = [&](CustomEventType type) {
				switch (type) {
					case CustomEventType::NativeWindowCreated:
						nativeWindowSet = true;
						break;
					case CustomEventType::ContentRectChanged:
						visibleAreaSet = true;
						break;
					default:
						break;
				}
			};

			RunEventPolling(
				implData,
				backendData,
				true,
				0,
				fnRef);
		}
	}

	JniMethodIds LoadJavaMethodIds(JNIEnv& jniEnv, jclass appClass, jobject mainActivity) {
		JniMethodIds returnValue = {};

		returnValue.updateAccessibility = jniEnv.GetStaticMethodID(
			appClass,
			"NativeEvent_AccessibilityUpdate",
			"(Ldidgy/dengine/DEngineApp;I[I[B)V");

		jclass activityClassObject = jniEnv.GetObjectClass(mainActivity);
		returnValue.openSoftInput = jniEnv.GetMethodID(
			activityClassObject,
			"NativeEvent_OpenSoftInput",
			"(Ljava/lang/String;I)V");
		returnValue.hideSoftInput = jniEnv.GetMethodID(
			activityClassObject,
			"NativeEvent_HideSoftInput",
			"()V");

		return returnValue;
	}
}

void* Application::impl::Backend::Initialize(
	Context& ctx,
	Context::Impl& implData)
{
	auto& backendData = *pBackendDataInit;
	//pBackendDataInit = nullptr;

	// First we attach this thread to the JVM,
	// this gives us the JniEnv handle for this thread and
	// allows this thread to do Java/JNI stuff.
	JNIEnv* jniEnv = nullptr;
	jint attachThreadResult = backendData.globalJavaVm->AttachCurrentThread(
		&jniEnv,
		nullptr);
	if (attachThreadResult != JNI_OK) {
		// Attaching failed. Crash the program. Can't really handle this,
		// we should probably use a more elegant method of crashing though.
		std::abort();
	}
	backendData.gameThreadJniEnv = jniEnv;

	// Initialize stuff that depends on this jniEnv.
	backendData.appClass = jniEnv->GetObjectClass(backendData.appHandle);
	backendData.jniMethodIds = LoadJavaMethodIds(
		*jniEnv,
		backendData.appClass,
		backendData.mainActivity);

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

	return &backendData;
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
	if (backendData.currWindowId.Has() || backendData.nativeWindow == nullptr)
		return Std::nullOpt;

	backendData.currWindowId = windowId;

	NewWindow_ReturnT returnValue = {};
	returnValue.platformHandle = backendData.nativeWindow;

	auto& windowData = returnValue.windowData;

	int width = ANativeWindow_getWidth(backendData.nativeWindow);
	int height = ANativeWindow_getHeight(backendData.nativeWindow);
	windowData.extent.width = (u32)width;
	windowData.extent.height = (u32)height;
	windowData.visibleOffset = backendData.visibleAreaOffset;
	windowData.visibleExtent = backendData.visibleAreaExtent;

	int density = AConfiguration_getDensity(backendData.currAConfig);
	windowData.dpiX = (f32)density;
	windowData.dpiY = (f32)density;

	windowData.contentScale = backendData.fontScale;

	return returnValue;
}

void Application::impl::Backend::DestroyWindow(
	Context::Impl& implData,
	void* backendDataIn,
	Context::Impl::WindowNode const& windowNode)
{
	auto& backendData = *static_cast<BackendData*>(backendDataIn);
	bool invalidDestroy =
		!backendData.currWindowId.Has() ||
		backendData.currWindowId.Get() != windowNode.id;
	if (invalidDestroy)
		throw std::runtime_error("Cannot destroy this window.");

	backendData.currWindowId = Std::nullOpt;
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

namespace DEngine::Application::impl::Backend {
	struct AccessibilityUpdateElementJni {
		int posX;
		int posY;
		int width;
		int height;
		int textStart;
		int textCount;
		int isClickable;

		static constexpr int memberCount = 7;

		void FillArray(int* ptr) const {
			ptr[0] = posX;
			ptr[1] = posY;
			ptr[2] = width;
			ptr[3] = height;
			ptr[4] = textStart;
			ptr[5] = textCount;
			ptr[6] = isClickable;
		}
	};
}

void Application::impl::Backend::UpdateAccessibility(
	Context::Impl& implData,
	void* backendDataIn,
	WindowID windowId,
	Std::RangeFnRef<AccessibilityUpdateElement> const& range,
	Std::ConstByteSpan textData)
{
	auto& backendData = *(BackendData*)backendDataIn;
	auto& jniEnv = *backendData.gameThreadJniEnv;
	auto rangeCount = range.Size();

	auto byteArray = jniEnv.NewByteArray(textData.Size());
	auto byteArrayPtr = jniEnv.GetByteArrayElements(byteArray, nullptr);
	std::memcpy(byteArrayPtr, textData.Data(), textData.Size());
	jniEnv.ReleaseByteArrayElements(byteArray, byteArrayPtr, 0);

	auto intArray = jniEnv.NewIntArray(rangeCount * AccessibilityUpdateElementJni::memberCount);
	auto intArrayPtr = jniEnv.GetIntArrayElements(intArray, nullptr);
	for (int i = 0; i < rangeCount; i++) {
		auto const& item = range.Invoke(i);
		AccessibilityUpdateElementJni out = {
			.posX = item.posX, .posY = item.posY,
			.width = item.width, .height = item.height,
			.textStart = item.textStart, .textCount = item.textCount };
		if (item.isClickable)
			out.isClickable = 1;
		out.FillArray(intArrayPtr + i * AccessibilityUpdateElementJni::memberCount);
	}
	jniEnv.ReleaseIntArrayElements(intArray, intArrayPtr, 0);

	backendData.gameThreadJniEnv->CallStaticVoidMethod(
		backendData.appClass,
		backendData.jniMethodIds.updateAccessibility,
		backendData.appHandle,
		(int)windowId,
		intArray,
		byteArray);
}