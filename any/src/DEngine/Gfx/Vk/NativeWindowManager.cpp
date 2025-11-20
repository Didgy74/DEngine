#include "NativeWindowManager.hpp"

#include "Vk.hpp"
#include "DeletionQueue.hpp"
#include "QueueData.hpp"
#include "Utilities.hpp"

#include <DEngine/Std/Containers/AllocRef.hpp>

#include <string>
#include <format>

import DEngine.Std.Vec;

namespace DEngine::Gfx::Vk::NativeWinMgrImpl
{
	// If extentHint is set, it will be used for the swapchain.
	// If not, the swapchain "currentExtent" will be used.
	[[nodiscard]] static NativeWinMgr_SwapchainSettings BuildSwapchainSettings(
		InstanceDispatch const& instance,
		vk::PhysicalDevice physDevice,
		vk::SurfaceKHR surface,
		Std::Opt<vk::Extent2D> const& extentHint,
		SurfaceInfo const& surfaceInfo)
	{
		auto surfaceCaps = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface);

		NativeWinMgr_SwapchainSettings temp = {};
		temp.compositeAlphaFlag = surfaceInfo.compositeAlphaToUse;

		if (extentHint.Has())
			temp.extents = extentHint.Get();
		else
			temp.extents = surfaceCaps.currentExtent;

		if (surfaceCaps.currentTransform == vk::SurfaceTransformFlagBitsKHR::eRotate90 ||
			surfaceCaps.currentTransform == vk::SurfaceTransformFlagBitsKHR::eRotate270)
			Std::Swap(temp.extents.width, temp.extents.height);


		// Handle swapchainData length
		u32 swapchainLength = Constants::preferredSwapchainLength;
		// If we need to, clamp the swapchainData length.
		// Upper clamp only applies if maxImageCount != 0
		if (surfaceCaps.maxImageCount != 0)
			swapchainLength = Std::Clamp(swapchainLength, surfaceCaps.minImageCount, surfaceCaps.maxImageCount);
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
		NativeWinMgr_SwapchainSettings settings,
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

	struct CreateDepthStencilAttachment_Result {
		BoxVmaImg img = {};
		BoxVkImageView imgView = {};
	};
	[[nodiscard]] static CreateDepthStencilAttachment_Result CreateDepthStencilAttachment(
		DeviceDispatch const& device,
		VmaAllocator const& vma,
		NativeWindowID windowId,
		vk::Format format,
		vk::Extent2D extent,
		DebugUtilsDispatch const* debugUtils);

	[[nodiscard]] static Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> CreateSwapchainFramebuffers(
		DeviceDispatch const& device,
		NativeWindowID windowId,
		Std::Span<vk::ImageView const> imgViews,
		vk::ImageView depthStencilImgView,
		vk::RenderPass renderPass,
		vk::Extent2D extents,
		DebugUtilsDispatch const* debugUtils);

	static void HandleCreationJobs(
		NativeWinMgr& manager,
		GlobUtils const& globUtils,
		Std::AllocRef const& transientAlloc);

	static void HandleDeletionJobs(
		NativeWinMgr& manager,
		GlobUtils const& globUtils,
		DelQueue& delQueue,
		Std::AllocRef const& transientAlloc);

