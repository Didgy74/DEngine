#include "ViewportManager.hpp"
#include "DeletionQueue.hpp"

#include "DynamicDispatch.hpp"
#include "GlobUtils.hpp"
#include "GuiResourceManager.hpp"

#include <DEngine/Gfx/impl/Assert.hpp>

#include <string>
#include <mutex>

import DEngine.Std.Vec;

namespace DEngine::Gfx::Vk::ViewportMgrImpl
{
	[[nodiscard]] static auto GetViewportDataNodeIt(
		ViewportMan& manager,
		ViewportID id)
	{
		auto nodeIt = Std::FindIf(
			manager.viewportNodes.begin(),
			manager.viewportNodes.end(),
			[id](auto const& val) { return id == val.id; });
		DENGINE_IMPL_GFX_ASSERT(nodeIt != manager.viewportNodes.end());
		return nodeIt;
	}

	[[nodiscard]] static auto& GetViewportData(
		ViewportMan& manager,
		ViewportID id)
	{
		auto nodeIt = Std::FindIf(
			manager.viewportNodes.begin(),
			manager.viewportNodes.end(),
			[id](auto const& val) { return id == val.id; });
		DENGINE_IMPL_GFX_ASSERT(nodeIt != manager.viewportNodes.end());
		return nodeIt->viewport;
	}

	static void TransitionGfxImage(
		DeviceDispatch const& device,
		vk::CommandBuffer cmdBuffer,
		vk::Image img,
		bool useEditorPipeline)
	{
		vk::ImageMemoryBarrier imgBarrier{};
		imgBarrier.image = img;
		imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgBarrier.subresourceRange.layerCount = 1;
		imgBarrier.subresourceRange.levelCount = 1;
		imgBarrier.srcAccessMask = {};
		// We want to write to the image as a render-target
		imgBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		imgBarrier.oldLayout = vk::ImageLayout::eUndefined;
		// If we're in editor mode, we want to sample from the graphics viewport
		// into the editor's GUI pass.
		// If we're not in editor mode, we use a render-pass where this
		// is the image that gets copied onto the swapchain. That render-pass
		// requires the image to be in transferSrcOptimal layout.
		if (useEditorPipeline)
			imgBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		else
			imgBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;

		device.cmdPipelineBarrier(
			cmdBuffer,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::DependencyFlagBits::eByRegion,
			{},{},imgBarrier );
	}

	[[nodiscard]] static ViewportMgr_GfxRenderTarget InitializeGfxViewportRenderTarget(
		GlobUtils const& globUtils,
		vk::CommandBuffer cmdBuffer,
		ViewportID viewportID,
		vk::Extent2D viewportSize)
	{
		auto const& device = globUtils.device;
		auto& vma = globUtils.vma;
		auto const* debugUtils = globUtils.DebugUtilsPtr();

		vk::Result vkResult = {};

		vk::ImageCreateInfo colorImageInfo = {};
		colorImageInfo.arrayLayers = 1;
		colorImageInfo.extent = vk::Extent3D{ viewportSize.width, viewportSize.height, 1 };
		colorImageInfo.format = vk::Format::eR8G8B8A8Unorm;
		colorImageInfo.imageType = vk::ImageType::e2D;
		colorImageInfo.initialLayout = vk::ImageLayout::eUndefined;
		colorImageInfo.mipLevels = 1;
		colorImageInfo.samples = vk::SampleCountFlagBits::e1;
		colorImageInfo.sharingMode = vk::SharingMode::eExclusive;
		colorImageInfo.tiling = vk::ImageTiling::eOptimal;
		colorImageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
		if (globUtils.editorMode) {
			// We want to sample from the image to show it in the editor.
			colorImageInfo.usage |= vk::ImageUsageFlagBits::eSampled;
		}
		VmaAllocationCreateInfo colorImgVmaAllocInfo = {};
		colorImgVmaAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		colorImgVmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		auto colorImg = device.CreateBox(vma, colorImageInfo, colorImgVmaAllocInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				colorImg.img,
				std::format("Graphics viewport #{} - Color Image", (u64)viewportID).c_str());
		}

		// We have to transition this image
		TransitionGfxImage(
			device,
			cmdBuffer,
			colorImg.img.Handle(),
			globUtils.editorMode);

