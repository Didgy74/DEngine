#include "NativeWindowManager.hpp"

#include "Vk.hpp"
#include "DeletionQueue.hpp"
#include "QueueData.hpp"

#include "DEngine/Math/Common.hpp"

#include <string>

using namespace DEngine;
using namespace DEngine::Gfx;

namespace DEngine::Gfx::Vk::NativeWindowManagerImpl
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

	static SwapchainSettings BuildSwapchainSettings(
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
			std::swap(temp.extents.width, temp.extents.height);


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
			throw std::runtime_error("DEngine - Vulkan: Unable to convert transform to a GUI rotation matrix.");
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

	static Std::StackVec<vk::Image, Constants::maxSwapchainLength> GetSwapchainImages(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		vk::SwapchainKHR swapchain,
		DebugUtilsDispatch const* debugUtils);

	static bool TransitionSwapchainImages(
		DeviceDispatch const& device,
		DeletionQueue const& deletionQueue,
		QueueData const& queues,
		Std::Span<const vk::Image> images);

	static Std::StackVec<vk::CommandBuffer, Constants::maxSwapchainLength> AllocateSwapchainCopyCmdBuffers(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		vk::CommandPool cmdPool,
		uSize swapchainImgCount,
		DebugUtilsDispatch const* debugUtils);

	static vk::Semaphore CreateImageReadySemaphore(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		DebugUtilsDispatch const* debugUtils);

	static void RecordSwapchainCmdBuffers(
		DeviceDispatch const& device,
		vk::Image srcImage,
		vk::Extent2D extents,
		Std::Span<vk::CommandBuffer const> cmdBuffers,
		Std::Span<vk::Image const> swapchainImages);

	struct CreateGuiImg_Result
	{
		vk::Image img;
		VmaAllocation alloc;
	};
	static CreateGuiImg_Result CreateGuiImg(
		DeviceDispatch const& device,
		VmaAllocator vma,
		NativeWindowID windowID,
		vk::Format format,
		vk::Extent2D extents,
		DebugUtilsDispatch const* debugUtils);

	static void TransitionGuiRenderTarget(
		DeviceDispatch const& device,
		QueueData const& queues,
		DeletionQueue const& deletionQueue,
		vk::Image img);

	static vk::ImageView CreateGuiImgView(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		vk::Image img,
		vk::Format format,
		DebugUtilsDispatch const* debugUtils);

	static vk::Framebuffer CreateGuiFramebuffer(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		vk::RenderPass renderPass,
		vk::Extent2D extent,
		vk::ImageView imgView,
		DebugUtilsDispatch const* debugUtils);

	static vk::CommandPool CreateGuiCmdPool(
		DeviceDispatch const& device,
		u32 queueFamilyIndex,
		NativeWindowID windowID,
		DebugUtilsDispatch const* debugUtils);

	static Std::StackVec<vk::CommandBuffer, Constants::maxInFlightCount> AllocGuiCmdBuffers(
		DeviceDispatch const& device,
		NativeWindowID windowID,
		u8 inFlightCount,
		vk::CommandPool cmdPool,
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

void Vk::NativeWindowManager::Update(
	NativeWindowManager& manager,
	GlobUtils const& globUtils,
	Std::Span<NativeWindowUpdate const> windowUpdates)
{
	// This code assumes the manager's lock is already locked.


	NativeWindowManagerImpl::HandleCreationJobs(
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
		auto& windowNode = *std::find_if(
			manager.nativeWindows.begin(),
			manager.nativeWindows.end(),
			[&item](Node const& val) -> bool {
				return item.id == val.id; });


		switch (item.event)
		{
		case NativeWindowEvent::Resize:
			NativeWindowManagerImpl::HandleWindowResize(
				manager,
				globUtils,
				windowNode);
			break;
		case NativeWindowEvent::Restore:
			NativeWindowManagerImpl::HandleWindowRestore(
				manager,
				globUtils,
				windowNode);
			break;
		default:
			break;
		}
	}
}

void Vk::NativeWindowManager::Initialize(
	NativeWindowManager& manager,
	DeviceDispatch const& device,
	QueueData const& queues,
	DebugUtilsDispatch const* debugUtils)
{
	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.queueFamilyIndex = queues.graphics.FamilyIndex();
	manager.copyToSwapchainCmdPool = device.createCommandPool(cmdPoolInfo);
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkCommandPool)manager.copyToSwapchainCmdPool;
		nameInfo.objectType = manager.copyToSwapchainCmdPool.objectType;
		std::string objectName = std::string("WindowManager - CopyImg CmdPool");
		nameInfo.pObjectName = objectName.data();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}	
}

