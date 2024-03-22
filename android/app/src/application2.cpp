#include <DEngine/detail/Application.hpp>


#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/input.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>

#include <dlfcn.h>

#include <thread>
#include <mutex>
#include <jni.h>

void DENGINE_APP_MAIN_ENTRYPOINT(int argc, char** argv);

namespace DEngine::Application::detail
{
	enum class LooperID
	{
		Input
	};
	struct BackendData
	{
		std::thread gameThread;
		std::mutex customEventQueueLock;

		AAssetManager* assetManager = nullptr;
		AInputQueue* inputQueue = nullptr;

		bool startCalled = false;

		ANativeWindow* nativeWindow = nullptr;
	};

	BackendData* pBackendData = nullptr;
}

using namespace DEngine;

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_MainActivity_nativeOnCreate(
	JNIEnv *env,
	jobject thiz,
	jobject assetManager)
{
	using namespace DEngine::Application;
	using namespace DEngine::Application::detail;

	Application::detail::pBackendData = new Application::detail::BackendData;
	auto& backendData = *Application::detail::pBackendData;

	backendData.assetManager = AAssetManager_fromJava(env, assetManager);

	auto lambda = []()
	{
		DENGINE_APP_MAIN_ENTRYPOINT(0, nullptr);
	};
	backendData.gameThread = std::thread(lambda);
	auto threadName = "MainGameThread";
	pthread_setname_np(backendData.gameThread.native_handle(), threadName);
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_MainActivity_nativeOnStart(
	JNIEnv *env,
	jobject thiz)
{
	auto& backendData = *Application::detail::pBackendData;
	std::lock_guard _{ backendData.lock };
	backendData.startCalled = true;
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_MainActivity_nativeOnSurfaceCreated(
	JNIEnv *env,
	jobject thiz,
	jobject nativeWindow)
{
	auto& backendData = *Application::detail::pBackendData;
	ANativeWindow* temp = ANativeWindow_fromSurface(env, nativeWindow);
	std::lock_guard _{ backendData.lock };
	backendData.nativeWindow = temp;
}

extern "C"
JNIEXPORT void JNICALL
Java_didgy_dengine_MainActivity_nativeOnInputQueueCreated(
	JNIEnv *env,
	jobject thiz,
	jobject inputQueue)
{
	auto& backendData = *Application::detail::pBackendData;
	std::lock_guard _{ backendData.lock };
	backendData.inputQueue = reinterpret_cast<AInputQueue*>(inputQueue);
}

bool Application::detail::Backend_Initialize()
{
	auto& backendData = *Application::detail::pBackendData;

	while(true)
	{
		std::lock_guard _{ backendData.lock };
		if (backendData.startCalled && backendData.nativeWindow && backendData.inputQueue)
			break;
	}

	ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS); // ALOOPER_PREPARE_ALLOW_NON_CALLBACKS ?
	AInputQueue_attachLooper(
		backendData.inputQueue,
		looper,
		(int)LooperID::Input,
		nullptr,
		&backendData);

	return true;
}

void Application::detail::Backend_ProcessEvents()
{

}

Application::WindowID Application::CreateWindow(
	char const* title,
	Extent extents)
{
	auto& appData = *detail::pAppData;
	auto& backendData = *detail::pBackendData;

	detail::AppData::WindowNode newNode{};
	newNode.id = (App::WindowID)appData.windowIdTracker;
	appData.windowIdTracker++;
	newNode.platformHandle = backendData.nativeWindow;

	int width = ANativeWindow_getWidth(backendData.nativeWindow);
	int height = ANativeWindow_getHeight(backendData.nativeWindow);
	newNode.windowData.size.width = (u32)width;
	newNode.windowData.size.height = (u32)height;

	appData.windows.push_back(newNode);

	return newNode.id;
}

Std::Opt<u64> Application::CreateVkSurface(
	WindowID window,
	uSize vkInstance,
	void const* vkAllocationCallbacks)
{
	PFN_vkGetInstanceProcAddr procAddr = nullptr;
	void* lib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	if (lib == nullptr)
		return {};
	procAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(lib, "vkGetInstanceProcAddr"));
	int closeReturnCode = dlclose(lib);
	lib = nullptr;
	if (procAddr == nullptr || closeReturnCode != 0)
		return {};

	auto funcPtr = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
		procAddr(
			*reinterpret_cast<VkInstance*>(&vkInstance),
			"vkCreateAndroidSurfaceKHR"));
	if (funcPtr == nullptr)
		return {};

	auto& backendData = *detail::pBackendData;

	VkAndroidSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	createInfo.window = backendData.nativeWindow;

	VkSurfaceKHR returnVal{};
	VkResult result = funcPtr(
		*reinterpret_cast<VkInstance*>(&vkInstance),
		&createInfo,
		reinterpret_cast<VkAllocationCallbacks const*>(vkAllocationCallbacks),
		&returnVal);
	if (result != VK_SUCCESS || returnVal == VkSurfaceKHR())
		return Std::nullOpt;

	u64 temp = *reinterpret_cast<u64*>(&returnVal);

	return Std::Opt{ temp };
}

