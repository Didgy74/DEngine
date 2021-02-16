#include "TextureManager.hpp"

#include "GlobUtils.hpp"

#include <Texas/Texas.hpp>
#include <Texas/Tools.hpp>
#include <Texas/VkTools.hpp>
#include <Texas/GLTools.hpp>
#include <DEngine/Application.hpp>

namespace DEngine::Gfx::Vk
{
	struct TextureFileStream : public Texas::InputStream
	{
		App::FileInputStream stream;
		virtual Texas::Result read(Texas::ByteSpan dst) noexcept override
		{
			bool result = stream.Read((char*)dst.data(), dst.size());
			if (result)
				return Texas::successResult;
			else
				return Texas::Result{ Texas::ResultType::UnknownError, "Error when reading file." };
		}
		virtual void ignore(std::size_t amount) noexcept override
		{
			stream.Seek(amount, App::FileInputStream::SeekOrigin::Current);
		}
		virtual std::size_t tell() noexcept override
		{
			return (uSize)stream.Tell().Value();
		}
		virtual void seek(std::size_t pos) noexcept override
		{
			stream.Seek(pos, App::FileInputStream::SeekOrigin::Start);
		}
	};
}

void DEngine::Gfx::Vk::TextureManager::Update(
	TextureManager& manager,
	GlobUtils const& globUtils,
	DrawParams const& drawParams,
	Gfx::TextureAssetInterface const& texAssetInterface)
{
	vk::Result vkResult{};

	for (auto& item : manager.database)
		item.second.isNeededHelper = false;

	for (auto textureID : drawParams.textureIDs)
	{
		auto iter = manager.database.find(textureID);
		if (iter != manager.database.end())
			iter->second.isNeededHelper = true;
		else
		{
			Inner newInner{};
			newInner.isNeededHelper = true;

			// First we need to get path to the thing
			char const* path = texAssetInterface.get(textureID);

			TextureFileStream fileStream{};
			fileStream.stream.Open(path);
			if (!fileStream.stream.IsOpen())
				throw std::runtime_error("Error. Could not open file-stream.");

			Texas::ResultValue<Texas::FileInfo> parseResult = Texas::parseStream(fileStream);
			if (!parseResult.isSuccessful())
				throw std::runtime_error("Error. Couldnt parse stream.");
			Texas::FileInfo& texFileInfo = parseResult.value();

			// Create the buffer for us to load the image-data onto.
			vk::BufferCreateInfo buffInfo{};
			buffInfo.sharingMode = vk::SharingMode::eExclusive;
			buffInfo.size = texFileInfo.memoryRequired();
			buffInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
			VmaAllocationCreateInfo stagingBufferVmaAllocInfo{};
			stagingBufferVmaAllocInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
			stagingBufferVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
			vk::Buffer stagingBuffer{};
			VmaAllocation stagingBufferVmaAlloc{};
			VmaAllocationInfo stagingBufferResultInfo{};
			vkResult = (vk::Result)vmaCreateBuffer(
				globUtils.vma,
				(VkBufferCreateInfo const*)&buffInfo,
				&stagingBufferVmaAllocInfo,
				(VkBuffer*)&stagingBuffer,
				&stagingBufferVmaAlloc,
				&stagingBufferResultInfo);
			if (vkResult != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate staging buffer for texture.");
			// Queue the staging buffer up for deletion.
			globUtils.deletionQueue.Destroy(stagingBufferVmaAlloc, stagingBuffer);
			if (globUtils.UsingDebugUtils())
			{
				std::string name = "TextureManager - Texture #" + std::to_string((u64)textureID) + " - StagingBuffer";
				globUtils.debugUtils.Helper_SetObjectName(
					globUtils.device.handle,
					stagingBuffer,
					name.c_str());
			}

			std::byte* workingMemory = nullptr;
			if (texFileInfo.workingMemoryRequired() > 0)
				workingMemory = new std::byte[texFileInfo.workingMemoryRequired()];

			Texas::ByteSpan dstImageDataSpan = {
					(std::byte*)stagingBufferResultInfo.pMappedData,
					static_cast<uSize>(buffInfo.size) };
			Texas::ByteSpan workingMemSpan = {
					workingMemory,
					static_cast<uSize>(texFileInfo.workingMemoryRequired()) };
			Texas::Result loadImageDataResult = Texas::loadImageData(fileStream, texFileInfo, dstImageDataSpan, workingMemSpan);
			if (!loadImageDataResult.isSuccessful())
			{
				std::string errorMsg = "DEngine - Vulkan: Texas was unable to load image-data. Detailed error: ";
				errorMsg += loadImageDataResult.errorMessage();
				throw std::runtime_error(errorMsg);
			}
			delete[] workingMemory;

			vk::ImageCreateInfo imgInfo{};
			imgInfo.arrayLayers = (u32)texFileInfo.textureInfo().layerCount;
			imgInfo.extent = (vk::Extent3D)Texas::toVkExtent3D(texFileInfo.textureInfo().baseDimensions);
			imgInfo.format = (vk::Format)Texas::toVkFormat(texFileInfo.textureInfo());
			imgInfo.imageType = (vk::ImageType)Texas::toVkImageType(texFileInfo.textureInfo().textureType);
			imgInfo.initialLayout = vk::ImageLayout::eUndefined;
			imgInfo.mipLevels = (u32)texFileInfo.textureInfo().mipCount;
			imgInfo.samples = vk::SampleCountFlagBits::e1;
			imgInfo.sharingMode = vk::SharingMode::eExclusive;
			imgInfo.tiling = vk::ImageTiling::eOptimal;
			imgInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
			VmaAllocationCreateInfo imgVmaAllocInfo{};
			imgVmaAllocInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY;
			vkResult = (vk::Result)vmaCreateImage(
				globUtils.vma,
				(VkImageCreateInfo const*)&imgInfo,
				&imgVmaAllocInfo,
				(VkImage*)&newInner.img,
				&newInner.imgVmaAlloc,
				nullptr);
			if (vkResult != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: VMA was unable to allocate image.");
			if (globUtils.UsingDebugUtils())
			{
				std::string name = "TextureManager - Texture #" + std::to_string((u64)textureID) + " - Img";
				globUtils.debugUtils.Helper_SetObjectName(
					globUtils.device.handle,
					newInner.img,
					name.c_str());
			}

			// Copy and transition image.
			vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
			cmdBufferAllocInfo.commandPool = manager.cmdPool;
			cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
			cmdBufferAllocInfo.commandBufferCount = 1;
			vk::CommandBuffer cmdBuffer{};
			vkResult = globUtils.device.allocateCommandBuffers(cmdBufferAllocInfo, &cmdBuffer);
			if (vkResult != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: Unable to allocate command buffer for copying texture.");
			if (globUtils.UsingDebugUtils())
			{
				std::string name = "TextureManager - Texture #" + std::to_string((u64)textureID) + " - CmdBuffer";
				globUtils.debugUtils.Helper_SetObjectName(
					globUtils.device.handle,
					cmdBuffer,
					name.c_str());
			}


			vk::CommandBufferBeginInfo cmdBeginInfo{};
			cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
			globUtils.device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);

			vk::BufferMemoryBarrier buffBarrier{};
			buffBarrier.buffer = stagingBuffer;
			buffBarrier.size = buffInfo.size;
			buffBarrier.srcAccessMask = {};
			buffBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
			vk::ImageMemoryBarrier imgBarrierA{};
			imgBarrierA.image = newInner.img;
			imgBarrierA.oldLayout = vk::ImageLayout::eUndefined;
			imgBarrierA.newLayout = vk::ImageLayout::eTransferDstOptimal;
			imgBarrierA.srcAccessMask = {};
			imgBarrierA.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
			imgBarrierA.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgBarrierA.subresourceRange.layerCount = imgInfo.arrayLayers;
			imgBarrierA.subresourceRange.levelCount = imgInfo.mipLevels;
			globUtils.device.cmdPipelineBarrier(
				cmdBuffer,
				vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags(),
				nullptr,
				buffBarrier,
				imgBarrierA);

			std::vector<vk::BufferImageCopy> buffImgCopies;
			for (u8 i = 0; i < imgInfo.mipLevels; i++)
			{
				buffImgCopies.push_back({});
				vk::BufferImageCopy& buffImgCopy = buffImgCopies.back();
				buffImgCopy.bufferOffset = Texas::calculateMipOffset(texFileInfo.textureInfo(), i);
				buffImgCopy.imageExtent = Texas::toVkExtent3D(Texas::calculateMipDimensions(
						texFileInfo.textureInfo().baseDimensions, 
						i));
				buffImgCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				buffImgCopy.imageSubresource.layerCount = imgInfo.arrayLayers;
				buffImgCopy.imageSubresource.mipLevel = i;
			}
			globUtils.device.cmdCopyBufferToImage(
				cmdBuffer,
				stagingBuffer,
				newInner.img,
				vk::ImageLayout::eTransferDstOptimal,
				{ (u32)buffImgCopies.size(), buffImgCopies.data() });

			vk::ImageMemoryBarrier imgBarrierB{};
			imgBarrierB.image = newInner.img;
			imgBarrierB.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			imgBarrierB.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imgBarrierB.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imgBarrierB.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			imgBarrierB.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgBarrierB.subresourceRange.layerCount = imgInfo.arrayLayers;
			imgBarrierB.subresourceRange.levelCount = imgInfo.mipLevels;
			globUtils.device.cmdPipelineBarrier(
				cmdBuffer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				imgBarrierB);

			globUtils.device.endCommandBuffer(cmdBuffer);

			vk::SubmitInfo submit{};
			submit.commandBufferCount = 1;
			submit.pCommandBuffers = &cmdBuffer;
			globUtils.queues.graphics.submit(submit, nullptr);
			// I think we can do this without the fence, because we wait for this stuff
			// in the graphics queue anyways...
			globUtils.deletionQueue.Destroy(manager.cmdPool, { &cmdBuffer, 1 });


			vk::ImageViewCreateInfo imgViewInfo{};
			imgViewInfo.format = (vk::Format)Texas::toVkFormat(texFileInfo.textureInfo());
			imgViewInfo.image = newInner.img;
			imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgViewInfo.subresourceRange.layerCount = (u32)texFileInfo.textureInfo().layerCount;
			imgViewInfo.subresourceRange.levelCount = (u32)texFileInfo.textureInfo().mipCount;
			imgViewInfo.viewType = (vk::ImageViewType)Texas::toVkImageViewType(texFileInfo.textureInfo().textureType);
			newInner.imgView = globUtils.device.createImageView(imgViewInfo);
			if (globUtils.UsingDebugUtils())
			{
				std::string name = "TextureManager - Texture #" + std::to_string((u64)textureID) + " - ImgView";
				globUtils.debugUtils.Helper_SetObjectName(
					globUtils.device.handle,
					newInner.imgView,
					name.c_str());
			}

			// Make the descriptor-set and update it
			vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
			descrSetAllocInfo.descriptorPool = manager.descrPool;
			descrSetAllocInfo.descriptorSetCount = 1;
			descrSetAllocInfo.pSetLayouts = &manager.descrSetLayout;
			vkResult = globUtils.device.allocateDescriptorSets(descrSetAllocInfo, &newInner.descrSet);
			if (vkResult != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: Could not alloate descriptor set.");
			if (globUtils.UsingDebugUtils())
			{
				std::string name = "TextureManager - Texture #" + std::to_string((u64)textureID) + " - DescrSet";
				globUtils.debugUtils.Helper_SetObjectName(
					globUtils.device.handle,
					newInner.descrSet,
					name.c_str());
			}

			vk::DescriptorImageInfo descrImgInfo{};
			descrImgInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			descrImgInfo.imageView = newInner.imgView;
			descrImgInfo.sampler = manager.sampler;
			vk::WriteDescriptorSet descrWrite{};
			descrWrite.descriptorCount = 1;
			descrWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			descrWrite.dstBinding = 0;
			descrWrite.dstSet = newInner.descrSet;
			descrWrite.pImageInfo = &descrImgInfo;
			globUtils.device.updateDescriptorSets(descrWrite, nullptr);

			manager.database.insert({ textureID, newInner });
		}

		// Delete unneeded stuff
		for (auto iter = manager.database.begin(); iter != manager.database.end(); iter++)
		{
			//auto& item = *iter;

		}
	}
}

void DEngine::Gfx::Vk::TextureManager::Init(
	TextureManager& manager,
	DeviceDispatch const& device,
	QueueData const& queues,
	DebugUtilsDispatch const* debugUtils)
{
	// Make the sampler
	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	manager.sampler = device.createSampler(samplerInfo);
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.sampler,
			"TextureManager - Sampler");
	}

	// Make descriptor-pool
	vk::DescriptorPoolSize descrPoolSize{};
	descrPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
	descrPoolSize.descriptorCount = TextureManager::descrPool_minCapacity;
	vk::DescriptorPoolCreateInfo descrPoolInfo{};
	descrPoolInfo.maxSets = TextureManager::descrPool_minCapacity;
	descrPoolInfo.poolSizeCount = 1;
	descrPoolInfo.pPoolSizes = &descrPoolSize;
	manager.descrPool = device.createDescriptorPool(descrPoolInfo);
	manager.descrPoolCapacity = descrPoolSize.descriptorCount;
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.descrPool,
			"TextureManager - DescrPool");
	}

	// Descriptor set layout
	vk::DescriptorSetLayoutBinding descrSetLayoutBinding{};
	descrSetLayoutBinding.binding = 0;
	descrSetLayoutBinding.descriptorCount = 1;
	descrSetLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descrSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
	vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo{};
	descrSetLayoutInfo.bindingCount = 1;
	descrSetLayoutInfo.pBindings = &descrSetLayoutBinding;
	manager.descrSetLayout = device.createDescriptorSetLayout(descrSetLayoutInfo);
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.descrSetLayout,
			"TextureManager - DescrSetLayout");
	}

	// Command pool
	vk::CommandPoolCreateInfo cmdPoolInfo{};
	cmdPoolInfo.queueFamilyIndex = queues.graphics.FamilyIndex();
	manager.cmdPool = device.createCommandPool(cmdPoolInfo);
	if (debugUtils)
	{
		debugUtils->Helper_SetObjectName(
			device.handle,
			manager.cmdPool,
			"TextureManager - CmdPool");
	}
}