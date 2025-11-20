#include <DEngine/Platform/Platform.hpp>
#include <DEngine/Platform/PlatformAssert.hpp>

#include "BackendData.hpp"
#include "HandleCustomEvent.hpp"
#include "HandleInputEvent.hpp"

#include <DEngine/Std/Containers/Defer.hpp>

#include <android/configuration.h>
#include <android/log.h>

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>

#include <dlfcn.h>
#include <sys/eventfd.h>

#include <iostream>

using namespace DEngine;
using namespace DEngine::Platform;

namespace DEngine::Platform::impl
{
	[[nodiscard]] static Orientation ToOrientation(uint8_t aconfigOrientation) {
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

namespace DEngine::Platform::impl::Backend
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
		Std::Defer cleanup { [&]() {
			pollSource = {};
		} };

		// These polling functions have parameters we never use, made some quick lambdas
		// to wrap them.
		auto pollOnceWrapper = [&](int timeout) {
			return ALooper_pollOnce(
				timeout,
				nullptr,
				nullptr,
				nullptr);
		};

		if (waitForEvents && timeoutNs == 0) {
			// We want to wait indefinitely.
			while (true) {
				// -1 means to wait indefinitely.
				int pollResult = pollOnceWrapper(-1);
				// If pollResult is NOT set to POLL_CALLBACK, it means the thread was awoken but
				// didn't fire any callbacks. This can happen haphazardly.
				// If we wake up without any event, keep waiting indefinitely.
				if (pollResult != ALOOPER_POLL_CALLBACK)
					continue;

				// If we did fire a callback, we only fired a single one.
				// We now want to poll events until we are empty, without waiting.
				bool continuePolling = pollResult == ALOOPER_POLL_CALLBACK;
				while (continuePolling) {
					pollResult = pollOnceWrapper(0);
					continuePolling = pollResult == ALOOPER_POLL_CALLBACK;
				}
				break;
			}
			bool continuePolling = true;
			while (continuePolling) {
				int pollResult = pollOnceWrapper(0);
				continuePolling = pollResult == ALOOPER_POLL_CALLBACK;
			}


		} else if (!waitForEvents && timeoutNs == 0) {
			// Poll all pending events with immediate timeout,
            // until we no longer fire any events.
			bool continuePolling = true;
            while (continuePolling) {
                int pollResult = pollOnceWrapper(0);
                continuePolling = pollResult == ALOOPER_POLL_CALLBACK;
            }
		} else {
            std::abort();
            // Unimplemented
        }
	}