void Vk::NativeWindowManager::BuildInitialNativeWindow(
	NativeWindowManager& manager,
	InstanceDispatch const& instance,
	DeviceDispatch const& device,
	vk::PhysicalDevice physDevice,
	QueueData const& queues,
	DeletionQueue const& deletionQueue,
	SurfaceInfo const& surfaceInfo,
	VmaAllocator vma,
	u8 inFlightCount,
	WsiInterface& initialWindowConnection,
	vk::SurfaceKHR surface,
	vk::RenderPass guiRenderPass,
	DebugUtilsDispatch const* debugUtils)
{

	NativeWindowData windowData{};
	windowData.wsiConnection = &initialWindowConnection;
	windowData.surface = surface;
	if (debugUtils)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkSurfaceKHR)windowData.surface;
		nameInfo.objectType = windowData.surface.objectType;
		std::string name = "NativeWindow #0 - Surface";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	NativeWindowManagerImpl::SwapchainSettings swapchainSettings = NativeWindowManagerImpl::BuildSwapchainSettings(
		instance,
		physDevice,
		surface,
		surfaceInfo);

	windowData.swapchain = NativeWindowManagerImpl::CreateSwapchain(
		device,
		NativeWindowID(0),
		windowData.surface,
		swapchainSettings,
		windowData.swapchain,
		debugUtils);

	windowData.swapchainImages = NativeWindowManagerImpl::GetSwapchainImages(
		device,
		(NativeWindowID)0,
		windowData.swapchain,
		debugUtils);

	NativeWindowManagerImpl::TransitionSwapchainImages(
		device,
		deletionQueue,
		queues,
		windowData.swapchainImages);

	windowData.copyCmdBuffers = NativeWindowManagerImpl::AllocateSwapchainCopyCmdBuffers(
		device,
		(NativeWindowID)0,
		manager.copyToSwapchainCmdPool,
		windowData.swapchainImages.Size(),
		debugUtils);

	windowData.swapchainImageReady = NativeWindowManagerImpl::CreateImageReadySemaphore(
		device,
		(NativeWindowID)0,
		debugUtils);

	WindowGuiData gui{};

	gui.extent = swapchainSettings.extents;
	gui.rotation = NativeWindowManagerImpl::RotationToMatrix(swapchainSettings.transform);
	gui.surfaceRotation = swapchainSettings.transform;

	vk::Format guiFormat = surfaceInfo.surfaceFormatToUse.format;
	auto createGuiImgResult = NativeWindowManagerImpl::CreateGuiImg(
		device,
		vma,
		(NativeWindowID)0,
		guiFormat,
		gui.extent,
		debugUtils);
	gui.img = createGuiImgResult.img;
	gui.vmaAllocation = createGuiImgResult.alloc;

	NativeWindowManagerImpl::RecordSwapchainCmdBuffers(
		device,
		gui.img,
		gui.extent,
		windowData.copyCmdBuffers,
		windowData.swapchainImages);

	NativeWindowManagerImpl::TransitionGuiRenderTarget(
		device,
		queues,
		deletionQueue,
		gui.img);

	gui.imgView = NativeWindowManagerImpl::CreateGuiImgView(
		device,
		(NativeWindowID)0,
		gui.img,
		guiFormat,
		debugUtils);

	gui.framebuffer = NativeWindowManagerImpl::CreateGuiFramebuffer(
		device,
		(NativeWindowID)0,
		guiRenderPass,
		gui.extent,
		gui.imgView,
		debugUtils);

	gui.cmdPool = NativeWindowManagerImpl::CreateGuiCmdPool(
		device,
		queues.graphics.FamilyIndex(),
		(NativeWindowID)0,
		debugUtils);

	gui.cmdBuffers = NativeWindowManagerImpl::AllocGuiCmdBuffers(
		device,
		(NativeWindowID)0,
		inFlightCount,
		gui.cmdPool,
		debugUtils);

	Node newNode{};
	newNode.id = NativeWindowID(0);
	newNode.windowData = windowData;
	newNode.gui = gui;
	manager.nativeWindows.push_back(newNode);
	manager.nativeWindowIdTracker += 1;
}

