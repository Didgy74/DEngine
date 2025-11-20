#include "TextureManager.hpp"

#include "GlobUtils.hpp"
#include "DeletionQueue.hpp"


#include <DEngine/Platform/Platform.hpp>

#include <format>

#include <Texas/Texas.hpp>
#include <Texas/Tools.hpp>
#include <Texas/VkTools.hpp>

import DEngine.Std.Vec;

using namespace DEngine;
using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

namespace DEngine::Gfx::Vk
{
	struct TextureFileStream : public Texas::InputStream {
		Platform::FileInputStream stream;
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
			stream.Seek(amount, Platform::FileInputStream::SeekOrigin::Current);
		}
		virtual std::size_t tell() noexcept override
		{
			return (uSize)stream.Tell().Value();
		}
		virtual void seek(std::size_t pos) noexcept override
		{
			stream.Seek(pos, Platform::FileInputStream::SeekOrigin::Start);
		}
	};
}

void TextureManager::Update(
	TextureManager& manager,
	GlobUtils const& globUtils,
	DelQueue& delQueue,
	StagingBufferAlloc& stagingBufferAlloc,
	vk::CommandBuffer cmdBuffer,
	DrawParams const& drawParams,
	Gfx::TextureAssetInterface const& texAssetInterface,
	Std::AllocRef const& transientAlloc)
{
	auto const* debugUtils = globUtils.DebugUtilsPtr();
	auto const& device = globUtils.device;

	vk::Result vkResult = {};

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

			TextureFileStream fileStream = {};
			fileStream.stream.Open(path);
			if (!fileStream.stream.IsOpen())
				throw std::runtime_error("Error. Could not open file-stream.");

			auto parseResult = Texas::parseStream(fileStream);
			if (!parseResult.isSuccessful())
				throw std::runtime_error("Error. Couldnt parse image file stream.");
			auto& texFileInfo = parseResult.value();

			auto vkImgFormat = (vk::Format)Texas::toVkFormat(texFileInfo.textureInfo());

			// Create the buffer for us to load the image-data onto.
			auto stagingBuffer = stagingBufferAlloc.SubAlloc2(
				device,
				(int)texFileInfo.memoryRequired(),
				bufferOffsetAlignmentForFormat(vkImgFormat));
			auto workingMemory = Std::NewVec<char>(transientAlloc);
			if (texFileInfo.workingMemoryRequired() > 0)
				workingMemory.Resize(texFileInfo.workingMemoryRequired());

			Texas::ByteSpan dstImageDataSpan = {
				(std::byte*)stagingBuffer.MappedMem().Data(),
				(uSize)texFileInfo.memoryRequired() };
			Texas::ByteSpan workingMemSpan = {
				(std::byte*)workingMemory.Data(),
				(uSize)texFileInfo.workingMemoryRequired() };
			// TODO: We should confirm that Texas is guaranteed to write the data sequentially
			auto loadImageDataResult = Texas::loadImageData(
				fileStream,
				texFileInfo,
				dstImageDataSpan,
				workingMemSpan);
			if (!loadImageDataResult.isSuccessful()) {
				std::string errorMsg = "DEngine - Vulkan: Texas was unable to load image-data. Detailed error: ";
				errorMsg += loadImageDataResult.errorMessage();
				throw std::runtime_error(errorMsg);
			}

			vk::ImageCreateInfo imgInfo = {};
			imgInfo.arrayLayers = (u32)texFileInfo.textureInfo().layerCount;
			imgInfo.extent = (vk::Extent3D)Texas::toVkExtent3D(texFileInfo.textureInfo().baseDimensions);
			imgInfo.format = vkImgFormat;
			imgInfo.imageType = (vk::ImageType)Texas::toVkImageType(texFileInfo.textureInfo().textureType);
			imgInfo.initialLayout = vk::ImageLayout::eUndefined;
			imgInfo.mipLevels = (u32)texFileInfo.textureInfo().mipCount;
			imgInfo.samples = vk::SampleCountFlagBits::e1;
			imgInfo.sharingMode = vk::SharingMode::eExclusive;
			imgInfo.tiling = vk::ImageTiling::eOptimal;
			imgInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
			VmaAllocationCreateInfo imgVmaAllocInfo = {};
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
			if (debugUtils) {
				std::string name = "TextureManager - Texture #" + std::to_string((u64)textureID) + " - Img";
				debugUtils->Helper_SetObjectName(
					device.handle,
					newInner.img,
					name.c_str());
			}

			vk::BufferMemoryBarrier buffBarrier = {};
			buffBarrier.buffer = stagingBuffer.Buffer();
			buffBarrier.offset = stagingBuffer.BufferOffset();
			buffBarrier.size = stagingBuffer.BufferSize();
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
			device.cmdPipelineBarrier(
				cmdBuffer,
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlagBits::eByRegion,
				{},
				buffBarrier,
				imgBarrierA);

			auto buffImgCopies = Std::NewVec<vk::BufferImageCopy>(transientAlloc);
			for (int i = 0; i < imgInfo.mipLevels; i++) {
				vk::BufferImageCopy buffImgCopy = {};
				buffImgCopy.bufferOffset += stagingBuffer.BufferOffset();
				buffImgCopy.bufferOffset += Texas::calculateMipOffset(texFileInfo.textureInfo(), i);
				buffImgCopy.imageExtent = Texas::toVkExtent3D(Texas::calculateMipDimensions(
					texFileInfo.textureInfo().baseDimensions,
					i));
				buffImgCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				buffImgCopy.imageSubresource.layerCount = imgInfo.arrayLayers;
				buffImgCopy.imageSubresource.mipLevel = i;
				buffImgCopies.PushBack(buffImgCopy);
			}
			device.cmdCopyBufferToImage(
				cmdBuffer,
				stagingBuffer.buffer,
				newInner.img,
				vk::ImageLayout::eTransferDstOptimal,
				{ (u32)buffImgCopies.Size(), buffImgCopies.Data() });

			vk::ImageMemoryBarrier imgBarrierB = {};
			imgBarrierB.image = newInner.img;
			imgBarrierB.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			imgBarrierB.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imgBarrierB.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imgBarrierB.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			imgBarrierB.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgBarrierB.subresourceRange.layerCount = imgInfo.arrayLayers;
			imgBarrierB.subresourceRange.levelCount = imgInfo.mipLevels;
			device.cmdPipelineBarrier(
				cmdBuffer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlagBits::eByRegion,
				{},
				{},
				imgBarrierB);

			vk::ImageViewCreateInfo imgViewInfo = {};
			imgViewInfo.format = (vk::Format)Texas::toVkFormat(texFileInfo.textureInfo());
			imgViewInfo.image = newInner.img;
			imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imgViewInfo.subresourceRange.layerCount = (u32)texFileInfo.textureInfo().layerCount;
			imgViewInfo.subresourceRange.levelCount = (u32)texFileInfo.textureInfo().mipCount;
			imgViewInfo.viewType = (vk::ImageViewType)Texas::toVkImageViewType(texFileInfo.textureInfo().textureType);
			newInner.imgView = device.createImageView(imgViewInfo);
			if (debugUtils != nullptr) {
				auto name = std::format("TextureManager - Texture #{}", (u64)textureID);
				debugUtils->Helper_SetObjectName(
					device.handle,
					newInner.imgView,
					name.c_str());
			}

			// Make the descriptor-set and update it
			Std::Array<vk::DescriptorSetLayout, 1> descrSetLayouts = { manager.descrSetLayout.Handle() };
			vk::DescriptorSetAllocateInfo descrSetAllocInfo{};
			descrSetAllocInfo.descriptorPool = manager.descrPool.Handle();
			descrSetAllocInfo.descriptorSetCount = descrSetLayouts.Size();
			descrSetAllocInfo.pSetLayouts = descrSetLayouts.Data();
			vkResult = device.Alloc(descrSetAllocInfo, &newInner.descrSet);
			if (vkResult != vk::Result::eSuccess)
				throw std::runtime_error("DEngine - Vulkan: Could not allocate descriptor set.");
			if (debugUtils) {
				std::string name = "TextureManager - Texture #" + std::to_string((u64)textureID) + " - DescrSet";
				debugUtils->Helper_SetObjectName(
					device.handle,
					newInner.descrSet,
					name.c_str());
			}

			vk::DescriptorImageInfo descrImgInfo = {};
			descrImgInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			descrImgInfo.imageView = newInner.imgView;
			descrImgInfo.sampler = manager.sampler.Handle();
			vk::WriteDescriptorSet descrWrite = {};
			descrWrite.descriptorCount = 1;
			descrWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			descrWrite.dstBinding = 0;
			descrWrite.dstSet = newInner.descrSet;
			descrWrite.pImageInfo = &descrImgInfo;
			device.UpdateDescriptorSets(descrWrite, {});
			manager.database.insert({ textureID, newInner });
		}
	}
}

