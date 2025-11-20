#include "BackendData.hpp"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

using namespace DEngine;
using namespace DEngine::Platform;
namespace XPlatformToBackend = DEngine::Platform::impl::XPlatformToBackend;

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
	auto winInstance = backendData.instanceHandle;

	Context::CreateVkSurface_ReturnT returnVal = {};

	if (perWindowData.hwnd == nullptr) {
		returnVal.vkResult = VK_ERROR_UNKNOWN;
		return returnVal;
	}

	VkInstance instance = {};
	static_assert(sizeof(VkInstance) == sizeof(vkInstanceIn));
	memcpy(&instance, &vkInstanceIn, sizeof(VkInstance));
	// Load the function pointer
	auto loadInstanceProcAddrFn = (PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddrFn;
	auto funcPtr = (PFN_vkCreateWin32SurfaceKHR)loadInstanceProcAddrFn(instance, "vkCreateWin32SurfaceKHR");
	DENGINE_IMPL_ASSERT(funcPtr != nullptr);

	VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hinstance = winInstance;
	surfaceInfo.hwnd = perWindowData.hwnd;

	VkSurfaceKHR outSurface = {};
	auto result = funcPtr(instance, &surfaceInfo, (VkAllocationCallbacks*)vkAllocationCallbacks, &outSurface);

	static_assert(sizeof(result) == sizeof(returnVal.vkResult));
	memcpy(&returnVal.vkResult, &result, returnVal.vkResult);
	static_assert(sizeof(VkSurfaceKHR) == sizeof(returnVal.vkSurface));
	memcpy(&returnVal.vkSurface, &outSurface, sizeof(returnVal.vkSurface));
	return returnVal;
}