Gfx::NativeWindowID Vk::NativeWindowManager::PushCreateWindowJob(
	NativeWindowManager& manager, 
	WsiInterface& windowConnection)
{
	std::lock_guard _{ manager.lock };

	CreateJob newJob{};
	newJob.id = (NativeWindowID)manager.nativeWindowIdTracker;
	manager.nativeWindowIdTracker++;
	newJob.windowConnection = &windowConnection;

	manager.createJobs.push_back(newJob);

	return newJob.id;
}

static vk::SwapchainKHR Vk::NativeWindowManagerImpl::CreateSwapchain(
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
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
	swapchainCreateInfo.minImageCount = settings.numImages;
	swapchainCreateInfo.oldSwapchain = oldSwapchain;
	vk::SwapchainKHR swapchain = device.createSwapchainKHR(swapchainCreateInfo);
	if (debugUtils)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkSwapchainKHR)swapchain;
		nameInfo.objectType = swapchain.objectType;
		std::string name;
		name += "NativeWindow #";
		name += std::to_string((u64)windowID);
		name += " - Swapchain";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return swapchain;
}

Std::StackVec<vk::Image, Vk::Constants::maxSwapchainLength> Vk::NativeWindowManagerImpl::GetSwapchainImages(
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
	if (debugUtils != nullptr)
	{
		for (uSize i = 0; i < returnVal.Size(); i += 1)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkImage)returnVal[i];
			nameInfo.objectType = returnVal[i].objectType;
			std::string name;
			name += "NativeWindow #";
			name += std::to_string((u64)windowID);
			name += " - SwapchainImg #";
			name += std::to_string(i);
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}
	}

	return returnVal;
}

static bool Vk::NativeWindowManagerImpl::TransitionSwapchainImages(
	DeviceDispatch const& device,
	DeletionQueue const& deletionQueue,
	QueueData const& queues,
	Std::Span<const vk::Image> images)
{
	vk::Result vkResult{};

	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.queueFamilyIndex = queues.graphics.FamilyIndex();
	vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer cmdBuffer{};
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Vulkan: Unable to allocate command buffer when transitioning swapchainData images.");

	// Record commandbuffer
	{
		vk::CommandBufferBeginInfo cmdBeginInfo{};
		cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);

		for (size_t i = 0; i < images.Size(); i++)
		{
			vk::ImageMemoryBarrier imgBarrier{};
			imgBarrier.image = images[i];
			imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgBarrier.subresourceRange.layerCount = 1;
			imgBarrier.subresourceRange.levelCount = 1;
			imgBarrier.oldLayout = vk::ImageLayout::eUndefined;
			imgBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
			imgBarrier.srcAccessMask = {};
			// We are going to be transferring onto the swapchain images.
			imgBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			device.cmdPipelineBarrier(
				cmdBuffer,
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags{},
				{ 0, nullptr },
				{ 0, nullptr },
				{ 1, &imgBarrier });
		}

		device.endCommandBuffer(cmdBuffer);
	}

	vk::Fence fence = device.createFence({});

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	queues.graphics.submit(submitInfo, fence);

	deletionQueue.Destroy(fence, cmdPool);

	return true;
}