	// Completely rebuilds the swapchain with related resources.
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

namespace DEngine::Gfx::Vk {
	[[nodiscard]] static bool HasAnyResizeEvents(Std::Span<NativeWindowUpdate const> const& windowUpdates) {
		for (auto const& item : windowUpdates)
		{
			switch (item.event)
			{
				case NativeWindowEvent::Resize:
				case NativeWindowEvent::Restore:
					return true;
				default: {}
			}
		}
		return false;
	}
}

void NativeWinMgr::ProcessEvents(
	NativeWinMgr& manager,
	GlobUtils const& globUtils,
	DeletionQueue& delQueue,
	Std::AllocRef const& transientAlloc,
	Std::Span<NativeWindowUpdate const> windowUpdates)
{
	auto const& device = globUtils.device;

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
	bool needToStall = HasAnyResizeEvents(windowUpdates);
	if (needToStall)
		device.waitIdle();

	for (auto const& item : windowUpdates) {
		auto const windowNodeIt = Std::FindIf(
			manager.main.nativeWindows.begin(),
			manager.main.nativeWindows.end(),
			[&item](auto const& val) { return item.id == val.id; });
		DENGINE_IMPL_GFX_ASSERT(windowNodeIt != manager.main.nativeWindows.end());
		auto& windowNode = *windowNodeIt;
		switch (item.event) {
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

	for (auto& windowNode : manager.main.nativeWindows) {
		auto& windowData = windowNode.windowData;
		if (!windowData.tagOutOfDate)
			continue;

		NativeWinMgrImpl::HandleWindowResize(
			manager,
			globUtils,
			delQueue,
			windowNode);
	}
}

void NativeWinMgr::Initialize(
	InitInfo const& initInfo)
{
	NativeWinMgr_PushCreateWindowJob(
		initInfo.manager,
		initInfo.initialWindow,
		initInfo.surface,
		initInfo.sizeHint);
}

void NativeWinMgr::Destroy(
	NativeWinMgr& manager,
	InstanceDispatch const& instance,
	DeviceDispatch const& device)
{
	for (auto& element : manager.main.nativeWindows) {
		auto& windowData = element.windowData;

		for (auto fb : windowData.framebuffers)
			device.Destroy(fb);

		for (auto imgView : windowData.swapchainImgViews)
			device.Destroy(imgView);

		device.Destroy(windowData.swapchain);

		instance.Destroy(windowData.surface);
	}

	manager.main.nativeWindows.clear();
}

void Vk::NativeWinMgr_PushCreateWindowJob(
	NativeWinMgr& manager,
	NativeWindowID windowId,
	Std::Opt<vk::SurfaceKHR> const& surface,
	Std::Opt<vk::Extent2D> const& sizeHint)
{
	NativeWinMgr::CreateJob newJob = {};
	newJob.id = windowId;
	newJob.surface = surface;
	newJob.sizeHint = sizeHint;

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
	NativeWinMgr_SwapchainSettings settings,
	vk::SwapchainKHR oldSwapchain,
	DebugUtilsDispatch const* debugUtils)
{
	vk::SwapchainCreateInfoKHR swapchainCreateInfo = {};
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
	auto swapchain = device.createSwapchainKHR(swapchainCreateInfo);
	if (debugUtils) {
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
	vk::Result vkResult = {};
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
	if (debugUtils) {
		for (uSize i = 0; i < returnVal.Size(); i += 1) {
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
	for (uSize i = 0; i < returnVal.Size(); i += 1) {
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

		if (debugUtils) {
			auto name = std::format(
				"NativeWindow #{} - Swapchain ImgView#{}",
				(u64)windowId,
				i);
			debugUtils->Helper_SetObjectName(device.handle, returnVal[i], name.c_str());
		}
	}
	return returnVal;
}

static auto NativeWinMgrImpl::CreateDepthStencilAttachment(
	DeviceDispatch const& device,
	VmaAllocator const& vma,
	NativeWindowID windowId,
	vk::Format format,
	vk::Extent2D extent,
	DebugUtilsDispatch const* debugUtils)
		-> CreateDepthStencilAttachment_Result
{
	vk::ImageCreateInfo imgCreateInfo = {};
	imgCreateInfo.flags = {};
	imgCreateInfo.imageType = vk::ImageType::e2D;
	imgCreateInfo.format = vk::Format::eS8Uint;
	imgCreateInfo.extent = vk::Extent3D{ extent.width, extent.height, 1 };
	imgCreateInfo.mipLevels = 1;
	imgCreateInfo.arrayLayers = 1;
	imgCreateInfo.samples = vk::SampleCountFlagBits::e1;
	imgCreateInfo.tiling = vk::ImageTiling::eOptimal;
	imgCreateInfo.usage =
		vk::ImageUsageFlagBits::eDepthStencilAttachment
		| vk::ImageUsageFlagBits::eTransientAttachment;
	imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	imgCreateInfo.queueFamilyIndexCount = 0;
	imgCreateInfo.pQueueFamilyIndices = nullptr;
	imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocCreateInfo.preferredFlags = (VkMemoryPropertyFlags)vk::MemoryPropertyFlagBits::eLazilyAllocated;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	auto imgResult = device.CreateBox(vma, imgCreateInfo, allocCreateInfo);
	if (debugUtils) {
		auto name = std::format("NativeWindow #{} - Depth Stencil Image", (int)windowId);
		debugUtils->Helper_SetObjectName(device.handle, imgResult.img.Handle(), name.c_str());
	}

	vk::ImageViewCreateInfo imgViewInfo = {};
	imgViewInfo.components = vk::ComponentSwizzle::eIdentity;
	imgViewInfo.format = vk::Format::eS8Uint;
	imgViewInfo.image = imgResult.img.Handle();
	imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eStencil;
	imgViewInfo.subresourceRange.baseArrayLayer = 0;
	imgViewInfo.subresourceRange.baseMipLevel = 0;
	imgViewInfo.subresourceRange.layerCount = 1;
	imgViewInfo.subresourceRange.levelCount = 1;
	imgViewInfo.viewType = vk::ImageViewType::e2D;
	auto depthStencilImgView = device.CreateBox(imgViewInfo);
	if (debugUtils) {
		auto name = std::format("NativeWindow #{} - Depth Stencil Image View", (int)windowId);
		debugUtils->Helper_SetObjectName(device.handle, depthStencilImgView.Handle(), name.c_str());
	}

	return CreateDepthStencilAttachment_Result {
		.img = Std::Move(imgResult.img),
		.imgView = Std::Move(depthStencilImgView), };
}

Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> NativeWinMgrImpl::CreateSwapchainFramebuffers(
	DeviceDispatch const& device,
	NativeWindowID windowId, 
	Std::Span<vk::ImageView const> imgViews,
	vk::ImageView depthStencilImgView,
	vk::RenderPass renderPass,
	vk::Extent2D extents, 
	DebugUtilsDispatch const* debugUtils)
{
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(imgViews));
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(depthStencilImgView));
	DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(renderPass));

	Std::StackVec<vk::Framebuffer, Const::maxSwapchainLength> returnVal;
	returnVal.Resize(imgViews.Size());
	for (uSize i = 0; i < returnVal.Size(); i += 1) {
		Std::Array<vk::ImageView, 2> attachments = {
			imgViews[i],
			depthStencilImgView };

		vk::FramebufferCreateInfo fbInfo = {};
		fbInfo.width = extents.width;
		fbInfo.height = extents.height;
		fbInfo.layers = 1;
		fbInfo.renderPass = renderPass;
		fbInfo.attachmentCount = attachments.Size();
		fbInfo.pAttachments = attachments.Data();
		returnVal[i] = device.createFramebuffer(fbInfo);

		if (debugUtils) {
			auto name = std::format("NativeWindow #{} - Swapchain Framebuffer #{}", (u64)windowId, i);
			debugUtils->Helper_SetObjectName(device.handle, returnVal[i], name.c_str());
		}
	}
	return returnVal;
}

static void NativeWinMgrImpl::HandleCreationJobs(
	NativeWinMgr& manager,
	GlobUtils const& globUtils,
	Std::AllocRef const& transientAlloc)
{
	auto const& instance = globUtils.instance;
	auto const& device = globUtils.device;
	auto& wsiInterface = *globUtils.wsiInterface;
	auto& vma = globUtils.vma;
	auto const* debugUtils = globUtils.DebugUtilsPtr();

	// Copy the jobs over so we can release the mutex early
	auto tempCreateJobs = Std::NewVec<NativeWinMgr::CreateJob>(transientAlloc);
	{
		std::scoped_lock lock{ manager.insertionJobs.lock };
		tempCreateJobs.Resize(manager.insertionJobs.createQueue.size());
		for (auto i = 0; i < tempCreateJobs.Size(); i += 1)
			tempCreateJobs[i] = manager.insertionJobs.createQueue[i];
		manager.insertionJobs.createQueue.clear();
	}

	vk::Result vkResult = {};

	// We need to go through each creation job and create them
	for (auto const& createJob : tempCreateJobs) {
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
		auto& windowData = newNode.windowData;

		if (createJob.surface.Has()) {
			windowData.surface = createJob.surface.Get();
		} else {
			auto createSurfaceResult = wsiInterface.CreateVkSurface(
				createJob.id,
				(void const*)globUtils.baseDispatch.raw.vkGetInstanceProcAddr,
				(uSize)(VkInstance)instance.handle,
				nullptr);
			vkResult = (vk::Result)createSurfaceResult.vkResult;
			if (vkResult != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: Could not create VkSurfaceKHR");

			windowData.surface = (vk::SurfaceKHR)(VkSurfaceKHR)createSurfaceResult.vkSurface;
		}

		if (debugUtils) {
			auto name = std::format("NativeWindow #{} - Surface", (u64)createJob.id);
			debugUtils->Helper_SetObjectName(device.handle, windowData.surface, name.c_str());
		}

		bool const physDeviceSupportsPresentation = globUtils.instance.getPhysicalDeviceSurfaceSupportKHR(
			globUtils.physDevice.handle,
			globUtils.queues.graphics.FamilyIndex(),
			windowData.surface);
		if (!physDeviceSupportsPresentation)
			throw std::runtime_error("DEngine - Vulkan: Physical device queue family does not support this surface.");

		auto swapchainSettings = NativeWinMgrImpl::BuildSwapchainSettings(
			instance,
			globUtils.physDevice.handle,
			windowData.surface,
			createJob.sizeHint,
			globUtils.surfaceInfo);

		auto const oldSwapchain = windowData.swapchain;
		windowData.swapchain = NativeWinMgrImpl::CreateSwapchain(
			globUtils.device,
			createJob.id,
			windowData.surface,
			swapchainSettings,
			oldSwapchain,
			debugUtils);
		auto const& swapchain = windowData.swapchain;

		windowData.swapchainImages = NativeWinMgrImpl::GetSwapchainImages(
			globUtils.device,
			createJob.id,
			swapchain,
			debugUtils);
		auto const& swapchainImgs = windowData.swapchainImages;

		windowData.swapchainImgViews = NativeWinMgrImpl::CreateSwapchainImgViews(
			globUtils.device,
			newNode.id,
			swapchainSettings.surfaceFormat.format,
			swapchainImgs.ToSpan(),
			debugUtils);
		auto const& swapchainImgViews = windowData.swapchainImgViews;

		auto depthStencilResources = NativeWinMgrImpl::CreateDepthStencilAttachment(
			device,
			vma,
			newNode.id,
			vk::Format(),
			swapchainSettings.extents,
			debugUtils);
		windowData.depthStencilImg = Std::Move(depthStencilResources.img);
		windowData.depthStencilImgView = Std::Move(depthStencilResources.imgView);

		windowData.framebuffers = NativeWinMgrImpl::CreateSwapchainFramebuffers(
			device,
			newNode.id,
			swapchainImgViews.ToSpan(),
			windowData.depthStencilImgView.Handle(),
			globUtils.guiRenderPass,
			swapchainSettings.extents,
			debugUtils);

		// One image-acquire semaphore per frame-in-flight slot, indexed by mainFenceIndex.
		windowData.swapchainImgReadySems.Clear();
		for (int i = 0; i < globUtils.MainFencesCount(); i += 1) {
			vk::SemaphoreCreateInfo semaphoreInfo = {};
			auto semaphore = device.createSemaphore(semaphoreInfo);
			if (semaphore.result != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: Unable to create image-ready semaphore.");
			windowData.swapchainImgReadySems.PushBack(semaphore.value);
			if (debugUtils != nullptr) {
				auto name = std::format(
					"NativeWindow #{} - Swapchain ImgReady Semaphore #{}",
					(int)newNode.id,
					(int)i);
				debugUtils->Helper_SetObjectName(device.handle, semaphore.value, name.c_str());
			}
		}

		// One render-finished semaphore per swapchain image, indexed by acquired image index.
		windowData.renderFinishedSems.Clear();
		for (uSize i = 0; i < swapchainImgs.Size(); i += 1) {
			vk::SemaphoreCreateInfo semaphoreInfo = {};
			auto semaphore = device.createSemaphore(semaphoreInfo);
			if (semaphore.result != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: Unable to create render-finished semaphore.");
			windowData.renderFinishedSems.PushBack(semaphore.value);
			if (debugUtils != nullptr) {
				auto name = std::format(
					"NativeWindow #{} - Render Finished Semaphore #{}",
					(int)newNode.id,
					(int)i);
				debugUtils->Helper_SetObjectName(
					device.handle,
					semaphore.value,
					name.c_str());
			}
		}

		windowData.extent = swapchainSettings.extents;
		windowData.surfaceTransform = swapchainSettings.transform;
	}
}

static void NativeWinMgrImpl::HandleDeletionJobs(
	NativeWinMgr& manager,
	GlobUtils const& globUtils,
	DelQueue& delQueue,
	Std::AllocRef const& transientAlloc)
{
	// Copy the jobs over so we can release the mutex early
	auto tempDeleteJobs = Std::NewVec<NativeWinMgr::DeleteJob>(transientAlloc);
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

		delQueue.Destroy(windowNode.windowData.framebuffers.ToSpan());
		delQueue.Destroy(windowNode.windowData.swapchainImgViews.ToSpan());
		for (auto const semaphore : windowNode.windowData.swapchainImgReadySems)
			delQueue.Destroy(semaphore);
		for (auto const semaphore : windowNode.windowData.renderFinishedSems)
			delQueue.Destroy(semaphore);
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
	auto& instance = globUtils.instance;
	auto& device = globUtils.device;
	auto& vma = globUtils.vma;
	auto const& debugUtils = globUtils.DebugUtilsPtr();

	// TODO: Need to handle passing size hint during resize.
	auto swapchainSettings = NativeWinMgrImpl::BuildSwapchainSettings(
		instance,
		globUtils.physDevice.handle,
		windowData.surface,
		Std::nullOpt,
		globUtils.surfaceInfo);

	// We need to resize this native-window and its GUI.
	vk::SwapchainKHR oldSwapchain = windowData.swapchain;
	windowData.swapchain = NativeWinMgrImpl::CreateSwapchain(
		device,
		windowNode.id,
		windowData.surface,
		swapchainSettings,
		oldSwapchain,
		globUtils.DebugUtilsPtr());
	if (oldSwapchain != vk::SwapchainKHR())
		delQueue.Destroy(oldSwapchain);

	// No need to delete the old image-handles, they belong to the swapchain.
	windowData.swapchainImages = NativeWinMgrImpl::GetSwapchainImages(
		device,
		windowNode.id,
		windowData.swapchain,
		globUtils.DebugUtilsPtr());

	// Recreate the per-image render-finished semaphores. The image count may have changed, and the
	// old semaphores may still be pending against the now-replaced swapchain.
	for (auto const semaphore : windowData.renderFinishedSems)
		delQueue.Destroy(semaphore);
	windowData.renderFinishedSems.Clear();
	for (uSize i = 0; i < windowData.swapchainImages.Size(); i += 1) {
		vk::SemaphoreCreateInfo semaphoreInfo = {};
		auto semaphore = device.createSemaphore(semaphoreInfo);
		if (semaphore.result != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to create render-finished semaphore.");
		windowData.renderFinishedSems.PushBack(semaphore.value);
		if (debugUtils != nullptr) {
			auto name = std::format(
				"NativeWindow #{} - Render Finished Semaphore #{}",
				(int)windowNode.id,
				(int)i);
			debugUtils->Helper_SetObjectName(device.handle, semaphore.value, name.c_str());
		}
	}

	for (uSize i = 0; i < windowData.swapchainImgViews.Size(); i += 1) {
		delQueue.Destroy(windowData.framebuffers[i]);
		delQueue.Destroy(windowData.swapchainImgViews[i]);
	}
	windowData.framebuffers.Clear();
	windowData.swapchainImgViews.Clear();

	windowData.swapchainImgViews = NativeWinMgrImpl::CreateSwapchainImgViews(
		device,
		windowNode.id,
		swapchainSettings.surfaceFormat.format,
		windowData.swapchainImages.ToSpan(),
		globUtils.DebugUtilsPtr());

	delQueue.Destroy(Std::Move(windowData.depthStencilImgView));
	delQueue.Destroy(Std::Move(windowData.depthStencilImg));
	auto depthStencilResources = NativeWinMgrImpl::CreateDepthStencilAttachment(
		device,
		vma,
		windowNode.id,
		vk::Format(),
		swapchainSettings.extents,
		debugUtils);
	windowData.depthStencilImg = Std::Move(depthStencilResources.img);
	windowData.depthStencilImgView = Std::Move(depthStencilResources.imgView);

	windowData.framebuffers = NativeWinMgrImpl::CreateSwapchainFramebuffers(
		device,
		windowNode.id,
		windowData.swapchainImgViews.ToSpan(),
		windowData.depthStencilImgView.Handle(),
		globUtils.guiRenderPass,
		swapchainSettings.extents,
		globUtils.DebugUtilsPtr());

	windowData.extent = swapchainSettings.extents;
	windowData.surfaceTransform = swapchainSettings.transform;
	windowData.tagOutOfDate = false;
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
		(void const*)globUtils.baseDispatch.raw.vkGetInstanceProcAddr,
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

NativeWinMgr_WindowData const& NativeWinMgr::GetWindowData(NativeWindowID in) const {
	auto const windowDataIt = Std::FindIf(
		main.nativeWindows.begin(),
		main.nativeWindows.end(),
		[&](auto const& node) { return node.id == in; });
	DENGINE_IMPL_GFX_ASSERT(windowDataIt != main.nativeWindows.end());
	return windowDataIt->windowData;
}

NativeWinMgr_WindowData& NativeWinMgr::GetWindowData(NativeWindowID in) {
	auto const windowDataIt = Std::FindIf(
		main.nativeWindows.begin(),
		main.nativeWindows.end(),
		[&](auto const& node) { return node.id == in; });
	DENGINE_IMPL_GFX_ASSERT(windowDataIt != main.nativeWindows.end());
	return windowDataIt->windowData;
}

void NativeWinMgr::TagSwapchainOutOfDate(NativeWinMgr& mgr, NativeWindowID in) {
	auto& windowData = mgr.GetWindowData(in);
	windowData.tagOutOfDate = true;
}