Std::StackVec<char const*, 5> Application::RequiredVulkanInstanceExtensions()
{
	Std::StackVec<char const*, 5> returnVal{};
	returnVal.PushBack("VK_KHR_surface");
	returnVal.PushBack("VK_KHR_android_surface");
	return returnVal;
}

void Application::LockCursor(bool state)
{

}

void Application::SetCursor(WindowID id, CursorType cursorType)
{

}

void Application::OpenSoftInput()
{
	const auto& backendData = *detail::pBackendData;
	//backendData.jniEnv->CallVoidMethod(backendData.app->activity->clazz, backendData.jniOpenSoftInput);
}

void Application::detail::Backend_Log(char const* msg)
{
	__android_log_print(ANDROID_LOG_ERROR, "DEngine: ", "%s", msg);
}


//
// File IO
//

namespace DEngine::Application::detail
{
	struct Backend_FileInputStreamData
	{
		AAsset* file = nullptr;
		// There is no AAsset_tell() so we need to remember the position manually.
		size_t pos = 0;
	};
}

Application::FileInputStream::FileInputStream()
{
	static_assert(sizeof(detail::Backend_FileInputStreamData) <= sizeof(FileInputStream::m_buffer));

	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}

Application::FileInputStream::FileInputStream(char const* path)
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	Open(path);
}

Application::FileInputStream::FileInputStream(FileInputStream&& other) noexcept
{
	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(detail::Backend_FileInputStreamData));
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));
}

Application::FileInputStream::~FileInputStream()
{
	Close();
}

Application::FileInputStream& DEngine::Application::FileInputStream::operator=(FileInputStream&& other) noexcept
{
	if (this == &other)
		return *this;

	Close();

	std::memcpy(&m_buffer[0], &other.m_buffer[0], sizeof(detail::Backend_FileInputStreamData));
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&other.m_buffer[0], &implData, sizeof(implData));

	return *this;
}


bool Application::FileInputStream::Seek(i64 offset, SeekOrigin origin)
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));

	if (implData.file == nullptr)
		return false;

	int posixOrigin = 0;
	switch (origin)
	{
		case SeekOrigin::Current:
			posixOrigin = SEEK_CUR;
			break;
		case SeekOrigin::Start:
			posixOrigin = SEEK_SET;
			break;
		case SeekOrigin::End:
			posixOrigin = SEEK_END;
			break;
	}
	off64_t result = AAsset_seek64(implData.file, static_cast<off64_t>(offset), posixOrigin);
	if (result == -1)
		return false;

	implData.pos = static_cast<size_t>(result);
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	return true;
}

bool Application::FileInputStream::Read(char* output, u64 size)
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file == nullptr)
		return false;

	int result = AAsset_read(implData.file, output, (size_t)size);
	if (result < 0)
		return false;

	implData.pos += size;
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));

	return true;
}

Std::Opt<u64> Application::FileInputStream::Tell() const
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file == nullptr)
		return {};

	return Std::Opt{ (u64)implData.pos };
}

bool Application::FileInputStream::IsOpen() const
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	return implData.file != nullptr;
}

bool Application::FileInputStream::Open(char const* path)
{
	Close();

	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	auto const& backendData = *detail::pBackendData;
	implData.file = AAssetManager_open(backendData.assetManager, path, AASSET_MODE_RANDOM);
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
	return implData.file != nullptr;
}

void Application::FileInputStream::Close()
{
	detail::Backend_FileInputStreamData implData{};
	std::memcpy(&implData, &m_buffer[0], sizeof(implData));
	if (implData.file != nullptr)
		AAsset_close(implData.file);

	implData = detail::Backend_FileInputStreamData{};
	std::memcpy(&m_buffer[0], &implData, sizeof(implData));
}