static Std::StackVec<vk::CommandBuffer, Vk::Constants::maxSwapchainLength> Vk::NativeWindowManagerImpl::AllocateSwapchainCopyCmdBuffers(
	DeviceDispatch const& device,
	NativeWindowID windowID,
	vk::CommandPool cmdPool,
	uSize swapchainImgCount,
	DebugUtilsDispatch const* debugUtils)
{
	Std::StackVec<vk::CommandBuffer, Constants::maxSwapchainLength> returnVal;
	vk::Result vkResult{};

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.commandBufferCount = (u32)swapchainImgCount;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	returnVal.Resize(cmdBufferAllocInfo.commandBufferCount);
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, returnVal.Data());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("Failed to allocate VkCommandBuffers for copy-to-swapchainData.");
	// Give names to swapchain copy command cmdBuffers
	if (debugUtils != nullptr)
	{
		for (uSize i = 0; i < returnVal.Size(); i++)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandBuffer)returnVal[i];
			nameInfo.objectType = returnVal[i].objectType;
			std::string name;
			name += "NativeWindow #";
			name += std::to_string((u64)windowID);
			name += " - CopyImg CmdBuffer #";
			name += std::to_string(i);
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}
	}

	return returnVal;
}

vk::Semaphore Vk::NativeWindowManagerImpl::CreateImageReadySemaphore(
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
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkSemaphore)returnVal;
		nameInfo.objectType = returnVal.objectType;
		std::string name;
		name += "NativeWindow #";
		name += std::to_string((u64)windowID);
		name += " - Swapchain ImgReady Semaphore";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}
	return returnVal;
}

static void Vk::NativeWindowManagerImpl::RecordSwapchainCmdBuffers(
	DeviceDispatch const& device,
	vk::Image srcImage,
	vk::Extent2D extents,
	Std::Span<vk::CommandBuffer const> cmdBuffers,
	Std::Span<vk::Image const> swapchainImages)
{
	//vk::Result vkResult{};

	for (uSize i = 0; i < cmdBuffers.Size(); i++)
	{
		vk::CommandBuffer cmdBuffer = cmdBuffers[i];
		vk::Image dstImage = swapchainImages[i];

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		device.beginCommandBuffer(cmdBuffer, beginInfo);

		// The renderpass handles transitioning the rendered image into transfer-src-optimal
		vk::ImageMemoryBarrier preTransfer_GuiBarrier{};
		preTransfer_GuiBarrier.image = srcImage;
		preTransfer_GuiBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		preTransfer_GuiBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
		preTransfer_GuiBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		preTransfer_GuiBarrier.subresourceRange.layerCount = 1;
		preTransfer_GuiBarrier.subresourceRange.levelCount = 1;
		preTransfer_GuiBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
		preTransfer_GuiBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags{},
			nullptr,
			nullptr,
			preTransfer_GuiBarrier);

		vk::ImageMemoryBarrier preTransfer_SwapchainBarrier{};
		preTransfer_SwapchainBarrier.image = dstImage;
		preTransfer_SwapchainBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
		preTransfer_SwapchainBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		preTransfer_SwapchainBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		preTransfer_SwapchainBarrier.subresourceRange.layerCount = 1;
		preTransfer_SwapchainBarrier.subresourceRange.levelCount = 1;
		preTransfer_SwapchainBarrier.srcAccessMask = {};
		preTransfer_SwapchainBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlags{},
			nullptr,
			nullptr,
			preTransfer_SwapchainBarrier);

		vk::ImageCopy copyRegion{};
		copyRegion.extent = vk::Extent3D{ extents.width, extents.height, 1 };
		copyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.dstSubresource.layerCount = 1;
		device.cmdCopyImage(
			cmdBuffer,
			srcImage,
			vk::ImageLayout::eTransferSrcOptimal,
			dstImage,
			vk::ImageLayout::eTransferDstOptimal,
			copyRegion);

		vk::ImageMemoryBarrier postTransfer_SwapchainBarrier{};
		postTransfer_SwapchainBarrier.image = dstImage;
		postTransfer_SwapchainBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		postTransfer_SwapchainBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		postTransfer_SwapchainBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		postTransfer_SwapchainBarrier.subresourceRange.layerCount = 1;
		postTransfer_SwapchainBarrier.subresourceRange.levelCount = 1;
		postTransfer_SwapchainBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		postTransfer_SwapchainBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::DependencyFlags{},
			nullptr,
			nullptr,
			postTransfer_SwapchainBarrier);

		// RenderPass handles transitioning rendered image back to color attachment output

		device.endCommandBuffer(cmdBuffer);
	}
}

