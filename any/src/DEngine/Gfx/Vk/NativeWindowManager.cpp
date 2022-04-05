#include "NativeWindowManager.hpp"

#include "Vk.hpp"
#include "DeletionQueue.hpp"
#include "QueueData.hpp"

#include <DEngine/Math/Common.hpp>
#include <DEngine/Std/FrameAllocator.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Std/Containers/Vec.hpp>

#include <string>

namespace DEngine::Gfx::Vk::NativeWinMgrImpl
{
	struct SwapchainSettings
	{
		vk::PresentModeKHR presentMode = {};
		vk::SurfaceFormatKHR surfaceFormat = {};
		vk::SurfaceTransformFlagBitsKHR transform = {};
		vk::CompositeAlphaFlagBitsKHR compositeAlphaFlag = {};
		vk::Extent2D extents = {};
		u32 numImages = {};
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
			return Math::Mat2{ 1, 0, 0, 1 };
		case vk::SurfaceTransformFlagBitsKHR::eRotate90:
			return Math::Mat2{ 0, -1, 1, 0 };
		case vk::SurfaceTransformFlagBitsKHR::eRotate180:
			return Math::Mat2{ -1, 0, 0, -1 };
		case vk::SurfaceTransformFlagBitsKHR::eRotate270:
			return Math::Mat2{ 0, 1, -1, 0 };
		default:
			DENGINE_IMPL_GFX_UNREACHABLE();
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
		NativeWinMgr& manager,
		GlobUtils const& globUtils,
		Std::FrameAlloc& transientAlloc);

	static void HandleDeletionJobs(
		NativeWinMgr& manager,
		GlobUtils const& globUtils,
		DeletionQueue& delQueue,
		Std::FrameAlloc& transientAlloc);

	static void HandleWindowResize(
		NativeWinMgr& manager,
		GlobUtils const& globUtils,
		DelQueue& delQueue,
		NativeWinMgr::Node& windowNode);

	static void HandleWindowRestore(
		NativeWinMgr& manager,
		GlobUtils const& globUtils,
		DelQueue& delQueue,
		NativeWinMgr::Node& windowNode);
}

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

void NativeWinMgr::ProcessEvents(
	NativeWinMgr& manager,
	GlobUtils const& globUtils,
	DeletionQueue& delQueue,
	Std::FrameAlloc& transientAlloc,
	Std::Span<NativeWindowUpdate const> windowUpdates)
{
	NativeWinMgrImpl::HandleCreationJobs(
		manager,
		globUtils,
		transientAlloc);
	NativeWinMgrImpl::HandleDeletionJobs(
		manager,
		globUtils,
		delQueue,
		transientAlloc);

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
			manager.main.nativeWindows.begin(),
			manager.main.nativeWindows.end(),
			[&item](auto const& val) { return item.id == val.id; });
		DENGINE_IMPL_GFX_ASSERT(windowNodeIt != manager.main.nativeWindows.end());
		auto& windowNode = *windowNodeIt;

		switch (item.event)
		{
		case NativeWindowEvent::Resize:
			NativeWinMgrImpl::HandleWindowResize(
				manager,
				globUtils,
				delQueue,
				windowNode);
			break;
		case NativeWindowEvent::Restore:
			NativeWinMgrImpl::HandleWindowRestore(
				manager,
				globUtils,
				delQueue,
				windowNode);
			break;
		default:
			break;
		}
	}
}

void NativeWinMgr::Initialize(
	InitInfo const& initInfo)
{
	NativeWinMgr_PushCreateWindowJob(
		initInfo.manager,
		initInfo.initialWindow);
}

void NativeWinMgr::Destroy(
	NativeWinMgr& manager,
	InstanceDispatch const& instance,
	DeviceDispatch const& device)
{
	for (auto& element : manager.main.nativeWindows)
	{
		auto& windowData = element.windowData;

		for (auto fb : windowData.framebuffers)
			device.Destroy(fb);

		for (auto imgView : windowData.swapchainImgViews)
			device.Destroy(imgView);

		device.Destroy(windowData.swapchain);

		device.Destroy(windowData.swapchainImgReadySem);

		instance.Destroy(windowData.surface);
	}

	manager.main.nativeWindows.clear();
}

