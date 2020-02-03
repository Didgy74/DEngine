#pragma once

#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"
#include "Constants.hpp"

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/FixedVector.hpp"
#include "DEngine/Containers/Optional.hpp"

namespace DEngine::Gfx::Vk
{
    template<typename T>
    [[nodiscard]] inline constexpr bool isValidIndex(T in) = delete;
    template<>
    [[nodiscard]] inline constexpr bool isValidIndex<std::uint32_t>(std::uint32_t in) { return in != static_cast<std::uint32_t>(-1); }
    template<>
    [[nodiscard]] inline constexpr bool isValidIndex<std::uint64_t>(std::uint64_t in) { return in != static_cast<std::uint64_t>(-1); }

    struct QueueInfo
    {
        static constexpr u32 invalidIndex = static_cast<u32>(-1);

        struct Queue
        {
            vk::Queue handle = nullptr;
            u32 familyIndex = invalidIndex;
            u32 queueIndex = invalidIndex;
        };

        Queue graphics{};
        Queue transfer{};

        inline bool dedicatedTransfer() const { return transfer.familyIndex != graphics.familyIndex; }
    };

    struct MemoryTypes
    {
        static constexpr u32 invalidIndex = static_cast<u32>(-1);

        u32 deviceLocal = invalidIndex;
        u32 hostVisible = invalidIndex;
        u32 deviceLocalAndHostVisible = invalidIndex;

        inline bool unifiedMemory() const { return deviceLocal == hostVisible && deviceLocal != invalidIndex && hostVisible != invalidIndex; }
    };

    struct PhysDeviceInfo
    {
        vk::PhysicalDevice handle = nullptr;
        vk::PhysicalDeviceProperties properties{};
        vk::PhysicalDeviceMemoryProperties memProperties{};
        vk::SampleCountFlagBits maxFramebufferSamples{};
        QueueInfo preferredQueues{};
        MemoryTypes memInfo{};
    };

    struct SurfaceInfo
    {
    public:
        vk::SurfaceKHR handle{};
        vk::SurfaceCapabilitiesKHR capabilities{};

        Cont::FixedVector<vk::PresentModeKHR, Constants::maxAvailablePresentModes> supportedPresentModes;
        Cont::FixedVector<vk::SurfaceFormatKHR, Constants::maxAvailableSurfaceFormats> supportedSurfaceFormats;
    };

    struct SwapchainSettings
    {
        vk::SurfaceKHR surface{};
        vk::SurfaceCapabilitiesKHR capabilities{};
        vk::PresentModeKHR presentMode{};
        vk::SurfaceFormatKHR surfaceFormat{};
        vk::SurfaceTransformFlagBitsKHR transform = {};
        vk::Extent2D extents{};
        u32 numImages{};
    };

    struct FencedCmdBuffer
    {
        vk::CommandBuffer handle{};
        vk::Fence fence{};
    };

    struct SwapchainData
    {
        vk::SwapchainKHR handle{};
        std::uint_least8_t uid = 0;

        vk::Extent2D extents{};
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
        vk::SurfaceFormatKHR surfaceFormat{};

        Cont::FixedVector<vk::Image, Constants::maxSwapchainLength> images{};

        vk::CommandPool cmdPool{};
        Cont::FixedVector<vk::CommandBuffer, Constants::maxSwapchainLength> cmdBuffers{};
        vk::Semaphore imageAvailableSemaphore{};
    };

    struct Device
    {
        vk::Device handle{};
        vk::PhysicalDevice physDeviceHandle{};
    };

    struct GUIRenderTarget
    {
        vk::DeviceMemory memory{};
        vk::DeviceSize memorySize{};
        vk::Format format{};
        vk::Extent2D extent{};
        vk::Image img{};
        vk::ImageView imgView{};
        vk::RenderPass renderPass{};
        vk::Framebuffer framebuffer{};
        vk::Semaphore imguiRenderFinished{};
    };

    struct GUIRenderCmdBuffers
    {
        vk::CommandPool cmdPool{};
        // Has length of resource sets
        Cont::FixedVector<FencedCmdBuffer, Constants::maxResourceSets> cmdBuffers{};
    };

    struct APIData
    {
        InstanceDispatch instance{};

        DebugUtilsDispatch debugUtils{};
        DebugUtilsDispatch const* DebugUtilsPtr() const { return debugUtils.raw.vkCmdBeginDebugUtilsLabelEXT != nullptr ? &debugUtils : nullptr; }
        vk::DebugUtilsMessengerEXT debugMessenger{};

        std::uint_least8_t resourceSetCount = 0;
        std::uint_least8_t currentResourceSet = 0;

        PhysDeviceInfo physDeviceInfo{};

        SurfaceInfo surface{};

        Vk::DeviceDispatch device{};

        vk::Queue renderQueue{};

        SwapchainData swapchain{};

        GUIRenderTarget guiRenderTarget{};

        GUIRenderCmdBuffers guiRenderCmdBuffers{};

        vk::PipelineLayout testPipelineLayout{};
        vk::Pipeline testPipeline{};
    };
}