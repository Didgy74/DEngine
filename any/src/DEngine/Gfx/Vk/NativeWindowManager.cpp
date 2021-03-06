#include "NativeWindowManager.hpp"

#include "Vk.hpp"
#include "DeletionQueue.hpp"
#include "QueueData.hpp"

#include <DEngine/Math/Common.hpp>
#include <DEngine/Std/Utility.hpp>

#include <string>

namespace DEngine::Gfx::Vk::NativeWinManImpl
{
	struct SwapchainSettings
	{
		vk::PresentModeKHR presentMode{};
		vk::SurfaceFormatKHR surfaceFormat{};
		vk::SurfaceTransformFlagBitsKHR transform{};
		vk::CompositeAlphaFlagBitsKHR compositeAlphaFlag{};
		vk::Extent2D extents{};
		u32 numImages{};
	};

	[[nodiscard]] static SwapchainSettings BuildSwapchainSettings(
		InstanceDispatch const& instance,
		vk::PhysicalDevice physDevice,
		vk::SurfaceKHR surface,
		SurfaceInfo const& surfaceInfo)
	{
		auto surfaceCaps = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface);

		SwapchainSettings temp{};
		temp.compositeAlphaFlag = surfaceInfo.compositeAlphaToUse;


		temp.extents = surfaceCaps.currentExtent;
		if (surfaceCaps.currentTransform == vk::SurfaceTransformFlagBitsKHR::eRotate90 ||
			surfaceCaps.currentTransform == vk::SurfaceTransformFlagBitsKHR::eRotate270)
			Std::Swap(temp.extents.width, temp.extents.height);


		// Handle swapchainData length
		u32 swapchainLength = Constants::preferredSwapchainLength;
		// If we need to, clamp the swapchainData length.
		// Upper clamp only applies if maxImageCount != 0
		if (surfaceCaps.maxImageCount != 0)
			swapchainLength = Math::Clamp(swapchainLength, surfaceCaps.minImageCount, surfaceCaps.maxImageCount);
		if (swapchainLength < 2)
			throw std::runtime_error("DEngine - Renderer: Vulkan backend doesn't support swapchainData length of 1.");
		else if (swapchainLength > Constants::maxInFlightCount)
			throw std::runtime_error("DEngine - Renderer: Cannot make a swapchain with length higher than maxResourceSets.");
		temp.numImages = swapchainLength;

		temp.presentMode = surfaceInfo.presentModeToUse;
		temp.surfaceFormat = surfaceInfo.surfaceFormatToUse;
		temp.transform = surfaceCaps.currentTransform;
		return temp;
	}

	static Math::Mat2 RotationToMatrix(vk::SurfaceTransformFlagBitsKHR transform)
	{
		switch (transform)
		{
		case vk::SurfaceTransformFlagBitsKHR::eIdentity:
			return Math::Mat2{ { { 1, 0, 0, 1 } } };
		case vk::SurfaceTransformFlagBitsKHR::eRotate90:
			return Math::Mat2{ { { 0, -1, 1, 0 } } };
		case vk::SurfaceTransformFlagBitsKHR::eRotate180:
			return Math::Mat2{ { { -1, 0, 0, -1 } } };
		case vk::SurfaceTransformFlagBitsKHR::eRotate270:
			return Math::Mat2{ { { 0, 1, -1, 0 } } };
		default:
			DENGINE_IMPL_UNREACHABLE();
			return Math::Mat2();
		}
	}

	static vk::SwapchainKHR CreateSwapchain(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		vk::SurfaceKHR surface,
		SwapchainSettings settings,
		vk::SwapchainKHR oldSwapchain,
		DebugUtilsDispatch const* debugUtils);

	static Std::StackVec<vk::Image, Const::maxSwapchainLength> GetSwapchainImages(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		vk::SwapchainKHR swapchain,
		DebugUtilsDispatch const* debugUtils);

	[[nodiscard]] static Std::StackVec<vk::ImageView, Const::maxSwapchainLength> CreateSwapchainImgViews(
		DeviceDispatch const& device,
		NativeWindowID windowId,
		vk::Format format,
		Std::Span<vk::Image const> swapchainImgs,
		DebugUtilsDispatch const* debugUtils);

	[[nodiscard]] static Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> CreateSwapchainFramebuffers(
		DeviceDispatch const& device,
		NativeWindowID windowId,
		Std::Span<vk::ImageView const> imgViews,
		vk::RenderPass renderPass,
		vk::Extent2D extents,
		DebugUtilsDispatch const* debugUtils);

	static vk::Semaphore CreateImageReadySemaphore(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		DebugUtilsDispatch const* debugUtils);

	static void HandleCreationJobs(
		NativeWindowManager& manager,
		GlobUtils const& globUtils);

	static void HandleWindowResize(
		NativeWindowManager& manager,
		GlobUtils const& globUtils,
		NativeWindowManager::Node& windowNode);

	static void HandleWindowRestore(
		NativeWindowManager& manager,
		GlobUtils const& globUtils,
		NativeWindowManager::Node& windowNode);
}

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;
namespace DEngine::Gfx::Vk
{
	using NativeWinMan = NativeWindowManager;
}

