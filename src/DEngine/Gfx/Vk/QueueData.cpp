#include "QueueData.hpp"

void DEngine::Gfx::Vk::QueueData::SafeQueue::submit(vk::ArrayProxy<vk::SubmitInfo const> submits, vk::Fence fence) const
{
	std::lock_guard lockGuard(m_lock);
	m_device->queueSubmit(m_handle, submits, fence);
}

vk::Result DEngine::Gfx::Vk::QueueData::SafeQueue::presentKHR(vk::PresentInfoKHR const& presentInfo) const
{
	std::lock_guard lockGuard(m_lock);
	return m_device->queuePresentKHR(m_handle, presentInfo);
}

void DEngine::Gfx::Vk::QueueData::SafeQueue::waitIdle() const
{
	std::lock_guard lockGuard(m_lock);
	m_device->waitQueueIdle(m_handle);
}

void DEngine::Gfx::Vk::QueueData::SafeQueue::Initialize(
	DevDispatch const& device, 
	u32 familyIndex, 
	u32 queueIndex, 
	SafeQueue& queue)
{
	queue.m_device = &device;
	queue.m_familyIndex = familyIndex;
	queue.m_queueIndex = queueIndex;
	queue.m_handle = device.getQueue(familyIndex, queueIndex);
}

void DEngine::Gfx::Vk::QueueData::Initialize(
	DevDispatch const& device, 
	QueueIndices indices, 
	QueueData& queueData)
{
	SafeQueue::Initialize(device, indices.graphics.familyIndex, indices.graphics.queueIndex, queueData.graphics);
	
	if (indices.transfer.familyIndex != invalidIndex)
		SafeQueue::Initialize(device, indices.transfer.familyIndex, indices.transfer.queueIndex, queueData.transfer);

	if (indices.compute.familyIndex != invalidIndex)
		SafeQueue::Initialize(device, indices.transfer.familyIndex, indices.transfer.queueIndex, queueData.compute);
}