		// Make the image view
		vk::ImageViewCreateInfo colorImgViewInfo = {};
		colorImgViewInfo.format = colorImageInfo.format;
		colorImgViewInfo.image = colorImg.img.Handle();
		colorImgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		colorImgViewInfo.subresourceRange.layerCount = 1;
		colorImgViewInfo.subresourceRange.levelCount = 1;
		colorImgViewInfo.viewType = vk::ImageViewType::e2D;
		auto colorImgView = device.CreateBox(colorImgViewInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				colorImgView,
				std::format("Graphics viewport #{} - Color Image View", (u64)viewportID).c_str());
		}

		vk::ImageCreateInfo stencilImgCreateInfo = {};
		stencilImgCreateInfo.flags = {};
		stencilImgCreateInfo.imageType = vk::ImageType::e2D;
		stencilImgCreateInfo.format = vk::Format::eS8Uint;
		stencilImgCreateInfo.extent = vk::Extent3D{ viewportSize.width, viewportSize.height, 1 };
		stencilImgCreateInfo.mipLevels = 1;
		stencilImgCreateInfo.arrayLayers = 1;
		stencilImgCreateInfo.samples = vk::SampleCountFlagBits::e1;
		stencilImgCreateInfo.tiling = vk::ImageTiling::eOptimal;
		stencilImgCreateInfo.usage =
			vk::ImageUsageFlagBits::eDepthStencilAttachment
			| vk::ImageUsageFlagBits::eTransientAttachment;
		stencilImgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		stencilImgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
		VmaAllocationCreateInfo stencilImgAllocInfo = {};
		stencilImgAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		stencilImgAllocInfo.preferredFlags = (VkMemoryPropertyFlags)vk::MemoryPropertyFlagBits::eLazilyAllocated;
		stencilImgAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		auto stencilImg = device.CreateBox(vma, stencilImgCreateInfo, stencilImgAllocInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				stencilImg.img.Handle(),
				std::format("Graphics viewport #{} - Depth Stencil Image", (u64)viewportID).c_str());
		}
		vk::ImageViewCreateInfo stencilImgViewInfo = {};
		stencilImgViewInfo.format = stencilImgCreateInfo.format;
		stencilImgViewInfo.image = stencilImg.img.Handle();
		stencilImgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eStencil;
		stencilImgViewInfo.subresourceRange.layerCount = 1;
		stencilImgViewInfo.subresourceRange.levelCount = 1;
		stencilImgViewInfo.viewType = vk::ImageViewType::e2D;
		auto stencilImgView = device.CreateBox(stencilImgViewInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				colorImgView,
				std::format("Graphics viewport #{} - Stencil Image View", (u64)viewportID).c_str());
		}

		Std::Array<vk::ImageView, 2> attachments = { colorImgView.Handle(), stencilImgView.Handle() };

		vk::FramebufferCreateInfo fbInfo{};
		fbInfo.attachmentCount = attachments.Size();
		fbInfo.pAttachments = attachments.Data();
		fbInfo.width = viewportSize.width;
		fbInfo.height = viewportSize.height;
		fbInfo.layers = 1;
		fbInfo.renderPass = globUtils.gfxRenderPass;
		auto framebuffer = device.CreateBox(fbInfo);
		if (debugUtils) {
			auto name = std::string("Graphics viewport #") + std::to_string((u64)viewportID) + " - Framebuffer";
			debugUtils->Helper_SetObjectName(
				device.handle,
				framebuffer,
				name.c_str());
		}

		ViewportMgr_GfxRenderTarget returnVal = {};
		returnVal.extent = viewportSize;
		returnVal.colorImg = Std::Move(colorImg.img);
		returnVal.colorImgView = Std::Move(colorImgView);
		returnVal.stencilImg = Std::Move(stencilImg.img);
		returnVal.stencilImgView = Std::Move(stencilImgView);
		returnVal.framebuffer = Std::Move(framebuffer);
		return returnVal;
	}

	// Assumes the viewportManager.viewportDatas is already locked.
	static void HandleViewportRenderTargetInitialization(
		ViewportMgr_GfxRenderTarget& renderTarget,
		GlobUtils const& globUtils,
		vk::CommandBuffer cmdBuffer,
		ViewportUpdate const& updateData)
	{
		// We need to create this virtual viewport
		renderTarget = InitializeGfxViewportRenderTarget(
			globUtils,
			cmdBuffer,
			updateData.id,
			{ updateData.width, updateData.height });
	}

	// Assumes the viewportManager.viewportDatas is already locked.
	static void HandleViewportResize(
		ViewportMgr_GfxRenderTarget& renderTarget,
		GlobUtils const& globUtils,
		vk::CommandBuffer cmdBuffer,
		DelQueue& delQueue,
		ViewportUpdate const& updateData)
	{
		// This image needs to be resized. We just make a new one and push a job to delete the old one.
		// VMA should handle memory allocations to be effective and reused.
		delQueue.Destroy(Std::Move(renderTarget.framebuffer));
		delQueue.Destroy(Std::Move(renderTarget.colorImgView));
		delQueue.Destroy(Std::Move(renderTarget.colorImg));
		delQueue.Destroy(Std::Move(renderTarget.stencilImgView));
		delQueue.Destroy(Std::Move(renderTarget.stencilImg));
		renderTarget = {};

		renderTarget = InitializeGfxViewportRenderTarget(
			globUtils,
			cmdBuffer,
			updateData.id,
			{ updateData.width, updateData.height });
	}

	static ViewportMgr_ViewportData InitializeViewport(
		GlobUtils const& globUtils,
		ViewportManager::CreateJob createJob,
		uSize camElementSize,
		vk::DescriptorSetLayout cameraDescrLayout)
	{
		auto const& device = globUtils.device;
		auto const* debugUtils = globUtils.DebugUtilsPtr();

		vk::Result vkResult = {};

		ViewportMgr_ViewportData viewport = {};

		// Make the descriptorpool and sets
		vk::DescriptorPoolSize descrPoolSize{};
		descrPoolSize.type = ViewportManager::cameraDataUniformDescrType;
		descrPoolSize.descriptorCount = globUtils.inFlightCount;

		vk::DescriptorPoolCreateInfo descrPoolInfo{};
		descrPoolInfo.maxSets = globUtils.inFlightCount;
		descrPoolInfo.poolSizeCount = 1;
		descrPoolInfo.pPoolSizes = &descrPoolSize;
		viewport.cameraDescrPool = device.Create(descrPoolInfo);
		if (debugUtils) {
			std::string name = "Graphics viewport #";
			name += std::to_string((int)createJob.id);
			name +=	" - Camera-data DescrPool";
			debugUtils->Helper_SetObjectName(device.handle, viewport.cameraDescrPool, name.c_str());
		}

		
		vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
		descrSetAllocInfo.descriptorPool = viewport.cameraDescrPool;
		descrSetAllocInfo.descriptorSetCount = globUtils.inFlightCount;
		Std::StackVec<vk::DescriptorSetLayout, Constants::maxInFlightCount> descrLayouts;
		descrLayouts.Resize(globUtils.inFlightCount);
		for (auto& item : descrLayouts)
			item = cameraDescrLayout;
		descrSetAllocInfo.pSetLayouts = descrLayouts.Data();
		viewport.camDataDescrSets.Resize(globUtils.inFlightCount);
		vkResult = device.Alloc(descrSetAllocInfo, viewport.camDataDescrSets.Data());
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to allocate descriptor sets for viewport camera data.");
		if (debugUtils) {
			for (uSize i = 0; i < viewport.camDataDescrSets.Size(); i += 1) {
				std::string name = "Graphics viewport #";
				name += std::to_string((int)createJob.id);
				name += " - Camera-data DescrSet #";
				name +=	std::to_string((int)i);
				debugUtils->Helper_SetObjectName(device.handle, viewport.camDataDescrSets[i], name.c_str());
			}
		}


		// Allocate the data
		vk::BufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferCreateInfo.size = globUtils.inFlightCount * camElementSize;
		bufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		VmaAllocationCreateInfo memAllocInfo{};
		memAllocInfo.flags =
			VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
			| VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
		memAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
		VmaAllocationInfo resultMemInfo{};
		vkResult = (vk::Result)vmaCreateBuffer(
			globUtils.vma,
			(VkBufferCreateInfo*)&bufferCreateInfo,
			&memAllocInfo,
			(VkBuffer*)&viewport.camDataBuffer,
			&viewport.camVmaAllocation,
			&resultMemInfo);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: VMA unable to allocate cam-data memory for viewport.");
		viewport.camDataMappedMem = { (char*)resultMemInfo.pMappedData, (uSize)resultMemInfo.size };
		if (debugUtils) {
			std::string name = "Graphics viewport #";
			name += std::to_string((int)createJob.id);
			name += " - CamData Buffer";
			debugUtils->Helper_SetObjectName(device.handle, viewport.camDataBuffer,name.c_str());
		}
		

		// Update descriptor sets
		Std::StackVec<vk::WriteDescriptorSet, Constants::maxInFlightCount> writes{};
		writes.Resize(globUtils.inFlightCount);
		Std::StackVec<vk::DescriptorBufferInfo, Constants::maxInFlightCount> bufferInfos{};
		bufferInfos.Resize(globUtils.inFlightCount);
		for (uSize i = 0; i < globUtils.inFlightCount; i += 1) {
			vk::WriteDescriptorSet& writeData = writes[i];
			vk::DescriptorBufferInfo& bufferInfo = bufferInfos[i];
			writeData.descriptorCount = 1;
			writeData.descriptorType = ViewportManager::cameraDataUniformDescrType;
			writeData.dstBinding = 0;
			writeData.dstSet = viewport.camDataDescrSets[i];
			writeData.pBufferInfo = &bufferInfo;

			bufferInfo.buffer = viewport.camDataBuffer;
			bufferInfo.offset = camElementSize * i;
			bufferInfo.range = camElementSize;
		}
		device.UpdateDescriptorSets(writes.ToSpan(), {});

		return viewport;
	}

	void HandleViewportDeleteJobs(
		ViewportManager& viewportManager,
		GlobUtils const& globUtils,
		DelQueue& delQueue,
		Std::AllocRef const& transientAlloc)
	{
		auto tempDeleteJobs = Std::NewVec<ViewportID>(transientAlloc);
		{
			std::scoped_lock lock { viewportManager.deleteQueue_Lock };

			tempDeleteJobs.Resize(viewportManager.deleteQueue.size());
			for (int i = 0; i < tempDeleteJobs.Size(); i++)
				tempDeleteJobs[i] = viewportManager.deleteQueue[i];
			viewportManager.deleteQueue.clear();
		}

		auto& viewportNodes = viewportManager.viewportNodes;

		// Execute pending deletions
		for (auto const id : tempDeleteJobs)
		{
			auto nodeIt = ViewportMgrImpl::GetViewportDataNodeIt(viewportManager, id);
			auto viewportNode = Std::Move(*nodeIt);
			viewportNodes.erase(nodeIt);

			auto& viewportData = viewportNode.viewport;

			// Delete all the resources
			delQueue.Destroy(viewportData.cameraDescrPool);
			delQueue.Destroy(viewportData.camVmaAllocation, viewportData.camDataBuffer);
			delQueue.Destroy(Std::Move(viewportData.renderTarget.framebuffer));
			delQueue.Destroy(Std::Move(viewportData.renderTarget.colorImgView));
			delQueue.Destroy(Std::Move(viewportData.renderTarget.colorImg));
			delQueue.Destroy(Std::Move(viewportData.renderTarget.stencilImgView));
			delQueue.Destroy(Std::Move(viewportData.renderTarget.stencilImg));
		}
	}

	void HandleViewportCreationJobs(
		ViewportManager& viewportManager,
		GlobUtils const& globUtils,
		Std::AllocRef const& transientAlloc)
	{
		auto tempCreateJobs = Std::NewVec<ViewportManager::CreateJob>(transientAlloc);
		{
			std::scoped_lock lock{ viewportManager.createQueue_Lock };
			tempCreateJobs.Resize(viewportManager.createQueue.size());
			for (int i = 0; i < tempCreateJobs.Size(); i += 1)
				tempCreateJobs[i] = viewportManager.createQueue[i];
			viewportManager.createQueue.clear();
		}

		for (auto const& createJob : tempCreateJobs) {
			auto newViewport = InitializeViewport(
				globUtils,
				createJob,
				viewportManager.camElementSize,
				viewportManager.cameraDescrLayout);

			ViewportMan::Node newNode = {};
			newNode.id = createJob.id;
			newNode.viewport = Std::Move(newViewport);
			viewportManager.viewportNodes.push_back(Std::Move(newNode));
		}
	}


	void UpdateCameraUniforms(
		ViewportMan& manager,
		DeviceDispatch const& device,
		vk::CommandBuffer cmdBuffer,
		Std::Span<ViewportUpdate const> const& viewportUpdates,
		int inFlightIndex,
		TransientAllocRef transientAlloc)
	{
		// Update the camera uniforms
		for (auto const& update : viewportUpdates) {
			auto& data = ViewportMgrImpl::GetViewportData(manager, update.id);
			auto offset = manager.camElementSize * inFlightIndex;
			DENGINE_IMPL_GFX_ASSERT(offset + manager.camElementSize <= data.camDataMappedMem.Size());
			std::memcpy(
				data.camDataMappedMem.Data() + offset,
				&update.transform,
				manager.camElementSize);
		}
	}

	void AllocateAndWriteSampledViewportDescrSet(
		ViewportMgr_ViewportData& viewportData,
		DeviceDispatch const& device,
		vk::DescriptorSetLayout setLayout,
		vk::Sampler sampler,
		vk::ImageView imgView,
		ViewportID id,
		DebugUtilsDispatch const* debugUtils)
	{
		// Set up our pool and sampler
		vk::DescriptorPoolSize poolSize = {};
		poolSize.type = vk::DescriptorType::eCombinedImageSampler;
		poolSize.descriptorCount = 1;
		vk::DescriptorPoolCreateInfo poolInfo = {};
		poolInfo.maxSets = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.poolSizeCount = 1;
		auto pool = device.Create(poolInfo);
		if (debugUtils){
			auto name = std::string("ViewportManager - Viewport Image #") + std::to_string((int)id) + " - DescrPool";
			debugUtils->Helper_SetObjectName(
				device.handle,
				pool,
				name.c_str());
		}

		vk::DescriptorSetAllocateInfo descrSetAllocInfo = {};
		descrSetAllocInfo.descriptorPool = pool;
		descrSetAllocInfo.descriptorSetCount = 1;
		descrSetAllocInfo.pSetLayouts = &setLayout;
		vk::DescriptorSet descrSet = {};
		auto vkResult = device.Alloc(descrSetAllocInfo, &descrSet);
		if (vkResult != vk::Result::eSuccess)
			throw std::runtime_error("DEngine - Vulkan: Unable to allocate descr set for viewport image.");
		if (debugUtils){
			auto name = std::string("ViewportManager - Viewport Image #") + std::to_string((int)id) + " - DescrSet";
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrSet,
				name.c_str());
		}

		vk::DescriptorImageInfo descrImgInfo = {};
		descrImgInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		descrImgInfo.imageView = imgView;
		descrImgInfo.sampler = sampler;
		vk::WriteDescriptorSet descrWrite = {};
		descrWrite.descriptorCount = 1;
		descrWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		descrWrite.dstSet = descrSet;
		descrWrite.pImageInfo = &descrImgInfo;
		device.UpdateDescriptorSets(descrWrite, {});

		viewportData.imgDescrPool = pool;
		viewportData.imgDescrSet = descrSet;
	}
}