void NativeWinMan::ProcessEvents(
	NativeWinMan& manager,
	GlobUtils const& globUtils,
	Std::Span<NativeWindowUpdate const> windowUpdates)
{
	NativeWinManImpl::HandleCreationJobs(
		manager,
		globUtils);

	// First we see if there are resizes at all, so we know if we have to stall the device.
	bool needToStall = false;
	for (auto const& item : windowUpdates)
	{
		switch (item.event)
		{
		case NativeWindowEvent::Resize:
		case NativeWindowEvent::Restore:
			needToStall = true;
			break;
		default:
			break;
		}
	}
	if (needToStall)
		globUtils.device.waitIdle();
	for (auto const& item : windowUpdates)
	{
		auto const windowNodeIt = Std::FindIf(
			manager.nativeWindows.begin(),
			manager.nativeWindows.end(),
			[&item](auto const& val) -> bool { return item.id == val.id; });
		DENGINE_DETAIL_GFX_ASSERT(windowNodeIt != manager.nativeWindows.end());
		auto& windowNode = *windowNodeIt;

		switch (item.event)
		{
		case NativeWindowEvent::Resize:
			NativeWinManImpl::HandleWindowResize(
				manager,
				globUtils,
				windowNode);
			break;
		case NativeWindowEvent::Restore:
			NativeWinManImpl::HandleWindowRestore(
				manager,
				globUtils,
				windowNode);
			break;
		default:
			break;
		}
	}
}

void NativeWindowManager::Initialize(
	NativeWindowManager& manager,
	DeviceDispatch const& device,
	QueueData const& queues,
	DebugUtilsDispatch const* debugUtils)
{


}

void NativeWindowManager::BuildInitialNativeWindow(
	NativeWindowManager& manager,
	InstanceDispatch const& instance,
	DeviceDispatch const& device,
	vk::PhysicalDevice physDevice,
	QueueData const& queues,
	DeletionQueue const& delQueue,
	SurfaceInfo const& surfaceInfo,
	VmaAllocator vma,
	u8 inFlightCount,
	WsiInterface& initialWindowConnection,
	vk::SurfaceKHR surface,
	vk::RenderPass guiRenderPass,
	DebugUtilsDispatch const* debugUtils)
{
	NativeWindowData windowData = {};
	windowData.wsiConnection = &initialWindowConnection;
	windowData.surface = surface;
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			windowData.surface,
			"NativeWindow #0 - Surface");
	}

	NativeWinManImpl::SwapchainSettings swapchainSettings = NativeWinManImpl::BuildSwapchainSettings(
		instance,
		physDevice,
		surface,
		surfaceInfo);

	windowData.swapchain = NativeWinManImpl::CreateSwapchain(
		device,
		NativeWindowID(0),
		windowData.surface,
		swapchainSettings,
		windowData.swapchain,
		debugUtils);

	windowData.swapchainImages = NativeWinManImpl::GetSwapchainImages(
		device,
		(NativeWindowID)0,
		windowData.swapchain,
		debugUtils);

	// Make imageview for each swapchain image
	windowData.swapchainImgViews = NativeWinManImpl::CreateSwapchainImgViews(
		device,
		(NativeWindowID)0,
		swapchainSettings.surfaceFormat.format,
		windowData.swapchainImages,
		debugUtils);

	// Build framebuffer for each swapchain image
	windowData.framebuffers = NativeWinManImpl::CreateSwapchainFramebuffers(
		device,
		(NativeWindowID)0,
		windowData.swapchainImgViews,
		guiRenderPass,
		swapchainSettings.extents,
		debugUtils);

	windowData.swapchainImgReadySem = NativeWinManImpl::CreateImageReadySemaphore(
		device,
		(NativeWindowID)0,
		debugUtils);

	WindowGuiData gui{};

	gui.extent = swapchainSettings.extents;
	gui.rotation = NativeWinManImpl::RotationToMatrix(swapchainSettings.transform);
	gui.surfaceRotation = swapchainSettings.transform;

	Node newNode{};
	newNode.id = NativeWindowID(0);
	newNode.windowData = windowData;
	newNode.gui = gui;
	manager.nativeWindows.push_back(newNode);
	manager.nativeWindowIdTracker += 1;
}

