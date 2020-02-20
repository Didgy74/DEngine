#pragma once

#include "Vk.hpp"
#include "VulkanIncluder.hpp"
#include "DynamicDispatch.hpp"

#include "DEngine/Containers/Span.hpp"

namespace DEngine::Gfx::Vk::Init
{
    struct CreateVkInstance_Return
    {
        vk::Instance instanceHandle{};
        bool debugUtilsEnabled = false;
    };
    CreateVkInstance_Return CreateVkInstance(
        Cont::Span<char const*> requiredExtensions,
        bool enableLayers,
        BaseDispatch const& baseDispatch,
        ILog* logger);

    vk::DebugUtilsMessengerEXT CreateLayerMessenger(
        vk::Instance instanceHandle,
        DebugUtilsDispatch const* debugUtilsOpt,
        const void* userData);

    PhysDeviceInfo LoadPhysDevice(
        InstanceDispatch const& instance,
        vk::SurfaceKHR surface);

    vk::Device CreateDevice(
        InstanceDispatch const& instance,
        PhysDeviceInfo const& physDevice);

    SurfaceInfo BuildSurfaceInfo(
        InstanceDispatch const& instance,
        vk::PhysicalDevice physDevice,
        vk::SurfaceKHR surface,
        ILog* logger);

    void RebuildSurfaceInfo(
        InstanceDispatch const& instance,
        PhysDeviceInfo const& physDevice,
        vk::SurfaceKHR newSurface,
        SurfaceInfo& outSurfaceInfo);

    SwapchainSettings BuildSwapchainSettings(
        InstanceDispatch const& instance,
        vk::PhysicalDevice physDevice,
        SurfaceInfo const& surfaceCaps,
        ILog* logger);

    SwapchainData CreateSwapchain(
        Vk::DeviceDispatch const& device,
        vk::Queue vkQueue,
        SwapchainSettings const& settings,
        DebugUtilsDispatch const* debugUtilsOpt);

    void RecreateSwapchain(
        InstanceDispatch const& instance,
        DevDispatch const& device,
        DebugUtilsDispatch const* debugUtils,
        vk::Queue queue,
        vk::PhysicalDevice physDevice,
        SurfaceInfo& surface,
        SwapchainData& swapchain);

    bool TransitionSwapchainImages(
        DeviceDispatch const& device,
        vk::Queue vkQueue,
        Cont::Span<const vk::Image> images);

    void RecordSwapchainCmdBuffers(
        DeviceDispatch const& device,
        SwapchainData const& swapchainData,
        vk::Image srcImg);

    [[nodiscard]] GUIRenderTarget CreateGUIRenderTarget(
        DeviceDispatch const& device,
        vk::Queue vkQueue,
        vk::Extent2D swapchainDimensions,
        vk::Format swapchainFormat,
        vk::RenderPass renderPass,
        DebugUtilsDispatch const* debugUtils);

    void RecreateGUIRenderTarget(
        DevDispatch const& device,
        vk::Extent2D swapchainDimensions,
        vk::Queue vkQueue,
        vk::RenderPass renderPass,
        GUIRenderTarget& renderTarget,
        DebugUtilsDispatch const* debugUtils);

    [[nodiscard]] GUIData CreateGUIData(
        DeviceDispatch const& device,
        vk::Format swapchainFormat,
        u8 resourceSetCount,
        vk::Extent2D swapchainDimensions,
        vk::Queue vkQueue,
        DebugUtilsDispatch const* debugUtils);

    void InitializeImGui(
        APIData& apiData,
        DevDispatch const& device,
        PFN_vkGetInstanceProcAddr instanceProcAddr,
        DebugUtilsDispatch const* debugUtils);

    vk::RenderPass BuildMainGfxRenderPass(
        DeviceDispatch const& device,
        bool useEditorPipeline,
        DebugUtilsDispatch const* debugUtils);

    void TransitionGfxImage(
        DeviceDispatch const& device,
        vk::Image img,
        vk::Queue queue,
        bool useEditorPipeline);

    GfxRenderTarget InitializeGfxViewport(
        DeviceDispatch const& device,
        u8 viewportID,
        u32 deviceLocalMemType,
        vk::Extent2D viewportSize,
        vk::RenderPass renderPass,
        vk::Queue queue,
        bool useEditorPipeline,
        DebugUtilsDispatch const* debugUtils);

    void ResizeGfxViewport(
        DevDispatch const& device,
        vk::Queue queue,
        vk::RenderPass renderPass,
        u8 viewportID,
        void* imguiTextureID,
        vk::Extent2D newSize,
        bool useEditorPipeline,
        GfxRenderTarget& viewportRef,
        DebugUtilsDispatch const* debugUtils);
}