Vk::NativeWindowManagerImpl::CreateGuiImg_Result DEngine::Gfx::Vk::NativeWindowManagerImpl::CreateGuiImg(
	DeviceDispatch const& device,
	VmaAllocator vma,
	NativeWindowID windowID,
	vk::Format format,
	vk::Extent2D extents,
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};
	CreateGuiImg_Result returnVal;

	// Allocate the rendertarget
	vk::ImageCreateInfo imgInfo{};
	imgInfo.arrayLayers = 1;
	imgInfo.extent = vk::Extent3D{ extents.width, extents.height, 1 };
	imgInfo.flags = vk::ImageCreateFlagBits{};
	imgInfo.format = format;
	imgInfo.imageType = vk::ImageType::e2D;
	imgInfo.initialLayout = vk::ImageLayout::eUndefined;
	imgInfo.mipLevels = 1;
	imgInfo.samples = vk::SampleCountFlagBits::e1;
	imgInfo.sharingMode = vk::SharingMode::eExclusive;
	imgInfo.tiling = vk::ImageTiling::eOptimal;
	imgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
	vkResult = (vk::Result)vmaCreateImage(
		vma,
		(VkImageCreateInfo const*)&imgInfo,
		&vmaAllocInfo,
		(VkImage*)&returnVal.img,
		&returnVal.alloc,
		nullptr);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate memory for GUI render-target.");
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkImage)returnVal.img;
		nameInfo.objectType = returnVal.img.objectType;
		std::string name;
		name += "NativeWindow #";
		name += std::to_string((u64)windowID);
		name += " - GUI Img";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return returnVal;
}

static void Vk::NativeWindowManagerImpl::TransitionGuiRenderTarget(
	DeviceDispatch const& device,
	QueueData const& queues,
	DeletionQueue const& deletionQueue,
	vk::Image img)
{
	vk::Result vkResult{};

	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.queueFamilyIndex = queues.graphics.FamilyIndex();
	vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);
	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	cmdBufferAllocInfo.commandBufferCount = 1;
	vk::CommandBuffer cmdBuffer{};
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to allocate command buffer when transitioning GUI render-target.");

	vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	device.beginCommandBuffer(cmdBuffer, cmdBufferBeginInfo);

	vk::ImageMemoryBarrier imgMemoryBarrier{};
	imgMemoryBarrier.image = img;
	imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
	imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
	imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgMemoryBarrier.subresourceRange.layerCount = 1;
	imgMemoryBarrier.subresourceRange.levelCount = 1;
	imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
	imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits{};

	device.cmdPipelineBarrier(
		cmdBuffer,
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::DependencyFlags{},
		nullptr,
		nullptr,
		imgMemoryBarrier);

	device.endCommandBuffer(cmdBuffer);

	vk::FenceCreateInfo fenceInfo{};
	vk::Fence tempFence = device.createFence(fenceInfo);

	vk::SubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	queues.graphics.submit(submitInfo, tempFence);

	deletionQueue.Destroy(tempFence, cmdPool);
}

vk::ImageView Vk::NativeWindowManagerImpl::CreateGuiImgView(
	DeviceDispatch const& device, 
	NativeWindowID windowID,
	vk::Image img, 
	vk::Format format, 
	DebugUtilsDispatch const* debugUtils)
{
	vk::ImageViewCreateInfo imgViewInfo{};
	imgViewInfo.format = format;
	imgViewInfo.image = img;
	imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgViewInfo.subresourceRange.baseArrayLayer = 0;
	imgViewInfo.subresourceRange.baseMipLevel = 0;
	imgViewInfo.subresourceRange.layerCount = 1;
	imgViewInfo.subresourceRange.levelCount = 1;
	imgViewInfo.viewType = vk::ImageViewType::e2D;
	vk::ImageView imgView = device.createImageView(imgViewInfo);
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkImageView)imgView;
		nameInfo.objectType = imgView.objectType;
		std::string name;
		name += "NativeWindow #";
		name += std::to_string((u64)windowID);
		name += " - GUI ImgView";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}

	return imgView;
}

