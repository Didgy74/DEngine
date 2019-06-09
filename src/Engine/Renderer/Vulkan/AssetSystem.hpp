#pragma once

#include "volk/volk.hpp"
#include <vulkan/vulkan.hpp>

#include "DeletionQueues.hpp"
#include "QueueInfo.hpp"
#include "MemoryTypes.hpp"
#include "../RendererData.hpp"

#include <functional>

namespace DRenderer::Vulkan
{
	struct VertexBufferObject;
	using VBO = VertexBufferObject;

	struct AssetData;
}

struct DRenderer::Vulkan::VertexBufferObject
{
	enum class Attribute
	{
		Position,
		TexCoord,
		Normal,
		Index,
		COUNT
	};

	vk::Buffer buffer = nullptr;
	vk::DeviceMemory deviceMemory = nullptr;
	std::array<size_t, size_t(Attribute::COUNT)> attributeSizes{};

	uint32_t indexCount = 0;
	vk::IndexType indexType{};

	size_t GetByteOffset(Attribute attribute) const;
	size_t& GetAttrSize(Attribute attr);
	size_t GetAttrSize(Attribute attr) const;
	size_t GetTotalBufferSize() const;
};

struct DRenderer::Vulkan::AssetData
{
	vk::Device device = nullptr;

	QueueInfo queueInfo{};

	std::unordered_map<size_t, VBO> vboDatabase;

	vk::CommandPool cmdPool = nullptr;
	// Has length of resource-set-count
	std::vector<vk::CommandBuffer> cmdBuffers;
	std::vector<vk::Semaphore> vboLoadingFinished;
	std::vector<vk::Fence> transferDoneFence;
	size_t currentSet = 0;
	size_t setCount = 0;

	std::vector<vk::Semaphore> waitSemaphoreQueue;
	std::vector<vk::PipelineStageFlags> waitStageFlagsQueue;
};

namespace DRenderer::Vulkan::AssetSystem
{
	void Init(
		vk::Device device, 
		const QueueInfo& queueInfo, 
		uint32_t resourceCount, 
		AssetData& assetSystem);

	void Terminate(AssetData& assetData);

	void LoadAssets(
		AssetData& assetSystem,
		const MemoryTypes& memTypes,
		const QueueInfo& queueInfo,
		DeletionQueues& deleteionQueues,
		const Core::PrepareRenderingEarlyParams& in);

	std::pair<const std::vector<vk::Semaphore>&, const std::vector<vk::PipelineStageFlags>&> GetWaitSemaphores(const AssetData&);

	void SubmittedFrame(AssetData& assetSystem);
}