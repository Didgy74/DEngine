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

    [[nodiscard]] inline GUIData CreateGUIData(
        DeviceDispatch const& device,
        vk::Format swapchainFormat,
        std::uint8_t resourceSetCount,
        vk::Extent2D swapchainDimensions,
        vk::Queue vkQueue,
        DebugUtilsDispatch const* debugUtils)
    {
        vk::Result vkResult{};
        GUIData returnVal{};

        // Create the renderpass
        {
            vk::AttachmentDescription colorAttachment{};
            colorAttachment.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
            colorAttachment.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
            colorAttachment.format = swapchainFormat;
            colorAttachment.samples = vk::SampleCountFlagBits::e1;
            colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
            colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
            colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
            vk::AttachmentDescription attachments[1] = { colorAttachment };
            vk::AttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
            vk::SubpassDescription subpassDescription{};
            subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
            subpassDescription.colorAttachmentCount = 1;
            subpassDescription.pColorAttachments = &colorAttachmentRef;
            // Set up render pass
            vk::RenderPassCreateInfo createInfo{};
            createInfo.attachmentCount = 1;
            createInfo.pAttachments = attachments;
            createInfo.subpassCount = 1;
            createInfo.pSubpasses = &subpassDescription;

            returnVal.renderPass = device.createRenderPass(createInfo);
            if (debugUtils != nullptr)
            {
                vk::DebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.objectHandle = (u64)(VkRenderPass)returnVal.renderPass;
                nameInfo.objectType = returnVal.renderPass.objectType;
                nameInfo.pObjectName = "GUI RenderPass";
                debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
            }
        }

        {
            // Create the viewport sampler

            vk::SamplerCreateInfo samplerInfo{};
            samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
            samplerInfo.magFilter = vk::Filter::eNearest;
            samplerInfo.minFilter = vk::Filter::eNearest;
            samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
            returnVal.viewportSampler = device.createSampler(samplerInfo);
        }

        // Create the commandbuffers
        {
            vk::CommandPoolCreateInfo cmdPoolInfo{};
            cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            returnVal.cmdPool = device.createCommandPool(cmdPoolInfo);
            if (debugUtils != nullptr)
            {
                vk::DebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.objectHandle = (u64)(VkCommandPool)returnVal.cmdPool;
                nameInfo.objectType = returnVal.cmdPool.objectType;
                nameInfo.pObjectName = "GUI Rendering CmdPool";
                debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
            }

            vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
            cmdBufferAllocInfo.commandPool = returnVal.cmdPool;
            cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
            cmdBufferAllocInfo.commandBufferCount = resourceSetCount;
            returnVal.cmdBuffers.resize(cmdBufferAllocInfo.commandBufferCount);
            vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, returnVal.cmdBuffers.data());
            if (vkResult != vk::Result::eSuccess)
                throw std::runtime_error("Vulkan: Unable to allocate command cmdBuffers for GUI rendering.");
        }

        returnVal.renderTarget = CreateGUIRenderTarget(device, vkQueue, swapchainDimensions, swapchainFormat, returnVal.renderPass, debugUtils);

        return returnVal;
    }

    void InitializeImGui(APIData& apiData, PFN_vkGetInstanceProcAddr instanceProcAddr);

    vk::RenderPass BuildMainGfxRenderPass(
        DeviceDispatch const& device,
        bool useEditorPipeline,
        DebugUtilsDispatch const* debugUtils);

    void TransitionGfxImage(
        DeviceDispatch const& device,
        vk::Image img,
        vk::Queue queue,
        bool useEditorPipeline);

    inline GfxRenderTarget InitializeMainRenderingStuff(
        DeviceDispatch const& device,
        std::uint8_t resourceSetCount,
        std::uint8_t viewportID,
        std::uint32_t deviceLocalMemType,
        vk::Extent2D viewportSize,
        vk::Extent2D maxSize,
        vk::RenderPass renderPass,
        vk::Queue queue,
        bool useEditorPipeline,
        DebugUtilsDispatch const* debugUtils)
    {
        vk::Result vkResult{};

        GfxRenderTarget returnVal{};
        returnVal.extent = viewportSize;

        // First we make a temp image that has max size, so we won't have to re-allocate memory later when resizing this image.
        vk::ImageCreateInfo imageInfo{};
        imageInfo.arrayLayers = 1;
        imageInfo.extent = vk::Extent3D{ maxSize.width, maxSize.height, 1 };
        imageInfo.format = vk::Format::eR8G8B8A8Srgb;
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.mipLevels = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
        if (useEditorPipeline)
        {
            // We want to sample from the image to show it in the editor.
            imageInfo.usage |= vk::ImageUsageFlagBits::eSampled;
        }

        vk::Image tempMaxImage = device.createImage(imageInfo);

        vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(tempMaxImage);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = deviceLocalMemType;
        
        device.destroyImage(tempMaxImage);

        vk::DeviceMemory memory = device.allocateMemory(allocInfo);
        returnVal.memory = memory;
        returnVal.memorySize = allocInfo.allocationSize;
        if (debugUtils)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkDeviceMemory)returnVal.memory;
            nameInfo.objectType = returnVal.memory.objectType;
            std::string name = std::string("Graphics viewport #0") + std::to_string(viewportID) + " - Memory";
            nameInfo.pObjectName = name.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        // Now we make the image we actually want to render to, which is likely smaller
        imageInfo.extent = vk::Extent3D{ viewportSize.width, viewportSize.height, 1 };
        vk::Image img = device.createImage(imageInfo);
        returnVal.img = img;
        if (debugUtils)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImage)returnVal.img;
            nameInfo.objectType = returnVal.img.objectType;
            std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Image";
            nameInfo.pObjectName = name.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        device.bindImageMemory(img, memory, 0);

        // We have to transition this image
        TransitionGfxImage(device, img, queue, useEditorPipeline);

        // Make the image view
        vk::ImageViewCreateInfo imgViewInfo{};
        imgViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imgViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imgViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imgViewInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imgViewInfo.format = vk::Format::eR8G8B8A8Srgb;
        imgViewInfo.image = img;
        imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewInfo.subresourceRange.baseArrayLayer = 0;
        imgViewInfo.subresourceRange.baseMipLevel = 0;
        imgViewInfo.subresourceRange.layerCount = 1;
        imgViewInfo.subresourceRange.levelCount = 1;
        imgViewInfo.viewType = vk::ImageViewType::e2D;
            
        vk::ImageView imgView = device.createImageView(imgViewInfo);
        returnVal.imgView = imgView;
        if (debugUtils)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImageView)returnVal.imgView;
            nameInfo.objectType = returnVal.imgView.objectType;
            std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Image View";
            nameInfo.pObjectName = name.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        vk::FramebufferCreateInfo fbInfo{};
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &returnVal.imgView;
        fbInfo.height = viewportSize.height;
        fbInfo.layers = 1;
        fbInfo.renderPass = renderPass;
        fbInfo.width = viewportSize.width;
        vk::Framebuffer framebuffer = device.createFramebuffer(fbInfo);
        returnVal.framebuffer = framebuffer;
        if (debugUtils)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkFramebuffer)returnVal.framebuffer;
            nameInfo.objectType = returnVal.framebuffer.objectType;
            std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - Framebuffer";
            nameInfo.pObjectName = name.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }


        // Make the command pool
        vk::CommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        returnVal.cmdPool = device.createCommandPool(cmdPoolInfo);
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkCommandPool)returnVal.cmdPool;
            nameInfo.objectType = returnVal.cmdPool.objectType;
            std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - CmdPool";
            nameInfo.pObjectName = name.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
        cmdBufferAllocInfo.commandPool = returnVal.cmdPool;
        cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        cmdBufferAllocInfo.commandBufferCount = resourceSetCount;
        returnVal.cmdBuffers.resize(cmdBufferAllocInfo.commandBufferCount);
        vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, returnVal.cmdBuffers.data());
        if (vkResult != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan: Unable to allocate command cmdBuffers for GUI rendering.");


        return returnVal;
    }
}