vk::Framebuffer DEngine::Gfx::Vk::NativeWindowManagerImpl::CreateGuiFramebuffer(
	DeviceDispatch const& device, 
	NativeWindowID windowID,
	vk::RenderPass renderPass,
	vk::Extent2D extent, 
	vk::ImageView imgView, 
	DebugUtilsDispatch const* debugUtils)
{
	vk::FramebufferCreateInfo fbInfo{};
	fbInfo.attachmentCount = 1;
	fbInfo.width = extent.width;
	fbInfo.height = extent.height;
	fbInfo.layers = 1;
	fbInfo.pAttachments = &imgView;
	fbInfo.renderPass = renderPass;
	vk::Framebuffer framebuffer = device.createFramebuffer(fbInfo);
	if (debugUtils)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkFramebuffer)framebuffer;
		nameInfo.objectType = framebuffer.objectType;
		std::string name;
		name += "NativeWindow #";
		name += std::to_string((u64)windowID);
		name += " - GUI Framebuffer";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}
	return framebuffer;
}

vk::CommandPool DEngine::Gfx::Vk::NativeWindowManagerImpl::CreateGuiCmdPool(
	DeviceDispatch const& device, 
	u32 queueFamilyIndex, 
	NativeWindowID windowID,
	DebugUtilsDispatch const* debugUtils)
{
	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
	vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);
	if (debugUtils != nullptr)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkCommandPool)cmdPool;
		nameInfo.objectType = cmdPool.objectType;
		std::string name;
		name += "NativeWindow #";
		name += std::to_string((u64)windowID);
		name += " - GUI CmdPool";
		nameInfo.pObjectName = name.c_str();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}
	return cmdPool;
}

Std::StackVec<vk::CommandBuffer, Vk::Constants::maxInFlightCount> Vk::NativeWindowManagerImpl::AllocGuiCmdBuffers(
	DeviceDispatch const& device, 
	NativeWindowID windowID, 
	u8 inFlightCount, 
	vk::CommandPool cmdPool, 
	DebugUtilsDispatch const* debugUtils)
{
	vk::Result vkResult{};
	Std::StackVec<vk::CommandBuffer, Vk::Constants::maxInFlightCount> returnVal;

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.commandBufferCount = inFlightCount;
	cmdBufferAllocInfo.commandPool = cmdPool;
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	returnVal.Resize(inFlightCount);
	vkResult = device.allocateCommandBuffers(cmdBufferAllocInfo, returnVal.Data());
	if (vkResult != vk::Result::eSuccess)
		throw std::runtime_error("DEngine - Vulkan: Unable to allocate cmd buffers.");
	if (debugUtils)
	{
		for (uSize i = 0; i < returnVal.Size(); i += 1)
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkCommandBuffer)returnVal[i];
			nameInfo.objectType = returnVal[i].objectType;
			std::string name;
			name += "NativeWindow #";
			name += std::to_string((u64)windowID);
			name += " - GUI CmdBuffer #";
			name += std::to_string(i);
			nameInfo.pObjectName = name.c_str();
			debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
		}
	}

	return returnVal;
}

