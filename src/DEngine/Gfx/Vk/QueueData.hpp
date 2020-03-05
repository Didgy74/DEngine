#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"

#include <mutex>

namespace DEngine::Gfx::Vk
{
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

    struct QueueData
    {
    public:
        static constexpr u32 invalidIndex = static_cast<u32>(-1);

        class SafeQueue
        {
        public:
            [[nodiscard]] u32 FamilyIndex() const { return m_familyIndex; }
            [[nodiscard]] u32 Index() const { return m_queueIndex; }
            [[nodiscard]] vk::Queue Handle() const { return m_handle; }
            
            [[nodiscard]] void submit(vk::ArrayProxy<vk::SubmitInfo const> submits, vk::Fence fence) const;
            [[nodiscard]] vk::Result presentKHR(vk::PresentInfoKHR const& presentInfo) const;
            [[nodiscard]] void waitIdle() const;

            static void Initialize(
                DevDispatch const& device,
                u8 queueType,
                u32 familyIndex,
                u32 queueIndex,
                DebugUtilsDispatch const* debugUtils,
                SafeQueue& queue);

        private:
            mutable std::mutex m_lock{};
            u32 m_familyIndex = invalidIndex;
            u32 m_queueIndex = invalidIndex;
            vk::Queue m_handle{};
            DevDispatch const* m_device{};
        };

        SafeQueue graphics{};
        SafeQueue transfer{};
        [[nodiscard]] inline bool HasTransfer() const { return transfer.Handle() != vk::Queue(); }
        SafeQueue compute{};
        [[nodiscard]] inline bool HasCompute() const { return compute.Handle() != vk::Queue(); }

        static void Initialize(
            DevDispatch const& device,
            QueueIndices indices,
            DebugUtilsDispatch const* debugUtils,
            QueueData& queueData);
    };
}