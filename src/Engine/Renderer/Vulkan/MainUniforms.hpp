#pragma once

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include "DeletionQueues.hpp"
#include "MemoryTypes.hpp"
#include "Constants.hpp"

#include <array>

namespace DRenderer::Vulkan
{
	struct MainUniforms
	{
		struct Constants
		{
			static constexpr size_t minimumObjectDataCapacity = 64;
			static constexpr size_t cameraDataBindingIndex = 0;
			static constexpr size_t objectDataBindingIndex = 1;

			Constants() = delete;
			Constants(const Constants&) = delete;
			Constants(Constants&&) = delete;
		};

		static void Init_CreateDescriptorSetAndPipelineLayout(MainUniforms& data, vk::Device device);
		static void Init_AllocateDescriptorSets(MainUniforms& data, uint32_t resourceSetCount);
		static void Init_BuildBuffers(
			MainUniforms& data,
			const MemoryTypes& memoryInfo,
			const vk::PhysicalDeviceMemoryProperties& memProperties,
			const vk::PhysicalDeviceLimits& limits);
		static void Init_ConfigureDescriptors(MainUniforms& data);
		static void Update(MainUniforms& data, size_t objectCount, DeletionQueues& deletionQueues);
		static void Terminate(MainUniforms& data);

		vk::Device device{};
		uint32_t resourceSetCount = 0;
		uint32_t memoryTypeIndex = Vulkan::Constants::invalidIndex;
		size_t nonCoherentAtomSize = 0;

		vk::DescriptorSetLayout descrSetLayout = nullptr;
		vk::PipelineLayout pipelineLayout = nullptr;
		vk::DescriptorPool descrSetPool = nullptr;
		// Has length of resource set count
		std::array<vk::DescriptorSet, Vulkan::Constants::maxResourceSets> descrSets{};
		std::array<vk::DescriptorUpdateTemplate, Vulkan::Constants::maxResourceSets> descrUpdateTemplates{};
		std::array<vk::DescriptorBufferInfo, Vulkan::Constants::maxResourceSets> descrUpdateData{};

		vk::DeviceMemory cameraBuffersMem = nullptr;
		vk::Buffer cameraBuffer = nullptr;
		uint8_t* cameraMemoryMap = nullptr;
		size_t cameraDataResourceSetSize = 0;

		uint8_t* GetCameraBufferResourceSet(uint32_t resourceSet);
		size_t GetCameraResourceSetSize() const;
		// Grabs the offset to the resource-set from the start of the whole camera-resource set buffer.
		size_t GetCameraResourceSetOffset(uint32_t resourceSet) const;

		vk::DeviceMemory objectDataMemory = nullptr;
		vk::Buffer objectDataBuffer = nullptr;
		size_t objectDataSize = 0;
		size_t objectDataResourceSetSize = 0;
		uint8_t* objectDataMappedMem = nullptr;

		// Grabs a pointer to the resource set at given resource set index
		uint8_t* GetObjectDataResourceSet(uint32_t resourceSet);
		// Grabs the offset from start of buffer to specified resource set.
		size_t GetObjectDataResourceSetOffset(uint32_t resourceSet) const;
		// Grabs the size of one resource set
		size_t GetObjectDataResourceSetSize() const;
		// Grabs the offset from start of resource set to specified object index
		size_t GetObjectDataDynamicOffset(size_t modelDataIndex) const;
		// Gets the size of one object's entry in the buffer.
		size_t GetObjectDataSize() const;
		// Grabs the amount of objects the buffer can hold.
		size_t GetObjectDataCapacity() const;
	};
}