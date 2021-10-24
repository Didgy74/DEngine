#include "NativeWindowManager.hpp"

#include "Vk.hpp"
#include "DeletionQueue.hpp"
#include "QueueData.hpp"

#include <DEngine/Math/Common.hpp>
#include <DEngine/Std/FrameAllocator.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Std/Containers/Vec.hpp>

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
		NativeWindowManager& manager,
		GlobUtils const& globUtils,
		Std::FrameAlloc& transientAlloc);

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
	Std::FrameAlloc& transientAlloc,
	Std::Span<NativeWindowUpdate const> windowUpdates)
{
	NativeWinManImpl::HandleCreationJobs(
		manager,
		globUtils,
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
	InitInfo const& initInfo)
{
	PushCreateWindowJob(*initInfo.manager, initInfo.initialWindow);
}

void NativeWinMan::PushCreateWindowJob(
	NativeWinMan& manager,
	NativeWindowID windowId) noexcept
{
	CreateJob newJob = {};
	newJob.id = windowId;

	std::lock_guard _{ manager.insertionJobs.lock };

	manager.insertionJobs.queue.push_back(newJob);
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

auto NativeWinManImpl::GetSwapchainImages(
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

Std::StackVec<vk::ImageView, Const::maxSwapchainLength> NativeWinManImpl::CreateSwapchainImgViews(
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
	GlobUtils const& globUtils,
	Std::FrameAllocator& transientAlloc)
{
	auto const* debugUtils = globUtils.DebugUtilsPtr();

	// Copy the jobs over so we can release the mutex early
	auto createJobs = Std::Vec<NativeWinMan::CreateJob, Std::FrameAlloc>(transientAlloc);
	{
		std::lock_guard createQueueLock{ manager.insertionJobs.lock };
		createJobs.Resize(manager.insertionJobs.queue.size());
		for (auto i = 0; i < createJobs.Size(); i += 1)
			createJobs[i] = manager.insertionJobs.queue[i];
		manager.insertionJobs.queue.clear();
	}

	vk::Result vkResult = {};

	// We need to go through each creation job and create them
	for (auto const& createJob : createJobs)
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

		NativeWinManImpl::SwapchainSettings swapchainSettings = NativeWinManImpl::BuildSwapchainSettings(
			globUtils.instance,
			globUtils.physDevice.handle,
			newNode.windowData.surface,
			globUtils.surfaceInfo);

		auto const oldSwapchain = newNode.windowData.swapchain;
		newNode.windowData.swapchain = NativeWinManImpl::CreateSwapchain(
			globUtils.device,
			createJob.id,
			newNode.windowData.surface,
			swapchainSettings,
			oldSwapchain,
			debugUtils);
		auto const& swapchain = newNode.windowData.swapchain;

		newNode.windowData.swapchainImages = NativeWinManImpl::GetSwapchainImages(
			globUtils.device,
			createJob.id,
			newNode.windowData.swapchain,
			debugUtils);
		auto const& swapchainImgs = newNode.windowData.swapchainImages;

		newNode.windowData.swapchainImgViews = NativeWinManImpl::CreateSwapchainImgViews(
			globUtils.device,
			newNode.id,
			swapchainSettings.surfaceFormat.format,
			swapchainImgs,
			debugUtils);
		auto const& swapchainImgViews = newNode.windowData.swapchainImgViews;

		newNode.windowData.framebuffers = NativeWinManImpl::CreateSwapchainFramebuffers(
			globUtils.device,
			newNode.id,
			swapchainImgViews,
			globUtils.guiRenderPass,
			swapchainSettings.extents,
			debugUtils);

		newNode.windowData.swapchainImgReadySem = NativeWinManImpl::CreateImageReadySemaphore(
			globUtils.device,
			createJob.id,
			debugUtils);

		WindowGuiData& gui = newNode.gui;
		gui.extent = swapchainSettings.extents;
		gui.rotation = NativeWinManImpl::RotationToMatrix(swapchainSettings.transform);
		gui.surfaceRotation = swapchainSettings.transform;
	}
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

	auto createSurfaceResult = globUtils.wsiInterface->CreateVkSurface(
		windowNode.id,
		(uSize)(VkInstance)globUtils.instance.handle,
		nullptr);

	vk::Result result = (vk::Result)createSurfaceResult.vkResult;
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to create new suface when restoring window.");

	vk::SurfaceKHR newSurface = (vk::SurfaceKHR)(VkSurfaceKHR)createSurfaceResult.vkSurface;

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
