#include "QueueData.hpp"

#include "DynamicDispatch.hpp"

void DEngine::Gfx::Vk::QueueData::SafeQueue::submit(vk::ArrayProxy<vk::SubmitInfo const> submits, vk::Fence fence) const
{
	std::lock_guard lockGuard(m_lock);
	m_device->raw.vkQueueSubmit(
		static_cast<VkQueue>(m_handle),
		submits.size(),
		reinterpret_cast<VkSubmitInfo const*>(submits.data()),
		static_cast<VkFence>(fence));
}

vk::Result DEngine::Gfx::Vk::QueueData::SafeQueue::presentKHR(vk::PresentInfoKHR const& presentInfo) const
{
	std::lock_guard lockGuard(m_lock);
	return m_device->queuePresentKHR(m_handle, presentInfo);
}

void DEngine::Gfx::Vk::QueueData::SafeQueue::waitIdle() const
{
	std::lock_guard lockGuard(m_lock);
	m_device->raw.vkQueueWaitIdle(static_cast<VkQueue>(m_handle));
}

namespace DEngine::Gfx::Vk
{
	enum class QueueType : u8
	{
		Graphics,
		Transfer,
		Compute
	};
}

void DEngine::Gfx::Vk::QueueData::SafeQueue::Initialize(
	SafeQueue& queue,
	DeviceDispatch const& device, 
	u8 queueType,
	u32 familyIndex, 
	u32 queueIndex, 
	DebugUtilsDispatch const* debugUtils)
{
	queue.m_device = &device;
	queue.m_familyIndex = familyIndex;
	queue.m_queueIndex = queueIndex;
	queue.m_handle = device.getQueue(familyIndex, queueIndex);

	if (debugUtils)
	{
		std::string name = std::string("Queue #") + std::to_string(familyIndex) + std::string(" - ");
		switch ((QueueType)queueType)
		{
		case QueueType::Graphics:
			name += "Graphics";
			break;
		case QueueType::Transfer:
			name += "Transfer";
			break;
		case QueueType::Compute:
			name += "Compute";
			break;
		}

		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectHandle = (u64)(VkQueue)queue.m_handle;
		nameInfo.objectType = queue.m_handle.objectType;
		nameInfo.pObjectName = name.data();
		debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
	}
}

void DEngine::Gfx::Vk::QueueData::Initialize(
	QueueData& queueData,
	DeviceDispatch const& device, 
	QueueIndices indices, 
	DebugUtilsDispatch const* debugUtils)
{
	SafeQueue::Initialize(
		queueData.graphics,
		device, 
		(u8)QueueType::Graphics,
		indices.graphics.familyIndex, 
		indices.graphics.queueIndex, 
		debugUtils);
	
	if (indices.transfer.familyIndex != invalidIndex)
	{
		SafeQueue::Initialize(
			queueData.transfer,
			device, 
			(u8)QueueType::Transfer,
			indices.transfer.familyIndex, 
			indices.transfer.queueIndex, 
			debugUtils);
	}
		
	if (indices.compute.familyIndex != invalidIndex)
	{
		SafeQueue::Initialize(
			queueData.compute,
			device,
			(u8)QueueType::Compute,
			indices.transfer.familyIndex,
			indices.transfer.queueIndex, 
			debugUtils);
	}
		
}