Gfx::NativeWindowID NativeWinMan::PushCreateWindowJob(
	NativeWinMan& manager,
	WsiInterface& windowConnection)
{
	std::lock_guard _{ manager.createQueueLock };

	CreateJob newJob{};
	newJob.id = (NativeWindowID)manager.nativeWindowIdTracker;
	manager.nativeWindowIdTracker++;
	newJob.windowConnection = &windowConnection;

	manager.createJobs.push_back(newJob);

	return newJob.id;
}

static vk::SwapchainKHR NativeWinManImpl::CreateSwapchain(
	DeviceDispatch const& device,
	NativeWindowID windowID,
	vk::SurfaceKHR surface,
	SwapchainSettings settings,
	vk::SwapchainKHR oldSwapchain,
	DebugUtilsDispatch const* debugUtils)
{
	vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageExtent = settings.extents;
	swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
	swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapchainCreateInfo.presentMode = settings.presentMode;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.preTransform = settings.transform;
	swapchainCreateInfo.clipped = 1;
	swapchainCreateInfo.compositeAlpha = settings.compositeAlphaFlag;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	swapchainCreateInfo.minImageCount = settings.numImages;
	swapchainCreateInfo.oldSwapchain = oldSwapchain;
	vk::SwapchainKHR swapchain = device.createSwapchainKHR(swapchainCreateInfo);
	if (debugUtils)
	{
		std::string name;
		name += "NativeWindow #";
		name += std::to_string((u64)windowID);
		name += " - Swapchain";
		debugUtils->Helper_SetObjectName(device.handle, swapchain, name.c_str());
	}

	return swapchain;
}

Std::StackVec<vk::Image, Constants::maxSwapchainLength> NativeWinManImpl::GetSwapchainImages(
	DeviceDispatch const& device, 
	NativeWindowID windowID, 
	vk::SwapchainKHR swapchain, 
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};
	Std::StackVec<vk::Image, Constants::maxSwapchainLength> returnVal;

	u32 swapchainImageCount = 0;
	vkResult = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, nullptr);
	if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
		throw std::runtime_error("Unable to grab swapchainData images from VkSwapchainKHR object.");
	if (swapchainImageCount > returnVal.Capacity())
		throw std::runtime_error("Unable to fit swapchainData image handles in allocated memory.");
	returnVal.Resize(swapchainImageCount);
	vkResult = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, returnVal.Data());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Unable to grab swapchainData images from VkSwapchainKHR object.");
	// Make names for the swapchainData images
	if (debugUtils)
	{
		for (uSize i = 0; i < returnVal.Size(); i += 1)
		{
			std::string name;
			name += "NativeWindow #";
			name += std::to_string((u64)windowID);
			name += " - SwapchainImg #";
			name += std::to_string(i);
			debugUtils->Helper_SetObjectName(device.handle, returnVal[i], name.c_str());
		}
	}

	return returnVal;
}