static void Vk::NativeWindowManagerImpl::HandleCreationJobs(
	NativeWindowManager& manager,
	GlobUtils const& globUtils)
{
	vk::Result vkResult{};

	// We need to go through each creation job and create them
	for (auto& createJob : manager.createJobs)
	{
		NativeWindowData windowData;
		windowData.wsiConnection = createJob.windowConnection;

		u64 newSurface = 0;
		vkResult = (vk::Result)windowData.wsiConnection->CreateVkSurface(
			(uSize)(VkInstance)globUtils.instance.handle,
			nullptr,
			newSurface);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Could not create VkSurfaceKHR");
		windowData.surface = (vk::SurfaceKHR)(VkSurfaceKHR)newSurface;
		if (globUtils.UsingDebugUtils())
		{
			vk::DebugUtilsObjectNameInfoEXT nameInfo{};
			nameInfo.objectHandle = (u64)(VkSurfaceKHR)windowData.surface;
			nameInfo.objectType = windowData.surface.objectType;
			std::string name;
			name += "NativeWindow #";
			name += std::to_string((u64)createJob.id);
			name += " - Surface";
			nameInfo.pObjectName = name.c_str();
			globUtils.debugUtils.setDebugUtilsObjectNameEXT(globUtils.device.handle, nameInfo);
		}

		bool physDeviceSupportsPresentation = globUtils.instance.getPhysicalDeviceSurfaceSupportKHR(
			globUtils.physDevice.handle,
			globUtils.queues.graphics.FamilyIndex(),
			windowData.surface);
		if (!physDeviceSupportsPresentation)
			throw std::runtime_error("DEngine - Vulkan: Phys device queue family does not support this surface.");

		NativeWindowManagerImpl::SwapchainSettings swapchainSettings = NativeWindowManagerImpl::BuildSwapchainSettings(
			globUtils.instance,
			globUtils.physDevice.handle,
			windowData.surface,
			globUtils.surfaceInfo);

		windowData.swapchain = NativeWindowManagerImpl::CreateSwapchain(
			globUtils.device,
			createJob.id,
			windowData.surface,
			swapchainSettings,
			windowData.swapchain,
			globUtils.DebugUtilsPtr());

		windowData.swapchainImages = NativeWindowManagerImpl::GetSwapchainImages(
			globUtils.device,
			createJob.id,
			windowData.swapchain,
			globUtils.DebugUtilsPtr());

		NativeWindowManagerImpl::TransitionSwapchainImages(
			globUtils.device,
			globUtils.deletionQueue,
			globUtils.queues,
			windowData.swapchainImages);

		windowData.copyCmdBuffers = NativeWindowManagerImpl::AllocateSwapchainCopyCmdBuffers(
			globUtils.device,
			createJob.id,
			manager.copyToSwapchainCmdPool,
			windowData.swapchainImages.Size(),
			globUtils.DebugUtilsPtr());

		windowData.swapchainImageReady = NativeWindowManagerImpl::CreateImageReadySemaphore(
			globUtils.device,
			createJob.id,
			globUtils.DebugUtilsPtr());

		WindowGuiData gui{};

		gui.extent = swapchainSettings.extents;
		gui.rotation = NativeWindowManagerImpl::RotationToMatrix(swapchainSettings.transform);
		gui.surfaceRotation = swapchainSettings.transform;

		vk::Format guiFormat = globUtils.surfaceInfo.surfaceFormatToUse.format;
		auto createGuiImgResult = NativeWindowManagerImpl::CreateGuiImg(
			globUtils.device,
			globUtils.vma,
			createJob.id,
			guiFormat,
			gui.extent,
			globUtils.DebugUtilsPtr());
		gui.img = createGuiImgResult.img;
		gui.vmaAllocation = createGuiImgResult.alloc;

		NativeWindowManagerImpl::TransitionGuiRenderTarget(
			globUtils.device,
			globUtils.queues,
			globUtils.deletionQueue,
			gui.img);

		NativeWindowManagerImpl::RecordSwapchainCmdBuffers(
			globUtils.device,
			gui.img,
			gui.extent,
			windowData.copyCmdBuffers,
			windowData.swapchainImages);

		gui.imgView = NativeWindowManagerImpl::CreateGuiImgView(
			globUtils.device,
			createJob.id,
			gui.img,
			guiFormat,
			globUtils.DebugUtilsPtr());

		gui.framebuffer = NativeWindowManagerImpl::CreateGuiFramebuffer(
			globUtils.device,
			createJob.id,
			globUtils.guiRenderPass,
			gui.extent,
			gui.imgView,
			globUtils.DebugUtilsPtr());

		gui.cmdPool = NativeWindowManagerImpl::CreateGuiCmdPool(
			globUtils.device,
			globUtils.queues.graphics.FamilyIndex(),
			createJob.id,
			globUtils.DebugUtilsPtr());

		gui.cmdBuffers = NativeWindowManagerImpl::AllocGuiCmdBuffers(
			globUtils.device,
			createJob.id,
			globUtils.inFlightCount,
			gui.cmdPool,
			globUtils.DebugUtilsPtr());

		NativeWindowManager::Node newNode;
		newNode.id = createJob.id;
		newNode.windowData = windowData;
		newNode.gui = gui;

		manager.nativeWindows.push_back(newNode);
	}
	manager.createJobs.clear();
}

