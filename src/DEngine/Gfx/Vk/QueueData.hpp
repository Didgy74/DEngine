#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "VulkanIncluder.hpp"

#include <mutex>

namespace DEngine::Gfx::Vk
{
    class DebugUtilsDispatch;
    class DeviceDispatch;

    struct QueueIndices
    {
        static constexpr u32 invalidIndex = static_cast<u32>(-1);

        struct Indices
        {
            u32 familyIndex = invalidIndex;
            u32 queueIndex = invalidIndex;
        };

        Indices graphics{};
        Indices transfer{};
        Indices compute{};
    };

    class QueueData
    {
    public:
        static constexpr u32 invalidIndex = static_cast<u32>(-1);

        class SafeQueue
        {
        public:
            [[nodiscard]] u32 FamilyIndex() const { return m_familyIndex; }
            [[nodiscard]] u32 Index() const { return m_queueIndex; }
            [[nodiscard]] vk::Queue Handle() const { return m_handle; }
            
            void submit(vk::ArrayProxy<vk::SubmitInfo const> submits, vk::Fence fence) const;
            [[nodiscard]] vk::Result presentKHR(vk::PresentInfoKHR const& presentInfo) const;
            void waitIdle() const;

            static void Initialize(
                SafeQueue& queue,
                DeviceDispatch const& device,
                u8 queueType,
                u32 familyIndex,
                u32 queueIndex,
                DebugUtilsDispatch const* debugUtils);

        private:
            mutable std::mutex m_lock{};
            u32 m_familyIndex = invalidIndex;
            u32 m_queueIndex = invalidIndex;
            vk::Queue m_handle{};
            DeviceDispatch const* m_device = nullptr;

            friend class DeviceDispatch;
        };

        SafeQueue graphics{};
        SafeQueue transfer{};
        [[nodiscard]] inline bool HasTransfer() const { return transfer.Handle() != vk::Queue(); }
        SafeQueue compute{};
        [[nodiscard]] inline bool HasCompute() const { return compute.Handle() != vk::Queue(); }

        static void Init(
            QueueData& queueData,
            DeviceDispatch const& device,
            QueueIndices indices,
            DebugUtilsDispatch const* debugUtils);
    };
}