[[nodiscard]] static Std::StackVec<vk::ImageView, Const::maxSwapchainLength> NativeWinManImpl::CreateSwapchainImgViews(
	DeviceDispatch const& device,
	NativeWindowID windowId,
	vk::Format format,
	Std::Span<vk::Image const> swapchainImgs,
	DebugUtilsDispatch const* debugUtils)
{
	Std::StackVec<vk::ImageView, Const::maxSwapchainLength> returnVal;
	returnVal.Resize(swapchainImgs.Size());
	for (uSize i = 0; i < returnVal.Size(); i += 1)
	{
		vk::ImageViewCreateInfo imgViewInfo = {};
		imgViewInfo.components = vk::ComponentSwizzle::eIdentity;
		imgViewInfo.format = format;
		imgViewInfo.image = swapchainImgs[i];
		imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgViewInfo.subresourceRange.baseArrayLayer = 0;
		imgViewInfo.subresourceRange.baseMipLevel = 0;
		imgViewInfo.subresourceRange.layerCount = 1;
		imgViewInfo.subresourceRange.levelCount = 1;
		imgViewInfo.viewType = vk::ImageViewType::e2D;
		returnVal[i] = device.createImageView(imgViewInfo);

		if (debugUtils)
		{
			std::string name = "NativeWindow #";
			name += std::to_string((u64)windowId);
			name += " - Swapchain ImgView #";
			name += std::to_string(i);
			debugUtils->Helper_SetObjectName(device.handle, returnVal[i], name.c_str());
		}
	}
	return returnVal;
}

Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> NativeWinManImpl::CreateSwapchainFramebuffers(
	DeviceDispatch const& device,
	NativeWindowID windowId, 
	Std::Span<vk::ImageView const> imgViews, 
	vk::RenderPass renderPass,
	vk::Extent2D extents, 
	DebugUtilsDispatch const* debugUtils)
{
	Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> returnVal;
	returnVal.Resize(imgViews.Size());
	for (uSize i = 0; i < returnVal.Size(); i += 1)
	{
		vk::FramebufferCreateInfo fbInfo = {};
		fbInfo.height = extents.height;
		fbInfo.layers = 1;
		fbInfo.renderPass = renderPass;
		fbInfo.width = extents.width;
		fbInfo.attachmentCount = 1;
		fbInfo.pAttachments = &imgViews[i];
		returnVal[i] = device.createFramebuffer(fbInfo);

		if (debugUtils)
		{
			std::string name = "NativeWindow #";
			name += std::to_string((u64)windowId);
			name +=	"- Swapchain Framebuffer #";
			name += (int)i;
			debugUtils->Helper_SetObjectName(device.handle, returnVal[i], name.c_str());
		}
	}
	return returnVal;
}

vk::Semaphore NativeWinManImpl::CreateImageReadySemaphore(
	DeviceDispatch const& device, 
	NativeWindowID windowID, 
	DebugUtilsDispatch const* debugUtils)
{
	vk::SemaphoreCreateInfo semaphoreInfo{};
	vk::ResultValue<vk::Semaphore> semaphoreResult = device.createSemaphore(semaphoreInfo);
	if (semaphoreResult.result != vk::Result::eSuccess)
		throw std::runtime_error("Unable to make semaphore.");
	vk::Semaphore returnVal = semaphoreResult.value;
	// Give name to the semaphore
	if (debugUtils != nullptr)
	{
		std::string name;
		name += "NativeWindow #";
		name += (int)windowID;
		name += " - Swapchain ImgReady Semaphore";
		debugUtils->Helper_SetObjectName(device.handle, returnVal, name.c_str());
	}
	return returnVal;
}

static void NativeWinManImpl::HandleCreationJobs(
	NativeWinMan& manager,
	GlobUtils const& globUtils)
{
	std::lock_guard createQueueLock{ manager.createQueueLock };

	vk::Result vkResult = {};

	// We need to go through each creation job and create them
	for (auto& createJob : manager.createJobs)
	{
		// Check if the ID already exists
		DENGINE_DETAIL_GFX_ASSERT(
			Std::FindIf(
				manager.nativeWindows.begin(),
				manager.nativeWindows.end(),
				[&createJob](auto const& val) -> bool { return val.id == createJob.id; }) == manager.nativeWindows.end());

		manager.nativeWindows.push_back({});

		auto& newNode = manager.nativeWindows.back();
		newNode.windowData.wsiConnection = createJob.windowConnection;

		u64 newSurface = 0;
		vkResult = (vk::Result)newNode.windowData.wsiConnection->CreateVkSurface(
			(uSize)(VkInstance)globUtils.instance.handle,
			nullptr,
			newSurface);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Could not create VkSurfaceKHR");
		newNode.windowData.surface = (vk::SurfaceKHR)(VkSurfaceKHR)newSurface;
		if (globUtils.UsingDebugUtils())
		{
			std::string name;
			name += "NativeWindow #";
			name += std::to_string((u64)createJob.id);
			name += " - Surface";
			globUtils.debugUtils.Helper_SetObjectName(globUtils.device.handle, newNode.windowData.surface, name.c_str());
		}

		bool physDeviceSupportsPresentation = globUtils.instance.getPhysicalDeviceSurfaceSupportKHR(
			globUtils.physDevice.handle,
			globUtils.queues.graphics.FamilyIndex(),
			newNode.windowData.surface);
		if (!physDeviceSupportsPresentation)
			throw std::runtime_error("DEngine - Vulkan: Physical device queue family does not support this surface.");

		NativeWinManImpl::SwapchainSettings swapchainSettings = NativeWinManImpl::BuildSwapchainSettings(
			globUtils.instance,
			globUtils.physDevice.handle,
			newNode.windowData.surface,
			globUtils.surfaceInfo);

		newNode.windowData.swapchain = NativeWinManImpl::CreateSwapchain(
			globUtils.device,
			createJob.id,
			newNode.windowData.surface,
			swapchainSettings,
			newNode.windowData.swapchain,
			globUtils.DebugUtilsPtr());

		newNode.windowData.swapchainImages = NativeWinManImpl::GetSwapchainImages(
			globUtils.device,
			createJob.id,
			newNode.windowData.swapchain,
			globUtils.DebugUtilsPtr());

		newNode.windowData.swapchainImgReadySem = NativeWinManImpl::CreateImageReadySemaphore(
			globUtils.device,
			createJob.id,
			globUtils.DebugUtilsPtr());
	}
	manager.createJobs.clear();
}