void Vk::NativeWindowManagerImpl::HandleWindowResize(
	NativeWindowManager& manager, 
	GlobUtils const& globUtils,
	NativeWindowManager::Node& windowNode)
{
	auto& windowData = windowNode.windowData;

	NativeWindowManagerImpl::SwapchainSettings swapchainSettings = NativeWindowManagerImpl::BuildSwapchainSettings(
		globUtils.instance,
		globUtils.physDevice.handle,
		windowData.surface,
		globUtils.surfaceInfo);

	// We need to resize this native-window and it's GUI.

	vk::SwapchainKHR oldSwapchain = windowData.swapchain;
	windowData.swapchain = NativeWindowManagerImpl::CreateSwapchain(
		globUtils.device,
		windowNode.id,
		windowData.surface,
		swapchainSettings,
		oldSwapchain,
		globUtils.DebugUtilsPtr());
	globUtils.device.destroy(oldSwapchain);

	windowData.swapchainImages = NativeWindowManagerImpl::GetSwapchainImages(
		globUtils.device,
		windowNode.id,
		windowData.swapchain,
		globUtils.DebugUtilsPtr());

	NativeWindowManagerImpl::TransitionSwapchainImages(
		globUtils.device,
		globUtils.deletionQueue,
		globUtils.queues,
		windowData.swapchainImages);

	globUtils.device.freeCommandBuffers(
		manager.copyToSwapchainCmdPool,
		{ (u32)windowData.copyCmdBuffers.Size(), windowData.copyCmdBuffers.Data() });
	windowData.copyCmdBuffers = NativeWindowManagerImpl::AllocateSwapchainCopyCmdBuffers(
		globUtils.device,
		windowNode.id,
		manager.copyToSwapchainCmdPool,
		windowData.swapchainImages.Size(),
		globUtils.DebugUtilsPtr());

	WindowGuiData& gui = windowNode.gui;
	gui.extent = swapchainSettings.extents;
	gui.rotation = NativeWindowManagerImpl::RotationToMatrix(swapchainSettings.transform);
	gui.surfaceRotation = swapchainSettings.transform;

	vmaDestroyImage(globUtils.vma, (VkImage)gui.img, gui.vmaAllocation);
	vk::Format guiFormat = globUtils.surfaceInfo.surfaceFormatToUse.format;
	auto createGuiImgResult = NativeWindowManagerImpl::CreateGuiImg(
		globUtils.device,
		globUtils.vma,
		windowNode.id,
		guiFormat,
		gui.extent,
		globUtils.DebugUtilsPtr());
	gui.img = createGuiImgResult.img;
	gui.vmaAllocation = createGuiImgResult.alloc;

	NativeWindowManagerImpl::RecordSwapchainCmdBuffers(
		globUtils.device,
		gui.img,
		gui.extent,
		windowData.copyCmdBuffers,
		windowData.swapchainImages);

	NativeWindowManagerImpl::TransitionGuiRenderTarget(
		globUtils.device,
		globUtils.queues,
		globUtils.deletionQueue,
		gui.img);

	gui.imgView = NativeWindowManagerImpl::CreateGuiImgView(
		globUtils.device,
		windowNode.id,
		gui.img,
		guiFormat,
		globUtils.DebugUtilsPtr());

	gui.framebuffer = NativeWindowManagerImpl::CreateGuiFramebuffer(
		globUtils.device,
		windowNode.id,
		globUtils.guiRenderPass,
		gui.extent,
		gui.imgView,
		globUtils.DebugUtilsPtr());

}

void Vk::NativeWindowManagerImpl::HandleWindowRestore(
	NativeWindowManager& manager,
	GlobUtils const& globUtils,
	NativeWindowManager::Node& windowNode)
{
	globUtils.device.destroy(windowNode.windowData.swapchain);
	windowNode.windowData.swapchain = vk::SwapchainKHR{};
	globUtils.instance.destroy(windowNode.windowData.surface);

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