	static void WaitForInitialRequiredEvents(
		Context::Impl& implData,
		BackendData& backendData)
	{
		// TODO: Likely deadlock here when waiting for ApplyWindowInsets event.
		bool nativeWindowSet = false;
		bool visibleAreaSet = false;
		while (!nativeWindowSet || !visibleAreaSet) {

			CustomEvent_CallbackFnT fnRef = [&](CustomEventType type) {
				switch (type) {
					case CustomEventType::NativeWindowCreated:
						nativeWindowSet = true;
						break;
					case CustomEventType::ApplyWindowInsets:
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

	JniMethodIds LoadJavaMethodIds(
		JNIEnv& jniEnv,
		jclass appClass,
		jclass nativeToAndroidClass,
		jobject mainActivity)
	{
		JniMethodIds returnValue = {};

		returnValue._openSoftInput = jniEnv.GetStaticMethodID(
			nativeToAndroidClass,
			JniMethodIds::openSoftInput_Name,
			JniMethodIds::openSoftInput_Signature);
		DENGINE_IMPL_APPLICATION_ASSERT(returnValue._openSoftInput != nullptr);

		returnValue._updateInputConnection = jniEnv.GetStaticMethodID(
			nativeToAndroidClass,
			JniMethodIds::updateInputConnection_Name,
			JniMethodIds::updateInputConnection_Signature);
		DENGINE_IMPL_APPLICATION_ASSERT(returnValue._updateInputConnection != nullptr);

		returnValue._accessibilityUpdate = jniEnv.GetStaticMethodID(
			nativeToAndroidClass,
			JniMethodIds::accessibilityUpdate_Name,
			JniMethodIds::accessibilityUpdate_Signature);
		DENGINE_IMPL_APPLICATION_ASSERT(returnValue._accessibilityUpdate != nullptr);
		/*
		returnValue.openSoftInput = jniEnv.GetMethodID(
			activityClassObject,
			"NativeEvent_OpenSoftInput",
			"(Ljava/lang/String;I)V");
		 */
		jclass activityClassObject = jniEnv.GetObjectClass(mainActivity);
		returnValue.hideSoftInput = jniEnv.GetMethodID(
			activityClassObject,
			"NativeEvent_HideSoftInput",
			"()V");

		return returnValue;
	}
}

namespace Backend = DEngine::Platform::impl::Backend;
namespace XPlatformToBackend = DEngine::Platform::impl::XPlatformToBackend;

Backend::BackendData* Backend::pBackendDataInit = nullptr;

Context::Impl::PlatformBackendBase* XPlatformToBackend::Initialize(
	Context& ctx,
	Context::Impl& implData)
{
	auto& backendData = *Backend::pBackendDataInit;
	//pBackendDataInit = nullptr;

	// Attach this thread to the JVM, this gives us the JniEnv handle for this thread and
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
	backendData.jniMethodIds = Backend::LoadJavaMethodIds(
		*jniEnv,
		backendData.appClass,
		backendData.nativeToAndroidClass,
		backendData.mainActivity);


	// Load the ALooper object for this thread. We don't want to use the
	// ALOOPER_PREPARE_ALLOW_NON_CALLBACKS flag, all our events are callback based.
	backendData.gameThreadAndroidLooper = ALooper_prepare(0);

	// Add the event-file-descriptor for our custom events to this looper.
	ALooper_addFd(
		backendData.gameThreadAndroidLooper,
		backendData.customEventFd,
		(int)Backend::LooperIdentifier::CustomEvent,
		ALOOPER_EVENT_INPUT,
		&Backend::looperCallback_CustomEvent,
		&backendData.pollSource);

	// We need to wait for a few stuff, like the input queue and our native window before we can
	// really do anything. Probably not an ideal solution.
	WaitForInitialRequiredEvents(implData, backendData);

	return &backendData;
}

void XPlatformToBackend::ProcessEvents(
	Context& ctx,
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	bool waitForEvents,
	u64 timeoutNs)
{
	auto& backendData = (Backend::BackendData&)backendDataIn;

	RunEventPolling(
		implData,
		backendData,
		waitForEvents,
		timeoutNs);
}

auto XPlatformToBackend::NewWindow_Blocking(
    Context::Impl& implData,
    Context::Impl::PlatformBackendBase& backendDataIn,
    Std::Span<char const> const& title,
    Extent extent)
-> Std::Opt<NewWindow_ReturnT>
{
	auto& backendData = (Backend::BackendData&)backendDataIn;

	// We don't support multiple windows.
	if (backendData.currWindowId.Has() || backendData.nativeWindow == nullptr)
		return Std::nullOpt;

	NewWindow_ReturnT returnVal = {};
    auto* perWindowData = new Backend::PerWindowData;
	perWindowData->aNativeWindow = backendData.nativeWindow;

	auto& windowData = returnVal.windowData;

	int width = ANativeWindow_getWidth(backendData.nativeWindow);
	int height = ANativeWindow_getHeight(backendData.nativeWindow);
	windowData.extent.width = (u32)width;
	windowData.extent.height = (u32)height;
	windowData.insets = backendData.windowInsets;

	int density = AConfiguration_getDensity(backendData.currAConfig);
	windowData.dpiX = (f32)density;
	windowData.dpiY = (f32)density;

	windowData.contentScale = backendData.fontScale;

	windowData.touchScrollSlopPx = backendData.touchScrollSlopPx;
	windowData.scrollBarWidthPx = backendData.scrollBarWidthPx;

    WindowID windowId = {};
    // Insert the window into storage
    {
        std::scoped_lock idLock{implData.windowsLock};
        windowId = (WindowID) implData.windowIdTracker;
        implData.windowIdTracker++;
        implData.windows.push_back(Context::Impl::WindowNode{
            .id = windowId,
            .windowData = returnVal.windowData,
            .events = {},
            .backendData = Std::Box{perWindowData},
        });
    }

    returnVal.windowId = windowId;
    backendData.currWindowId = windowId;

	return returnVal;
}

void XPlatformToBackend::DestroyWindow(
	Context::Impl& implData,
    Context::Impl::PlatformBackendBase& backendDataIn,
	Context::Impl::WindowNode const& windowNode)
{
    auto& backendData = (Backend::BackendData&)backendDataIn;
	bool invalidDestroy =
		!backendData.currWindowId.Has() ||
		backendData.currWindowId.Get() != windowNode.id;
	if (invalidDestroy)
		throw std::runtime_error("Cannot destroy this window.");

	backendData.currWindowId = Std::nullOpt;
}

Context::CreateVkSurface_ReturnT XPlatformToBackend::CreateVkSurface(
    Context::Impl& implData,
    Context::Impl::PlatformBackendBase& backendDataIn,
    WindowPlatformBackendBase& windowBackendDataIn,
	void const* vkGetInstanceProcAddrFn,
    uSize vkInstanceIn,
    void const* vkAllocationCallbacks) noexcept
{
    auto& backendData = (Backend::BackendData&)backendDataIn;
	auto& perWindowData = (Backend::PerWindowData&)windowBackendDataIn;

	Context::CreateVkSurface_ReturnT returnValue = {};

	// If we have no active window, we can't make a surface.
	if (!backendData.nativeWindow || perWindowData.aNativeWindow != backendData.nativeWindow)
	{
		returnValue.vkResult = VK_ERROR_UNKNOWN;
		return returnValue;
	}

	VkInstance instance = {};
	static_assert(sizeof(instance) == sizeof(vkInstanceIn));
	memcpy(&instance, &vkInstanceIn, sizeof(instance));

	auto procAddr = (PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddrFn;

	// Load the function pointer
	auto funcPtr = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
		procAddr(
			instance,
			"vkCreateAndroidSurfaceKHR"));
	if (funcPtr == nullptr) {
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

Std::StackVec<char const*, 5> DEngine::Platform::GetRequiredVkInstanceExtensions() noexcept
{
	return { {
		"VK_KHR_surface",
		"VK_KHR_android_surface"
	} };
}

void XPlatformToBackend::Destroy(Context::Impl::PlatformBackendBase* data)
{
}

void Backend::WriteToLogcat(
	LogSeverity severity,
	Std::Span<char const> const& msg)
{
	std::string outString;
	outString.reserve(msg.Size());
	for (auto const &item: msg)
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

void XPlatformToBackend::Log(
	Context::Impl&,
    Context::Impl::PlatformBackendBase&,
	LogSeverity severity,
	Std::Span<char const> const& msg)
{
	Backend::WriteToLogcat(severity, msg);
}

bool XPlatformToBackend::StartTextInputSession(
    Context::Impl& implData,
    WindowID windowId,
    Context::Impl::PlatformBackendBase& backendDataIn,
    SoftInputFilter inputFilter,
    Std::Span<char const> const& textInput,
	u64 selectionStart,
	u64 selectionCount)
{
	auto& backendData = (Backend::BackendData&)backendDataIn;

	// We have to be in the correct thread
	DENGINE_IMPL_APPLICATION_ASSERT(std::this_thread::get_id() == backendData.gameThread.get_id());

	std::vector<jchar> tempString;
	tempString.reserve(textInput.Size());
	for (uSize i = 0; i < textInput.Size(); i++) {
		tempString.push_back((jchar) textInput[i]);
	}
	jstring javaString = backendData.gameThreadJniEnv->NewString(
		tempString.data(),
		tempString.size());

	backendData.jniMethodIds.openSoftInput(
		backendData.gameThreadJniEnv,
		backendData.nativeToAndroidClass,
		backendData.mainActivity,
		javaString,
		selectionStart,
		selectionCount,
		inputFilter);

	backendData.gameThreadJniEnv->functions->DeleteLocalRef(
		backendData.gameThreadJniEnv,
		javaString);

	return true;
}

bool XPlatformToBackend::UpdateTextInputConnection(
	Context::Impl& implData,
	Context::Impl::PlatformBackendBase& backendDataIn,
	WindowID windowId,
	u64 selStart,
	u64 selCount,
	Std::Span<u32 const> newText)
{
	auto& backendData = (Backend::BackendData&)backendDataIn;
	auto& jniEnv = *backendData.gameThreadJniEnv;

	int textCount = newText.Size();
	std::vector<jbyte> textBytes;
	textBytes.reserve(textCount);
	for (auto const& item : newText)
		textBytes.push_back((jbyte)item);

	jbyteArray textArray = jniEnv.NewByteArray(textCount);
	jniEnv.SetByteArrayRegion(textArray, 0, textCount, textBytes.data());

	const auto result = backendData.jniMethodIds.UpdateInputConnection(
		jniEnv,
		backendData.nativeToAndroidClass,
		backendData.appHandle,
		backendData.mainActivity,
		(jlong)windowId,
		(jint)selStart,
		(jint)selCount,
		textArray);
	jniEnv.DeleteLocalRef(textArray);

	return result;
}

void XPlatformToBackend::UpdateTextInputConnectionSelection(
	Context::Impl::PlatformBackendBase& backendDataIn,
	u64 selIndex,
	u64 selCount)
{
	// Uhh, this is called by the UI as a response to the prior event?
	//std::abort();
}

void XPlatformToBackend::StopTextInputSession(
	Context::Impl& implData,
    Context::Impl::PlatformBackendBase& backendDataIn)
{
	auto& backendData = (Backend::BackendData&)backendDataIn;

	backendData.gameThreadJniEnv->CallVoidMethod(
		backendData.mainActivity,
		backendData.jniMethodIds.hideSoftInput);
}


void XPlatformToBackend::UpdateAccessibility(
	Context::Impl& implData,
    Context::Impl::PlatformBackendBase& backendDataIn,
	WindowID windowId,
	Std::RangeFnRef<AccessibilityUpdateElement> const& range,
	Std::Span<char const> textData)
{
	auto& backendData = (Backend::BackendData&)backendDataIn;
	auto& jniEnv = *backendData.gameThreadJniEnv;

	int tempCount = range.Size();

	// TODO: For now we gotta split the data up into arrays of primitive members.
	// Should be handled differently in the future. Maybe using fancy compile-time
	// program to handle yeeting of C-structs between C++ and Java?
	std::vector<jlong> widgetIdsTemp;
	widgetIdsTemp.resize(tempCount);
	std::vector<jint> posXTemp;
	posXTemp.resize(tempCount);
	std::vector<jint> posYTemp;
	posYTemp.resize(tempCount);
	std::vector<jint> widthTemp;
	widthTemp.resize(tempCount);
	std::vector<jint> heightTemp;
	heightTemp.resize(tempCount);
	std::vector<jboolean> clickableTemp;
	clickableTemp.resize(tempCount);
	std::vector<jint> textStartTemp;
	textStartTemp.resize(tempCount);
	std::vector<jint> textCountTemp;
	textCountTemp.resize(tempCount);

	int i = 0;
	for (auto const& accessItem : range) {
		widgetIdsTemp[i] = accessItem.widgetId;
		posXTemp[i] = accessItem.posX;
		posYTemp[i] = accessItem.posY;
		widthTemp[i] = accessItem.width;
		heightTemp[i] = accessItem.height;
		clickableTemp[i] = accessItem.isClickable;
		textStartTemp[i] = accessItem.textStart;
		textCountTemp[i] = accessItem.textCount;
		i++;
	}

	jlongArray widgetIds = jniEnv.NewLongArray(tempCount);
	jniEnv.SetLongArrayRegion(widgetIds, 0, tempCount, widgetIdsTemp.data());
	jintArray posX = jniEnv.NewIntArray(tempCount);
	jniEnv.SetIntArrayRegion(posX, 0, tempCount, posXTemp.data());
	jintArray posY = jniEnv.NewIntArray(tempCount);
	jniEnv.SetIntArrayRegion(posY, 0, tempCount, posYTemp.data());
	jintArray width = jniEnv.NewIntArray(tempCount);
	jniEnv.SetIntArrayRegion(width, 0, tempCount, widthTemp.data());
	jintArray height = jniEnv.NewIntArray(tempCount);
	jniEnv.SetIntArrayRegion(height, 0, tempCount, heightTemp.data());
	jbooleanArray clickable = jniEnv.NewBooleanArray(tempCount);
	jniEnv.SetBooleanArrayRegion(clickable, 0, tempCount, clickableTemp.data());
	jintArray textStart = jniEnv.NewIntArray(tempCount);
	jniEnv.SetIntArrayRegion(textStart, 0, tempCount, textStartTemp.data());
	jintArray textCount = jniEnv.NewIntArray(tempCount);
	jniEnv.SetIntArrayRegion(textCount, 0, tempCount, textCountTemp.data());

	static_assert(sizeof(jbyte) == sizeof(textData[0]));
	jbyteArray textDataTemp = jniEnv.NewByteArray(textData.Size());
	jniEnv.SetByteArrayRegion(
		textDataTemp,
		0,
		textData.Size(),
		reinterpret_cast<jbyte const*>(textData.Data()));

	backendData.jniMethodIds.AccessibilityUpdate(
		&jniEnv,
		backendData.nativeToAndroidClass,
		backendData.appHandle,
		backendData.mainActivity,
		static_cast<jlong>(windowId),
		widgetIds,
		posX,
		posY,
		width,
		height,
		clickable,
		textStart,
		textCount,
		textDataTemp);

	jniEnv.DeleteLocalRef(widgetIds);
	jniEnv.DeleteLocalRef(posX);
	jniEnv.DeleteLocalRef(posY);
	jniEnv.DeleteLocalRef(width);
	jniEnv.DeleteLocalRef(height);
	jniEnv.DeleteLocalRef(clickable);
	jniEnv.DeleteLocalRef(textStart);
	jniEnv.DeleteLocalRef(textCount);
	jniEnv.DeleteLocalRef(textDataTemp);

    // Unimplemented
    //std::abort();
}