using namespace DEngine;
using namespace DEngine::Gfx;
using ViewportManager = Gfx::Vk::ViewportManager;

bool ViewportManager::Node::IsInitialized() const
{
	return !viewport.renderTarget.colorImg.IsNull();
}

bool ViewportManager::Init(
	ViewportManager& manager,
	DeviceDispatch const& device,
	uSize minUniformBufferOffsetAlignment,
	DebugUtilsDispatch const* debugUtils)
{
	// Set up the camera descr layout
	{
		vk::DeviceSize elementSize = 64;
		if (minUniformBufferOffsetAlignment > elementSize)
			elementSize = minUniformBufferOffsetAlignment;
		manager.camElementSize = (uSize)elementSize;

		vk::DescriptorSetLayoutBinding binding = {};
		binding.binding = 0;
		binding.descriptorCount = 1;
		binding.descriptorType = ViewportManager::cameraDataUniformDescrType;
		binding.stageFlags = vk::ShaderStageFlagBits::eVertex;
		vk::DescriptorSetLayoutCreateInfo descrLayoutInfo = {};
		descrLayoutInfo.bindingCount = 1;
		descrLayoutInfo.pBindings = &binding;
		manager.cameraDescrLayout = device.Create(descrLayoutInfo);
		if (debugUtils){
			debugUtils->Helper_SetObjectName(
				device.handle,
				manager.cameraDescrLayout,
				"ViewportManager - Cam DescrLayout");
		}
	}


	// Set up the sampled image descr layout
	{
		vk::DescriptorSetLayoutBinding imgDescrBinding{};
		imgDescrBinding.binding = 0;
		imgDescrBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		imgDescrBinding.descriptorCount = 1;
		imgDescrBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo = {};
		descrSetLayoutInfo.bindingCount = 1;
		descrSetLayoutInfo.pBindings = &imgDescrBinding;
		auto descrLayout = device.Create(descrSetLayoutInfo);
		if (debugUtils) {
			debugUtils->Helper_SetObjectName(
				device.handle,
				descrLayout,
				"ViewportManager - Viewport DescrSetLayout");
		}
		manager.imgDescrSetLayout = descrLayout;

		vk::SamplerCreateInfo samplerInfo = {};
		samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
		samplerInfo.magFilter = vk::Filter::eLinear;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.minFilter = vk::Filter::eLinear;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
		auto sampler = device.Create(samplerInfo);
		if (debugUtils){
			debugUtils->Helper_SetObjectName(
				device.handle,
				sampler,
				"ViewportManager - Viewport Image Sampler");
		}
		manager.imgSampler = sampler;
	}


	return true;
}