void TextureManager::Init(
	TextureManager& manager,
	DeviceDispatch const& device,
	QueueData const& queues,
	DebugUtilsDispatch const* debugUtils)
{
	// Make the sampler
	vk::SamplerCreateInfo samplerInfo = {};
	samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	samplerInfo.magFilter = vk::Filter::eLinear;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	samplerInfo.minFilter = vk::Filter::eLinear;
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	auto sampler = device.CreateBox(samplerInfo);
	if (debugUtils != nullptr) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			sampler.Handle(),
			"TextureManager - Sampler");
	}

	// Make descriptor-pool
	vk::DescriptorPoolSize descrPoolSize = {};
	descrPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
	descrPoolSize.descriptorCount = TextureManager::descrPool_minCapacity;
	vk::DescriptorPoolCreateInfo descrPoolInfo{};
	descrPoolInfo.maxSets = TextureManager::descrPool_minCapacity;
	descrPoolInfo.poolSizeCount = 1;
	descrPoolInfo.pPoolSizes = &descrPoolSize;
	auto descrPool = device.CreateBox(descrPoolInfo);
	if (debugUtils != nullptr) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			descrPool.Handle(),
			"TextureManager - DescrPool");
	}

	// Descriptor set layout
	vk::DescriptorSetLayoutBinding descrSetLayoutBinding = {};
	descrSetLayoutBinding.binding = 0;
	descrSetLayoutBinding.descriptorCount = 1;
	descrSetLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descrSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
	vk::DescriptorSetLayoutCreateInfo descrSetLayoutInfo = {};
	descrSetLayoutInfo.bindingCount = 1;
	descrSetLayoutInfo.pBindings = &descrSetLayoutBinding;
	auto descrSetLayout = device.CreateBox(descrSetLayoutInfo);
	if (debugUtils != nullptr) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			descrSetLayout.Handle(),
			"TextureManager - DescrSetLayout");
	}

	// Command pool
	vk::CommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.queueFamilyIndex = queues.graphics.FamilyIndex();
	auto cmdPool = device.CreateBox(cmdPoolInfo);
	if (debugUtils) {
		debugUtils->Helper_SetObjectName(
			device.handle,
			cmdPool.Handle(),
			"TextureManager - CmdPool");
	}

	manager.descrPoolCapacity = descrPoolSize.descriptorCount;
	manager.descrPool = Std::Move(descrPool);
	manager.descrSetLayout = Std::Move(descrSetLayout);
	manager.cmdPool = Std::Move(cmdPool);
	manager.sampler = Std::Move(sampler);
}
