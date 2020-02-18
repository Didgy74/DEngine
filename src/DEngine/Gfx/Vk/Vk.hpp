#pragma once

#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"
#include "Constants.hpp"

#include "DEngine/Gfx/Gfx.hpp"

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

        inline bool dedicatedTransfer() const
        {
            return transfer.familyIndex != invalidIndex && transfer.familyIndex != graphics.familyIndex;
        }
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
        vk::CompositeAlphaFlagBitsKHR compositeAlphaFlag = {};

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
        vk::CompositeAlphaFlagBitsKHR compositeAlphaFlag = {};
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
        std::uint8_t uid = 0;

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

    struct GfxRenderTarget
    {
        vk::Extent2D extent{};
        vk::DeviceMemory memory{};
        vk::DeviceSize memorySize{};
        vk::Image img{};
        vk::ImageView imgView{};
        vk::Framebuffer framebuffer{};
        vk::CommandPool cmdPool{};
        Cont::FixedVector<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
    };

    struct GUIRenderTarget
    {
        vk::Extent2D extent{};
        vk::Format format{};
        vk::DeviceMemory memory{};
        vk::DeviceSize memorySize{};
        vk::Image img{};
        vk::ImageView imgView{};
        vk::Framebuffer framebuffer{};
    };

    struct GUIData
    {
        vk::RenderPass renderPass{};
        vk::Sampler viewportSampler = vk::Sampler{};
        GUIRenderTarget renderTarget{};
        vk::CommandPool cmdPool{};
        // Has length of resource sets
        Cont::FixedVector<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
    };

    struct APIData
    {
        bool (*createVkSurfacePFN)(u64 vkInstance, void* userData, u64* vkSurface) = nullptr;
        void* createVkSurfaceUserData = nullptr;

        InstanceDispatch instance{};
        DebugUtilsDispatch debugUtils{};
        DebugUtilsDispatch const* DebugUtilsPtr() const { return debugUtils.raw.vkCmdBeginDebugUtilsLabelEXT != nullptr ? &debugUtils : nullptr; }
        vk::DebugUtilsMessengerEXT debugMessenger{};
        Vk::DeviceDispatch device{};

        Gfx::ILog* logger = nullptr;

        PhysDeviceInfo physDevice{};
        SurfaceInfo surface{};
        SwapchainData swapchain{};

        vk::Queue renderQueue{};

        std::uint8_t resourceSetCount = 0;
        std::uint8_t currentResourceSet = 0;

        Cont::FixedVector<vk::Fence, Constants::maxResourceSets> mainFences{};

        GUIData guiData{};

        // The main renderpass for rendering the 3D stuff
        vk::RenderPass gfxRenderPass{};
        Cont::FixedVector<GfxRenderTarget, Gfx::Constants::maxViewportCount> viewportRenderTargets{};

        vk::PipelineLayout testPipelineLayout{};
        vk::Pipeline testPipeline{};
    };
}