void Vk::NativeWinMgr_PushCreateWindowJob(
	NativeWinMgr& manager,
	NativeWindowID windowId)
{
	NativeWinMgr::CreateJob newJob = {};
	newJob.id = windowId;

	std::scoped_lock _{ manager.insertionJobs.lock };

	manager.insertionJobs.createQueue.push_back(newJob);
}

void Vk::NativeWinMgr_PushDeleteWindowJob(
	NativeWinMgr& manager,
	NativeWindowID windowId)
{
	NativeWinMgr::DeleteJob newJob = {};
	newJob.id = windowId;

	std::scoped_lock _{ manager.insertionJobs.lock };

	manager.insertionJobs.deleteQueue.push_back(newJob);
}

static vk::SwapchainKHR NativeWinMgrImpl::CreateSwapchain(
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

auto NativeWinMgrImpl::GetSwapchainImages(
	DeviceDispatch const& device, 
	NativeWindowID windowID, 
	vk::SwapchainKHR swapchain, 
	DebugUtilsDispatch const* debugUtils)
	-> Std::StackVec<vk::Image, Constants::maxSwapchainLength>
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

Std::StackVec<vk::ImageView, Const::maxSwapchainLength> NativeWinMgrImpl::CreateSwapchainImgViews(
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

Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> NativeWinMgrImpl::CreateSwapchainFramebuffers(
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

vk::Semaphore NativeWinMgrImpl::CreateImageReadySemaphore(
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

static void NativeWinMgrImpl::HandleCreationJobs(
	NativeWinMgr& manager,
	GlobUtils const& globUtils,
	Std::FrameAllocator& transientAlloc)
{
	auto const* debugUtils = globUtils.DebugUtilsPtr();

	// Copy the jobs over so we can release the mutex early
	auto tempCreateJobs = Std::Vec<NativeWinMgr::CreateJob, Std::FrameAlloc>{ transientAlloc };
	{
		std::scoped_lock lock{ manager.insertionJobs.lock };
		tempCreateJobs.Resize(manager.insertionJobs.createQueue.size());
		for (auto i = 0; i < tempCreateJobs.Size(); i += 1)
			tempCreateJobs[i] = manager.insertionJobs.createQueue[i];
		manager.insertionJobs.createQueue.clear();
	}

	vk::Result vkResult = {};

	// We need to go through each creation job and create them
	for (auto const& createJob : tempCreateJobs)
	{
		// Check if the ID already exists
		[[maybe_unused]] auto idExists = [&manager](NativeWindowID id) {
			return Std::AnyOf(
				manager.main.nativeWindows.begin(),
				manager.main.nativeWindows.end(),
				[id](auto const& item) { return item.id == id; });
		};
		DENGINE_IMPL_GFX_ASSERT(!idExists(createJob.id));

		manager.main.nativeWindows.push_back({});

		auto& newNode = manager.main.nativeWindows.back();
		newNode.id = createJob.id;

		auto createSurfaceResult = globUtils.wsiInterface->CreateVkSurface(
			createJob.id,
			(uSize)(VkInstance)globUtils.instance.handle,
			nullptr);
		vkResult = (vk::Result)createSurfaceResult.vkResult;
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Could not create VkSurfaceKHR");

		newNode.windowData.surface = (vk::SurfaceKHR)(VkSurfaceKHR)createSurfaceResult.vkSurface;
		if (debugUtils)
		{
			std::string name;
			name += "NativeWindow #";
			name += std::to_string((u64)createJob.id);
			name += " - Surface";
			debugUtils->Helper_SetObjectName(globUtils.device.handle, newNode.windowData.surface, name.c_str());
		}

		bool const physDeviceSupportsPresentation = globUtils.instance.getPhysicalDeviceSurfaceSupportKHR(
			globUtils.physDevice.handle,
			globUtils.queues.graphics.FamilyIndex(),
			newNode.windowData.surface);
		if (!physDeviceSupportsPresentation)
			throw std::runtime_error("DEngine - Vulkan: Physical device queue family does not support this surface.");

		NativeWinMgrImpl::SwapchainSettings swapchainSettings = NativeWinMgrImpl::BuildSwapchainSettings(
			globUtils.instance,
			globUtils.physDevice.handle,
			newNode.windowData.surface,
			globUtils.surfaceInfo);

		auto const oldSwapchain = newNode.windowData.swapchain;
		newNode.windowData.swapchain = NativeWinMgrImpl::CreateSwapchain(
			globUtils.device,
			createJob.id,
			newNode.windowData.surface,
			swapchainSettings,
			oldSwapchain,
			debugUtils);
		auto const& swapchain = newNode.windowData.swapchain;

		newNode.windowData.swapchainImages = NativeWinMgrImpl::GetSwapchainImages(
			globUtils.device,
			createJob.id,
			newNode.windowData.swapchain,
			debugUtils);
		auto const& swapchainImgs = newNode.windowData.swapchainImages;

		newNode.windowData.swapchainImgViews = NativeWinMgrImpl::CreateSwapchainImgViews(
			globUtils.device,
			newNode.id,
			swapchainSettings.surfaceFormat.format,
			swapchainImgs,
			debugUtils);
		auto const& swapchainImgViews = newNode.windowData.swapchainImgViews;

		newNode.windowData.framebuffers = NativeWinMgrImpl::CreateSwapchainFramebuffers(
			globUtils.device,
			newNode.id,
			swapchainImgViews,
			globUtils.guiRenderPass,
			swapchainSettings.extents,
			debugUtils);

		newNode.windowData.swapchainImgReadySem = NativeWinMgrImpl::CreateImageReadySemaphore(
			globUtils.device,
			createJob.id,
			debugUtils);

		auto& gui = newNode.gui;
		gui.extent = swapchainSettings.extents;
		gui.rotation = NativeWinMgrImpl::RotationToMatrix(swapchainSettings.transform);
		gui.surfaceRotation = swapchainSettings.transform;
	}
}

static void NativeWinMgrImpl::HandleDeletionJobs(
	NativeWinMgr& manager,
	GlobUtils const& globUtils,
	DeletionQueue& delQueue,
	Std::FrameAlloc& transientAlloc)
{
	// Copy the jobs over so we can release the mutex early
	auto tempDeleteJobs = Std::Vec<NativeWinMgr::DeleteJob, Std::FrameAlloc>{ transientAlloc };
	{
		std::lock_guard lock{ manager.insertionJobs.lock };
		tempDeleteJobs.Resize(manager.insertionJobs.deleteQueue.size());
		for (auto i = 0; i < tempDeleteJobs.Size(); i += 1)
			tempDeleteJobs[i] = manager.insertionJobs.deleteQueue[i];
		manager.insertionJobs.deleteQueue.clear();
	}

	auto& nativeWindows = manager.main.nativeWindows;

	for (auto const& deleteJob : tempDeleteJobs)
	{
		auto const windowNodeIt = Std::FindIf(
			nativeWindows.begin(),
			nativeWindows.end(),
			[&deleteJob](auto const& val) { return val.id == deleteJob.id; });
		DENGINE_IMPL_GFX_ASSERT(windowNodeIt != nativeWindows.end());
		auto windowNode = Std::Move(*windowNodeIt);
		nativeWindows.erase(windowNodeIt);

		delQueue.Destroy(windowNode.windowData.framebuffers);
		delQueue.Destroy(windowNode.windowData.swapchainImgViews);
		delQueue.Destroy(windowNode.windowData.swapchainImgReadySem);
		delQueue.Destroy(windowNode.windowData.swapchain);
		delQueue.Destroy(windowNode.windowData.surface);
	}
}

void NativeWinMgrImpl::HandleWindowResize(
	NativeWinMgr& manager,
	GlobUtils const& globUtils,
	DelQueue& delQueue,
	NativeWinMgr::Node& windowNode)
{
	auto& windowData = windowNode.windowData;

	NativeWinMgrImpl::SwapchainSettings swapchainSettings = NativeWinMgrImpl::BuildSwapchainSettings(
		globUtils.instance,
		globUtils.physDevice.handle,
		windowData.surface,
		globUtils.surfaceInfo);

	// We need to resize this native-window and it's GUI.

	vk::SwapchainKHR oldSwapchain = windowData.swapchain;
	windowData.swapchain = NativeWinMgrImpl::CreateSwapchain(
		globUtils.device,
		windowNode.id,
		windowData.surface,
		swapchainSettings,
		oldSwapchain,
		globUtils.DebugUtilsPtr());
	if (oldSwapchain != vk::SwapchainKHR())
		delQueue.Destroy(oldSwapchain);

	// No need to delete the old image-handles, they belong to the swapchain.
	windowData.swapchainImages = NativeWinMgrImpl::GetSwapchainImages(
		globUtils.device,
		windowNode.id,
		windowData.swapchain,
		globUtils.DebugUtilsPtr());

	for (uSize i = 0; i < windowData.swapchainImgViews.Size(); i += 1)
	{
		delQueue.Destroy(windowData.framebuffers[i]);
		delQueue.Destroy(windowData.swapchainImgViews[i]);
	}
	windowData.framebuffers.Clear();
	windowData.swapchainImgViews.Clear();

	windowData.swapchainImgViews = NativeWinMgrImpl::CreateSwapchainImgViews(
		globUtils.device,
		windowNode.id,
		swapchainSettings.surfaceFormat.format,
		windowData.swapchainImages,
		globUtils.DebugUtilsPtr());

	windowData.framebuffers = NativeWinMgrImpl::CreateSwapchainFramebuffers(
		globUtils.device,
		windowNode.id,
		windowData.swapchainImgViews,
		globUtils.guiRenderPass,
		swapchainSettings.extents,
		globUtils.DebugUtilsPtr());

	auto& gui = windowNode.gui;
	gui.extent = swapchainSettings.extents;
	gui.rotation = NativeWinMgrImpl::RotationToMatrix(swapchainSettings.transform);
	gui.surfaceRotation = swapchainSettings.transform;
}

void NativeWinMgrImpl::HandleWindowRestore(
	NativeWinMgr& manager,
	GlobUtils const& globUtils,
	DelQueue& delQueue,
	NativeWinMgr::Node& windowNode)
{
	auto& windowData = windowNode.windowData;
	auto const& instance = globUtils.instance;
	auto const& device = globUtils.device;

	device.waitIdle();

	// We have to destroy the previous swapchain and it's surface because
	// Android destroys all the surface resources when the window is minimized
	device.Destroy(windowData.swapchain);
	windowData.swapchain = vk::SwapchainKHR{};
	instance.Destroy(windowData.surface);
	windowData.surface = vk::SurfaceKHR{};

	auto createSurfaceResult = globUtils.wsiInterface->CreateVkSurface(
		windowNode.id,
		(uSize)(VkInstance)instance.handle,
		nullptr);

	auto result = (vk::Result)createSurfaceResult.vkResult;
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create new suface when restoring window.");

	auto newSurface = (vk::SurfaceKHR)(VkSurfaceKHR)createSurfaceResult.vkSurface;

	// Check that our surface works with our device
	bool surfaceSupported = instance.getPhysicalDeviceSurfaceSupportKHR(
		globUtils.physDevice.handle,
		globUtils.queues.graphics.FamilyIndex(),
		newSurface);
	if (!surfaceSupported)
		throw std::runtime_error("DEngine - Vulkan: New SurfaceKHR not supported by device.");

	windowNode.windowData.surface = newSurface;

	HandleWindowResize(
		manager,
		globUtils,
		delQueue,
		windowNode);
}