void Vk::ViewportManager::NewViewport(
	ViewportManager& viewportManager,
	ViewportID& viewportID)
{
	ViewportManager::CreateJob createJob{};

	std::lock_guard lock{ viewportManager.createQueue_Lock };
	createJob.id = (ViewportID)viewportManager.viewportIDTracker;
	viewportID = createJob.id;

	viewportManager.createQueue.push_back(createJob);
	viewportManager.viewportIDTracker += 1;
}

void ViewportManager::DeleteViewport(
	ViewportManager& manager,
	ViewportID viewportId)
{
	std::lock_guard lock{ manager.deleteQueue_Lock };
	manager.deleteQueue.push_back(viewportId);
}

void ViewportManager::ProcessEvents(
	ViewportManager& manager,
	ProcessEvents_Params const& params)
{
	auto const& globUtils = params.globUtils;
	auto const& device = globUtils.device;
	auto& transientAlloc = params.transientAlloc;
	auto& cmdBuffer = params.cmdBuffer;
	auto const& viewportUpdates = params.viewportUpdates;
	auto& delQueue = params.delQueue;
	auto inFlightIndex = params.inFlightIndex;
	auto* debugUtils = params.debugUtils;

	vk::Result vkResult = {};

	ViewportMgrImpl::HandleViewportDeleteJobs(manager, globUtils, delQueue, transientAlloc);
	ViewportMgrImpl::HandleViewportCreationJobs(manager, globUtils, transientAlloc);

	ViewportMgrImpl::UpdateCameraUniforms(
		manager,
		device,
		cmdBuffer,
		viewportUpdates,
		inFlightIndex,
		transientAlloc);

	// First we handle any viewportManager that need to be initialized, not resized.
	for (auto const& updateData : viewportUpdates) {
		auto id = updateData.id;
		auto& viewportData = ViewportMgrImpl::GetViewportData(manager, updateData.id);
		if (!viewportData.renderTarget.IsInitialized()) {
			// This image is not initalized.
			ViewportMgrImpl::HandleViewportRenderTargetInitialization(
				viewportData.renderTarget,
				globUtils,
				cmdBuffer,
				updateData);

			ViewportMgrImpl::AllocateAndWriteSampledViewportDescrSet(
				viewportData,
				device,
				manager.imgDescrSetLayout,
				manager.imgSampler,
				viewportData.renderTarget.colorImgView.Handle(),
				id,
				debugUtils);
		}
		else if (updateData.width != viewportData.renderTarget.extent.width ||
			updateData.height != viewportData.renderTarget.extent.height)
		{
			// This image needs to be resized.
			ViewportMgrImpl::HandleViewportResize(
				viewportData.renderTarget,
				globUtils,
				cmdBuffer,
				delQueue,
				updateData);

			delQueue.Destroy(viewportData.imgDescrPool);

			ViewportMgrImpl::AllocateAndWriteSampledViewportDescrSet(
				viewportData,
				device,
				manager.imgDescrSetLayout,
				manager.imgSampler,
				viewportData.renderTarget.colorImgView.Handle(),
				id,
				debugUtils);
		}
	}
}

Vk::ViewportMgr_ViewportData const& ViewportManager::GetViewportData(ViewportID id) const
{
	auto iter = Std::FindIf(
		this->viewportNodes.begin(),
		this->viewportNodes.end(),
		[&](auto const& item) { return item.id == id; });
	DENGINE_IMPL_GFX_ASSERT(iter != this->viewportNodes.end());
	return iter->viewport;
}