void NativeWinManImpl::HandleWindowResize(
	NativeWinMan& manager,
	GlobUtils const& globUtils,
	NativeWinMan::Node& windowNode)
{
	auto& windowData = windowNode.windowData;

	NativeWinManImpl::SwapchainSettings swapchainSettings = NativeWinManImpl::BuildSwapchainSettings(
		globUtils.instance,
		globUtils.physDevice.handle,
		windowData.surface,
		globUtils.surfaceInfo);

	// We need to resize this native-window and it's GUI.

	vk::SwapchainKHR oldSwapchain = windowData.swapchain;
	windowData.swapchain = NativeWinManImpl::CreateSwapchain(
		globUtils.device,
		windowNode.id,
		windowData.surface,
		swapchainSettings,
		oldSwapchain,
		globUtils.DebugUtilsPtr());
	if (oldSwapchain != vk::SwapchainKHR())
		globUtils.delQueue.Destroy(oldSwapchain);

	// No need to delete the old image-handles, they belong to the swapchain.
	windowData.swapchainImages = NativeWinManImpl::GetSwapchainImages(
		globUtils.device,
		windowNode.id,
		windowData.swapchain,
		globUtils.DebugUtilsPtr());

	for (uSize i = 0; i < windowData.swapchainImgViews.Size(); i += 1)
	{
		globUtils.delQueue.Destroy(windowData.framebuffers[i]);
		globUtils.delQueue.Destroy(windowData.swapchainImgViews[i]);
	}
	windowData.framebuffers.Clear();
	windowData.swapchainImgViews.Clear();

	windowData.swapchainImgViews = NativeWinManImpl::CreateSwapchainImgViews(
		globUtils.device,
		windowNode.id,
		swapchainSettings.surfaceFormat.format,
		windowData.swapchainImages,
		globUtils.DebugUtilsPtr());

	windowData.framebuffers = NativeWinManImpl::CreateSwapchainFramebuffers(
		globUtils.device,
		windowNode.id,
		windowData.swapchainImgViews,
		globUtils.guiRenderPass,
		swapchainSettings.extents,
		globUtils.DebugUtilsPtr());

	WindowGuiData& gui = windowNode.gui;
	gui.extent = swapchainSettings.extents;
	gui.rotation = NativeWinManImpl::RotationToMatrix(swapchainSettings.transform);
	gui.surfaceRotation = swapchainSettings.transform;
}

void NativeWinManImpl::HandleWindowRestore(
	NativeWinMan& manager,
	GlobUtils const& globUtils,
	NativeWinMan::Node& windowNode)
{
	auto& windowData = windowNode.windowData;

	globUtils.device.waitIdle();

	// We have to destroy the previous swapchain and it's surface because
	// Android destroys all the surface resources when the window is minimized
	globUtils.device.destroy(windowData.swapchain);
	windowData.swapchain = vk::SwapchainKHR{};
	globUtils.instance.destroy(windowData.surface);
	windowData.surface = vk::SurfaceKHR{};

	vk::SurfaceKHR newSurface{};
	vk::Result result = (vk::Result)windowNode.windowData.wsiConnection->CreateVkSurface(
		(uSize)(VkInstance)globUtils.instance.handle,
		nullptr,
		*reinterpret_cast<u64*>(&newSurface));
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create new suface when restoring window.");

	// Check that our surface works with our device
	bool surfaceSupported = globUtils.instance.getPhysicalDeviceSurfaceSupportKHR(
		globUtils.physDevice.handle,
		globUtils.queues.graphics.FamilyIndex(),
		newSurface);
	if (!surfaceSupported)
		throw std::runtime_error("DEngine - Vulkan: New SurfaceKHR not supported by device.");

	windowNode.windowData.surface = newSurface;

	HandleWindowResize(
		manager,
		globUtils,
		windowNode);
}
