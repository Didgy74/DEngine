#include "Vk.hpp"
#include "DEngine/Gfx/VkInterface.hpp"
#include "DEngine/Gfx/Assert.hpp"

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Span.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/FixedVector.hpp"

#include <string>

#undef max

//vk::DispatchLoaderDynamic vk::defaultDispatchLoaderDynamic;

namespace DEngine::Gfx::Vk
{
    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        vk::DebugUtilsMessengerCallbackDataEXT const* pCallbackData,
        void* pUserData)
    {
        ILog const* logger = static_cast<ILog const*>(pUserData);

        if (logger != nullptr)
            logger->log(pCallbackData->pMessage);

        return 0;
    }

    namespace Init
    {
        struct CreateVkInstanceResult
        {
            vk::Instance instanceHandle{};
            bool debugUtilsEnabled = false;
        };
        CreateVkInstanceResult CreateVkInstance(
            Cont::Span<char const*> requiredExtensions,
            bool enableLayers,
            BaseDispatch const& baseDispatch);

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

        SwapchainData CreateSwapchain(
            Vk::DeviceDispatch const& device,
            vk::Queue vkQueue,
            SwapchainSettings const& settings,
            DebugUtilsDispatch const* debugUtilsOpt);

        bool RecreateSwapchain(
            SwapchainData& swapchainData, 
            vk::Device vkDevice, 
            vk::Queue vkQueue, 
            SwapchainSettings settings);

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
            DebugUtilsDispatch const* debugUtilsOpt);

        GUIRenderCmdBuffers MakeImguiRenderCmdBuffers(
            DeviceDispatch const& device,
            u32 resourceSetCount,
            DebugUtilsDispatch const* debugUtils);

        void TestImageBleh(APIData& apiData);

        void Test(APIData& apiData);
    }

    SurfaceInfo BuildSurfaceInfo(
        InstanceDispatch const& instance,
        vk::PhysicalDevice physDevice,
        vk::SurfaceKHR surface);

    SwapchainSettings BuildSwapchainSettings(
        InstanceDispatch const& instance,
        vk::PhysicalDevice physDevice,
        SurfaceInfo const& surfaceCaps);
}

namespace DEngine::Gfx::Vk
{
    void RebuildSwapchain(
        InstanceDispatch const& instance, 
        DevDispatch const& device,
        DebugUtilsDispatch const * debugUtils,
        vk::Queue queue,
        vk::PhysicalDevice physDevice,
        SurfaceInfo& surface,
        SwapchainData& swapchain)
    {
        // Query surface for new details
        vk::Result vkResult{};

        vk::SurfaceCapabilitiesKHR capabilities = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface.handle);
        surface.capabilities = capabilities;

        SwapchainSettings settings = {};
        settings.surface = surface.handle;
        settings.numImages = static_cast<std::uint32_t>(swapchain.images.size());
        settings.presentMode = swapchain.presentMode;
        settings.surfaceFormat = swapchain.surfaceFormat;
        settings.capabilities = surface.capabilities;
        settings.transform = surface.capabilities.currentTransform;
        settings.extents = settings.capabilities.currentExtent;

        // We have figured out the settings to build the new swapchainData, now we actually make it

        swapchain.extents = settings.extents;
        swapchain.surfaceFormat = settings.surfaceFormat;
        swapchain.presentMode = settings.presentMode;

        vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageExtent = settings.extents;
        swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapchainCreateInfo.presentMode = settings.presentMode;
        swapchainCreateInfo.surface = settings.surface;
        swapchainCreateInfo.preTransform = settings.transform;
        swapchainCreateInfo.clipped = 1;
        swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
        swapchainCreateInfo.minImageCount = settings.numImages;
        swapchainCreateInfo.oldSwapchain = swapchain.handle;

        swapchain.handle = device.createSwapchainKHR(swapchainCreateInfo);
        // Make name for the swapchainData
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkSwapchainKHR)swapchain.handle;
            nameInfo.objectType = swapchain.handle.objectType;
            std::string objectName = std::string("Swapchain #0") + std::to_string(swapchain.uid);
            nameInfo.pObjectName = objectName.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        u32 swapchainImageCount = 0;
        vkResult = device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, nullptr);
        if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
            throw std::runtime_error("Unable to grab swapchainData images from VkSwapchainKHR object.");
        device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, swapchain.images.data());
        // Make names for the swapchainData images
        if (debugUtils != nullptr)
        {
            for (uSize i = 0; i < swapchain.images.size(); i++)
            {
                vk::DebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.objectHandle = (u64)(VkImage)swapchain.images[i];
                nameInfo.objectType = swapchain.images[i].objectType;
                std::string objectName = std::string("Swapchain #0") + std::to_string(swapchain.uid) + "- Image #0" + std::to_string(i);
                nameInfo.pObjectName = objectName.data();
                debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
            }
        }

        // Transition new swapchainData images
        Init::TransitionSwapchainImages(device, queue, swapchain.images);

        // Command buffers are not resettable, we deallocate the old ones and allocate new ones
        device.freeCommandBuffers(swapchain.cmdPool, { static_cast<std::uint32_t>(swapchain.cmdBuffers.size()), swapchain.cmdBuffers.data() });

        vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
        cmdBufferAllocInfo.commandPool = swapchain.cmdPool;
        cmdBufferAllocInfo.commandBufferCount = (u32)swapchain.images.size();
        cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, swapchain.cmdBuffers.data());
        if (vkResult != vk::Result::eSuccess)
            throw std::runtime_error("Failed to allocate VkCommandBuffers for copy-to-swapchainData.");
        // Give names to swapchainData command buffers
        if (debugUtils != nullptr)
        {
            for (uSize i = 0; i < swapchain.cmdBuffers.size(); i++)
            {
                vk::DebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.objectHandle = (u64)(VkCommandBuffer)swapchain.cmdBuffers[i];
                nameInfo.objectType = swapchain.cmdBuffers[i].objectType;
                std::string objectName = std::string("Swapchain #0 - Copy cmd buffer #0") + std::to_string(i);
                nameInfo.pObjectName = objectName.data();
                debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
            }
        }
    }
    void RecreateGUIRenderTarget(
        DevDispatch const& device,
        DebugUtilsDispatch const* debugUtils,
        vk::Extent2D swapchainDimensions,
        vk::Queue vkQueue,
        GUIRenderTarget& renderTarget)
    {
        vk::Result vkResult{};

        renderTarget.extent = swapchainDimensions;

        device.destroyImageView(renderTarget.imgView);
        device.destroyImage(renderTarget.img);

        // Allocate the new rendertarget
        {
            vk::ImageCreateInfo imgInfo{};
            imgInfo.arrayLayers = 1;
            imgInfo.extent = vk::Extent3D{ swapchainDimensions.width, swapchainDimensions.height, 1 };
            imgInfo.flags = vk::ImageCreateFlagBits{};
            imgInfo.format = renderTarget.format;
            imgInfo.imageType = vk::ImageType::e2D;
            imgInfo.initialLayout = vk::ImageLayout::eUndefined;
            imgInfo.mipLevels = 1;
            imgInfo.samples = vk::SampleCountFlagBits::e1;
            imgInfo.sharingMode = vk::SharingMode::eExclusive;
            imgInfo.tiling = vk::ImageTiling::eOptimal;
            imgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

            renderTarget.img = device.createImage(imgInfo);
            if (debugUtils != nullptr)
            {
                vk::DebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.objectHandle = (u64)(VkImage)renderTarget.img;
                nameInfo.objectType = renderTarget.img.objectType;
                nameInfo.pObjectName = "GUI Render target";
                debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
            }

            vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(renderTarget.img);
            if (memReqs.size > renderTarget.memorySize)
            {
                vk::MemoryAllocateInfo memAllocInfo{};
                memAllocInfo.allocationSize = memReqs.size;
                memAllocInfo.memoryTypeIndex = 0;
                renderTarget.memory = device.allocateMemory(memAllocInfo);
                if (debugUtils != nullptr)
                {
                    vk::DebugUtilsObjectNameInfoEXT nameInfo{};
                    nameInfo.objectHandle = (u64)(VkDeviceMemory)renderTarget.memory;
                    nameInfo.objectType = renderTarget.memory.objectType;
                    nameInfo.pObjectName = "GUI Image-memory";
                    debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
                }
            }

            device.bindImageMemory(renderTarget.img, renderTarget.memory, 0);

            vk::ImageViewCreateInfo imgViewInfo{};
            imgViewInfo.format = renderTarget.format;
            imgViewInfo.image = renderTarget.img;
            imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imgViewInfo.subresourceRange.baseArrayLayer = 0;
            imgViewInfo.subresourceRange.baseMipLevel = 0;
            imgViewInfo.subresourceRange.layerCount = 1;
            imgViewInfo.subresourceRange.levelCount = 1;
            imgViewInfo.viewType = vk::ImageViewType::e2D;

            renderTarget.imgView = device.createImageView(imgViewInfo);
            if (debugUtils != nullptr)
            {
                vk::DebugUtilsObjectNameInfoEXT nameInfo{};
                nameInfo.objectHandle = (u64)(VkImageView)renderTarget.imgView;
                nameInfo.objectType = renderTarget.imgView.objectType;
                nameInfo.pObjectName = "GUI Render target imageview";
                debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
            }
        }

        vk::FramebufferCreateInfo fbInfo{};
        fbInfo.attachmentCount = 1;
        fbInfo.width = swapchainDimensions.width;
        fbInfo.height = swapchainDimensions.height;
        fbInfo.layers = 1;
        fbInfo.pAttachments = &renderTarget.imgView;
        fbInfo.renderPass = renderTarget.renderPass;
        renderTarget.framebuffer = device.createFramebuffer(fbInfo);
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkFramebuffer)renderTarget.framebuffer;
            nameInfo.objectType = renderTarget.framebuffer.objectType;
            nameInfo.pObjectName = "GUI Framebuffer";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }


        // Transition the image
        {
            vk::CommandPoolCreateInfo cmdPoolInfo{};
            cmdPoolInfo.queueFamilyIndex = 0;
            vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);
            vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
            cmdBufferAllocInfo.commandPool = cmdPool;
            cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
            cmdBufferAllocInfo.commandBufferCount = 1;
            vk::CommandBuffer cmdBuffer{};
            vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
            DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

            vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
            cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            device.beginCommandBuffer(cmdBuffer, cmdBufferBeginInfo);

            vk::ImageMemoryBarrier imgMemoryBarrier{};
            imgMemoryBarrier.image = renderTarget.img;
            imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
            imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imgMemoryBarrier.subresourceRange.layerCount = 1;
            imgMemoryBarrier.subresourceRange.levelCount = 1;
            imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
            imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits{};

            device.cmdPipelineBarrier(
                cmdBuffer,
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::DependencyFlags{},
                {},
                {},
                { imgMemoryBarrier });

            device.endCommandBuffer(cmdBuffer);

            vk::FenceCreateInfo fenceInfo{};
            vk::Fence tempFence = device.createFence(fenceInfo);

            vk::SubmitInfo submitInfo{};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffer;
            device.queueSubmit(vkQueue, { submitInfo }, tempFence);

            vkResult = device.waitForFences({ tempFence }, true, std::numeric_limits<uint64_t>::max());
            DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

            device.destroyFence(tempFence, nullptr);
            device.destroyCommandPool(cmdPool, nullptr);
        }
    }
}

void DEngine::Gfx::Vk::Draw(Data& gfxData, void* apiDataBuffer, float scale)
{
    vk::Result vkResult{};
    APIData& apiData = *(reinterpret_cast<APIData*>(apiDataBuffer));
    
    std::uint_least8_t currentResourceSet = apiData.currentResourceSet;
    apiData.currentResourceSet = (apiData.currentResourceSet + 1) % apiData.resourceSetCount;

    FencedCmdBuffer guiCmdBuffer = apiData.guiRenderCmdBuffers.cmdBuffers[currentResourceSet];

    vkResult = apiData.device.waitForFences({ guiCmdBuffer.fence }, true, std::numeric_limits<u64>::max());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Vulkan: Unable to wait for GUI rendering cmd buffer.");
    apiData.device.resetFences({ guiCmdBuffer.fence });
    // Begin recording GUI render cmd buffer
    {
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        apiData.device.beginCommandBuffer(guiCmdBuffer.handle, beginInfo);

        vk::RenderPassBeginInfo rpBegin{};
        rpBegin.framebuffer = apiData.guiRenderTarget.framebuffer;
        rpBegin.renderPass = apiData.guiRenderTarget.renderPass;
        rpBegin.renderArea.extent = apiData.guiRenderTarget.extent;
        rpBegin.clearValueCount = 1;
        vk::ClearColorValue clearVal{};
        clearVal.setFloat32({1.f, 0.f, 0.f, 1.f});
        vk::ClearValue test = clearVal;
        rpBegin.pClearValues = &test;

        apiData.device.cmdBeginRenderPass(guiCmdBuffer.handle, rpBegin, vk::SubpassContents::eInline);
        {
            vk::Viewport viewport{};
            viewport.width = apiData.guiRenderTarget.extent.width;
            viewport.height = apiData.guiRenderTarget.extent.height;
            apiData.device.cmdSetViewport(guiCmdBuffer.handle, 0, { viewport });
            apiData.device.cmdBindPipeline(guiCmdBuffer.handle, vk::PipelineBindPoint::eGraphics, apiData.testPipeline);
            apiData.device.cmdDraw(guiCmdBuffer.handle, 3, 1, 0, 0);
        }
        apiData.device.cmdEndRenderPass(guiCmdBuffer.handle);

        apiData.device.endCommandBuffer(guiCmdBuffer.handle);
    }
    // Submit gui rendering command buffer
    {
        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &guiCmdBuffer.handle;
        apiData.device.queueSubmit(apiData.renderQueue, { submitInfo }, guiCmdBuffer.fence);
    }
    

    vk::ResultValue<std::uint32_t> imageIndexOpt = apiData.device.acquireNextImageKHR(
        apiData.swapchain.handle, 
        std::numeric_limits<u64>::max(), 
        apiData.swapchain.imageAvailableSemaphore, 
        vk::Fence());
    // Submit the copy-to-swapchainData operation and present image.
    if (imageIndexOpt.result == vk::Result::eSuccess)
    {
        u32 const nextSwapchainImgIndex = imageIndexOpt.value;
        // We don't need semaphore (I think) between this cmd buffer and the rendering one, it's handled by barriers
        vk::SubmitInfo copyImageSubmitInfo{};
        copyImageSubmitInfo.commandBufferCount = 1;
        copyImageSubmitInfo.pCommandBuffers = &apiData.swapchain.cmdBuffers[nextSwapchainImgIndex];
        vk::PipelineStageFlags const imgCopyDoneStage = vk::PipelineStageFlagBits::eTransfer;
        copyImageSubmitInfo.waitSemaphoreCount = 1;
        copyImageSubmitInfo.pWaitSemaphores = &apiData.swapchain.imageAvailableSemaphore;
        copyImageSubmitInfo.pWaitDstStageMask = &imgCopyDoneStage;
        apiData.device.queueSubmit(apiData.renderQueue, { copyImageSubmitInfo }, vk::Fence());

        vk::PresentInfoKHR presentInfo{};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &apiData.swapchain.handle;
        presentInfo.pImageIndices = &nextSwapchainImgIndex;
        vkResult = apiData.device.queuePresentKHR(apiData.renderQueue, presentInfo);
        if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eSuboptimalKHR)
            throw std::runtime_error("Vulkan: Queue presentation submit failed.");
    }
    else if (imageIndexOpt.result == vk::Result::eSuboptimalKHR)
    {
        // We need to signal the semaphore we sent into the vkAcquireNextImageKHR
        vk::SubmitInfo copyImageSubmitInfo{};
        vk::PipelineStageFlags const imgCopyDoneStage = vk::PipelineStageFlagBits::eTopOfPipe;
        copyImageSubmitInfo.waitSemaphoreCount = 1;
        copyImageSubmitInfo.pWaitSemaphores = &apiData.swapchain.imageAvailableSemaphore;
        copyImageSubmitInfo.pWaitDstStageMask = &imgCopyDoneStage;
        apiData.device.queueSubmit(apiData.renderQueue, { copyImageSubmitInfo }, vk::Fence());
        apiData.device.waitIdle();
        RebuildSwapchain(
            apiData.instance,
            apiData.device,
            apiData.DebugUtilsPtr(),
            apiData.renderQueue,
            apiData.physDeviceInfo.handle,
            apiData.surface,
            apiData.swapchain);

        RecreateGUIRenderTarget(
            apiData.device,
            apiData.DebugUtilsPtr(),
            apiData.swapchain.extents,
            apiData.renderQueue,
            apiData.guiRenderTarget);

        Init::RecordSwapchainCmdBuffers(apiData.device, apiData.swapchain, apiData.guiRenderTarget.img);
    }
    else
        throw std::runtime_error("Vulkan: vkAcquireNextImageKHR failed.");
    
}

bool DEngine::Gfx::Vk::InitializeBackend(Data& gfxData, InitInfo const& initInfo, void*& apiDataBuffer)
{
    apiDataBuffer = new APIData;
    APIData& apiData = *static_cast<APIData*>(apiDataBuffer);

    vk::Result result{};

    PFN_vkGetInstanceProcAddr instanceProcAddr = Vk::loadInstanceProcAddressPFN();

    BaseDispatch baseDispatch = BaseDispatch::Build(instanceProcAddr);

    auto createVkInstanceResult = Init::CreateVkInstance(initInfo.requiredVkInstanceExtensions, true, baseDispatch);
    apiData.instance.handle = createVkInstanceResult.instanceHandle;

    if (createVkInstanceResult.debugUtilsEnabled)
    {
        apiData.debugUtils = DebugUtilsDispatch::Build(apiData.instance.handle, instanceProcAddr);
        apiData.debugMessenger = Init::CreateLayerMessenger(apiData.instance.handle, apiData.DebugUtilsPtr(), initInfo.iLog);
    }

    // TODO: I don't think I like this code
    // Create the VkSurface using the callback;
    vk::SurfaceKHR surface{};
    bool surfaceCreateResult = initInfo.createVkSurfacePFN((u64)(VkInstance)apiData.instance.handle, initInfo.createVkSurfaceUserData, reinterpret_cast<u64*>(&surface));
    if (surfaceCreateResult == false || surface == vk::SurfaceKHR{})
        throw std::runtime_error("Unable to create VkSurfaceKHR object during initialization.");

    apiData.instance = InstanceDispatch::Build(apiData.instance.handle, instanceProcAddr);

    apiData.physDeviceInfo = Init::LoadPhysDevice(apiData.instance, surface);

    apiData.surface = BuildSurfaceInfo(apiData.instance, apiData.physDeviceInfo.handle, surface);

    SwapchainSettings swapchainSettings = BuildSwapchainSettings(apiData.instance, apiData.physDeviceInfo.handle, apiData.surface);

    PFN_vkGetDeviceProcAddr deviceProcAddr = (PFN_vkGetDeviceProcAddr)instanceProcAddr((VkInstance)apiData.instance.handle, "vkGetDeviceProcAddr");

    vk::Device deviceHandle = Init::CreateDevice(apiData.instance, apiData.physDeviceInfo);
    apiData.device.copy(DeviceDispatch::Build(deviceHandle, deviceProcAddr));

    apiData.renderQueue = apiData.device.getQueue(0, 0);

    apiData.swapchain = Init::CreateSwapchain(apiData.device, apiData.renderQueue, swapchainSettings, apiData.DebugUtilsPtr());

    apiData.guiRenderTarget = Init::CreateGUIRenderTarget(
        apiData.device, 
        apiData.renderQueue, 
        apiData.swapchain.extents, 
        apiData.swapchain.surfaceFormat.format, 
        apiData.DebugUtilsPtr());

    Init::RecordSwapchainCmdBuffers(apiData.device, apiData.swapchain, apiData.guiRenderTarget.img);

    apiData.resourceSetCount = 2;
    apiData.currentResourceSet = 0;
    apiData.guiRenderCmdBuffers = Init::MakeImguiRenderCmdBuffers(apiData.device, apiData.resourceSetCount, apiData.DebugUtilsPtr());

    Init::Test(apiData);

    return true;
}

DEngine::Gfx::Vk::Init::CreateVkInstanceResult DEngine::Gfx::Vk::Init::CreateVkInstance(
    Cont::Span<char const*> const requiredExtensions,
    bool const enableLayers,
    BaseDispatch const& baseDispatch)
{
    vk::Result vkResult{};
    CreateVkInstanceResult returnValue{};

    // Build what extensions we are going to use
    Cont::FixedVector<const char*, Constants::maxRequiredInstanceExtensions> totalRequiredExtensions;
    if (requiredExtensions.size() > totalRequiredExtensions.maxSize())
        throw std::out_of_range("Application requires more instance extensions than is maximally allocated during Vulkan instance creation. Increase it in the backend.");
    // First copy all required instance extensions
    for (uSize i = 0; i < requiredExtensions.size(); i++)
        totalRequiredExtensions.pushBack(requiredExtensions[i]);

    // Next add extensions required by renderer, don't add duplicates
    for (const char* requiredExtension : Constants::requiredInstanceExtensions)
    {
        bool extensionAlreadyPresent = false;
        for (const char* existingExtension : totalRequiredExtensions)
        {
            if (std::strcmp(requiredExtension, existingExtension) == 0)
            {
                extensionAlreadyPresent = true;
                break;
            }
        }
        if (extensionAlreadyPresent == false)
        {
            if (totalRequiredExtensions.canPushBack() == false)
                throw std::out_of_range("Could not fit all required instance extensions in allocated memory during Vulkan instance creation. Increase it in the backend.");
            totalRequiredExtensions.pushBack(requiredExtension);
        }
    }

    {
        // Check if all the required extensions are also available
        u32 instanceExtensionCount = 0;
        vkResult = baseDispatch.enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
        if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
            throw std::runtime_error("Vulkan: Unable to enumerate available instance extension properties.");
        std::vector<vk::ExtensionProperties> availableExtensions(instanceExtensionCount);
        baseDispatch.enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableExtensions.data());
        for (const char* required : totalRequiredExtensions)
        {
            bool requiredExtensionIsAvailable = false;
            for (const auto& available : availableExtensions)
            {
                if (std::strcmp(required, available.extensionName) == 0)
                {
                    requiredExtensionIsAvailable = true;
                    break;
                }
            }
            if (requiredExtensionIsAvailable == false)
                throw std::runtime_error("Required Vulkan instance extension is not available.");
        }
    }

    // Add Khronos validation layer if both it and debug_utils is available
    Cont::FixedVector<const char*, 5> layersToUse{};
    if constexpr (Constants::enableDebugUtils)
    {
        if (enableLayers)
        {
            // First check if the Khronos validation layer is present
            u32 availableLayerCount = 0;
            vkResult = baseDispatch.enumerateInstanceLayerProperties(&availableLayerCount, nullptr);
            if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
                throw std::runtime_error("Failed to enumerate instance layer properties during Vulkan instance creation.");
            Cont::FixedVector<vk::LayerProperties, Constants::maxAvailableInstanceLayers> availableLayers;
            if (availableLayerCount > availableLayers.maxSize())
                throw std::runtime_error("Could not fit available layers in allocated memory during Vulkan instance creation.");
            availableLayers.resize(availableLayerCount);
            baseDispatch.enumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

            bool layerIsAvailable = false;
            for (const auto& availableLayer : availableLayers)
            {
                char const* wantedLayerName = Constants::preferredValidationLayer.data();
                char const* availableLayerName = availableLayer.layerName;
                if (std::strcmp(wantedLayerName, availableLayerName) == 0)
                {
                    layerIsAvailable = true;
                    break;
                }
            }
            if (layerIsAvailable == true)
            {
                // If the layer is available, we check if it implements VK_EXT_debug_utils
                u32 layerExtensionCount = 0;
                vkResult = baseDispatch.enumerateInstanceExtensionProperties(Constants::preferredValidationLayer.data(), &layerExtensionCount, nullptr);
                if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
                    throw std::runtime_error("Vulkan: Unable to enumerate available instance extension properties.");
                Cont::FixedVector<vk::ExtensionProperties, Constants::maxAvailableLayerExtensions> availableExtensions;
                if (layerExtensionCount > availableExtensions.maxSize())
                    throw std::runtime_error("Vulkan: Unable to load layer's extensions into allocated memory.");
                availableExtensions.resize(layerExtensionCount);
                baseDispatch.enumerateInstanceExtensionProperties(Constants::preferredValidationLayer.data(), &layerExtensionCount, availableExtensions.data());

                bool debugUtilsIsAvailable = false;
                for (auto const& availableLayerExtension : availableExtensions)
                {
                    if (std::strcmp(availableLayerExtension.extensionName, Constants::debugUtilsExtensionName.data()) == 0)
                    {
                        debugUtilsIsAvailable = true;
                        break;
                    }
                }
                
                if (debugUtilsIsAvailable)
                {
                    // Debug utils and the Khronos validation layer is available.
                    // Push them onto the vectors
                    if (!totalRequiredExtensions.canPushBack())
                        throw std::runtime_error("Vulkan: Unable to push debug-utils on extensions to use, fixed-vector too small.");
                    totalRequiredExtensions.pushBack(Constants::debugUtilsExtensionName.data());
                    if (!layersToUse.canPushBack())
                        throw std::runtime_error("Vulkan: Unable to push Khronos validation layer onto layers to use, fixed-vector too small.");
                    layersToUse.pushBack(Constants::preferredValidationLayer.data());

                    returnValue.debugUtilsEnabled = true;
                }
            }
        }
    }

    vk::InstanceCreateInfo instanceInfo{};
    instanceInfo.enabledExtensionCount = (u32)totalRequiredExtensions.size();
    instanceInfo.ppEnabledExtensionNames = totalRequiredExtensions.data();
    instanceInfo.enabledLayerCount = (u32)layersToUse.size();
    instanceInfo.ppEnabledLayerNames = layersToUse.data();

    vk::ApplicationInfo appInfo{};
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pApplicationName = "DidgyImguiVulkanTest";
    appInfo.pEngineName = "Nothing special";
    instanceInfo.pApplicationInfo = &appInfo;

    vk::Instance instance = baseDispatch.createInstance(instanceInfo);

    returnValue.instanceHandle = instance;

    return returnValue;
}

vk::DebugUtilsMessengerEXT DEngine::Gfx::Vk::Init::CreateLayerMessenger(
    vk::Instance instanceHandle, 
    DebugUtilsDispatch const* debugUtilsOpt, 
    const void* userData)
{
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
    debugMessengerInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        //vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
    debugMessengerInfo.messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    debugMessengerInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)VulkanDebugCallback;
    debugMessengerInfo.pUserData = const_cast<void*>(userData);

    VkDebugUtilsMessengerCreateInfoEXT* infoPtr = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerInfo;

    return debugUtilsOpt->createDebugUtilsMessengerEXT(instanceHandle, debugMessengerInfo);
}

DEngine::Gfx::Vk::PhysDeviceInfo DEngine::Gfx::Vk::Init::LoadPhysDevice(
    InstanceDispatch const& instance,
    vk::SurfaceKHR surface)
{
    PhysDeviceInfo physDeviceInfo{};
    vk::Result vkResult{};

    u32 physicalDeviceCount = 0;
    vkResult = instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr);
    if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
        throw std::runtime_error("Vulkan: Unable to enumerate physical devices.");
    if (physicalDeviceCount == 0)
        throw std::runtime_error("Vulkan: Host machine has no Vulkan-capable devices.");
    Cont::FixedVector<vk::PhysicalDevice, Constants::maxAvailablePhysicalDevices> physDevices;
    if (physicalDeviceCount > physDevices.maxSize())
        throw std::out_of_range("Vulkan: Could not fit all Vulkan physical physDevice handles in allocated memory during Vulkan physDevice creation.");
    physDevices.resize(physicalDeviceCount);
    instance.enumeratePhysicalDevices(&physicalDeviceCount, physDevices.data());

    // For now we just select the first physDevice we find.
    physDeviceInfo.handle = physDevices[0];

    // Find preferred queues
    u32 queueFamilyPropertyCount = 0;
    instance.getPhysicalDeviceQueueFamilyProperties(physDeviceInfo.handle, &queueFamilyPropertyCount, nullptr);
    Cont::FixedVector<vk::QueueFamilyProperties, Constants::maxAvailableQueueFamilies> availableQueueFamilies;
    if (queueFamilyPropertyCount > availableQueueFamilies.maxSize())
        throw std::runtime_error("Failed to fit all VkQueueFamilyProperties inside allocated memory during Vulkan initialization.");
    availableQueueFamilies.resize(queueFamilyPropertyCount);
    instance.getPhysicalDeviceQueueFamilyProperties(physDeviceInfo.handle , &queueFamilyPropertyCount, availableQueueFamilies.data());

    // Find graphics queue
    for (u32 i = 0; i < queueFamilyPropertyCount; i++)
    {
        const auto& queueFamily = availableQueueFamilies[i];
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            physDeviceInfo.preferredQueues.graphics.familyIndex = i;
            physDeviceInfo.preferredQueues.graphics.queueIndex = 0;
            break;
        }
    }
    DENGINE_GFX_ASSERT(isValidIndex(physDeviceInfo.preferredQueues.graphics.familyIndex));
    // Find transfer queue, prefer a queue on a different family than graphics. 
    for (u32 i = 0; i < queueFamilyPropertyCount; i++)
    {
        if (i == physDeviceInfo.preferredQueues.graphics.familyIndex)
            continue;

        const auto& queueFamily = availableQueueFamilies[i];
        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
        {
            physDeviceInfo.preferredQueues.transfer.familyIndex = i;
            physDeviceInfo.preferredQueues.transfer.queueIndex = 0;
            break;
        }
    }

    // Check presentation support
    bool presentSupport = instance.getPhysicalDeviceSurfaceSupportKHR(physDeviceInfo.handle, physDeviceInfo.preferredQueues.graphics.familyIndex, surface);
    if (presentSupport == false)
        throw std::runtime_error("Vulkan: No surface present support.");

    physDeviceInfo.properties = instance.getPhysicalDeviceProperties(physDeviceInfo.handle);

    physDeviceInfo.memProperties = instance.getPhysicalDeviceMemoryProperties(physDeviceInfo.handle);

    // Find physDevice-local memory
    for (u32 i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
    {
        if (physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            physDeviceInfo.memInfo.deviceLocal = i;
            break;
        }
    }
    if (isValidIndex(physDeviceInfo.memInfo.deviceLocal) == false)
        throw std::runtime_error("Unable to find any physDevice-local memory during Vulkan initialization.");

    // Find host-visible memory
    for (u32 i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
    {
        if (physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
        {
            physDeviceInfo.memInfo.hostVisible = i;
            break;
        }
    }
    if (isValidIndex(physDeviceInfo.memInfo.hostVisible) == false)
        throw std::runtime_error("Unable to find any physDevice-local memory during Vulkan initialization.");

    // Find host-visible | physDevice local memory
    for (u32 i = 0; i < physDeviceInfo.memProperties.memoryTypeCount; i++)
    {
        vk::MemoryPropertyFlags searchFlags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;
        if ((physDeviceInfo.memProperties.memoryTypes[i].propertyFlags & searchFlags) == searchFlags)
        {
            if (physDeviceInfo.memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
            {
                physDeviceInfo.memInfo.deviceLocalAndHostVisible = i;
                break;
            }
        }
    }

    return physDeviceInfo;
}

DEngine::Gfx::Vk::SurfaceInfo DEngine::Gfx::Vk::BuildSurfaceInfo(
    InstanceDispatch const& instance,
    vk::PhysicalDevice physDevice, 
    vk::SurfaceKHR surface)
{
    vk::Result vkResult{};

    SurfaceInfo returnVal{};

    returnVal.handle = surface;

    returnVal.capabilities = instance.getPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface);

    // Grab present modes
    u32 presentModeCount = 0;
    vkResult = instance.getPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, nullptr);
    if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
        throw std::runtime_error("Vulkan: Unable to query present modes for Vulkan surface.");
    DENGINE_GFX_ASSERT(presentModeCount != 0);
    if (presentModeCount > returnVal.supportedPresentModes.maxSize())
        throw std::runtime_error("Vulkan: Unable to fit all supported present modes in allocated memory.");
    returnVal.supportedPresentModes.resize(presentModeCount);
    instance.getPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentModeCount, returnVal.supportedPresentModes.data());

    // Grab surface formats
    u32 surfaceFormatCount = 0;
    vkResult = instance.getPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &surfaceFormatCount, (vk::SurfaceFormatKHR*)nullptr);
    if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
        throw std::runtime_error("Vulkan: Unable to query surface formats for Vulkan surface.");
    DENGINE_GFX_ASSERT(surfaceFormatCount != 0);
    if (surfaceFormatCount > returnVal.supportedSurfaceFormats.maxSize())
        throw std::runtime_error("Vulkan: Unable to fit all supported surface formats in allocated memory.");
    returnVal.supportedSurfaceFormats.resize(surfaceFormatCount);
    instance.getPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &surfaceFormatCount, returnVal.supportedSurfaceFormats.data());

    return returnVal;
}

DEngine::Gfx::Vk::SwapchainSettings DEngine::Gfx::Vk::BuildSwapchainSettings(
    InstanceDispatch const& instance, 
    vk::PhysicalDevice physDevice,
    SurfaceInfo const& surfaceCaps)
{
    vk::Result vkResult{};

    SwapchainSettings settings{};

    settings.surface = surfaceCaps.handle;

    settings.capabilities = surfaceCaps.capabilities;
    settings.extents = settings.capabilities.currentExtent;

    // Handle swapchainData length
    u32 swapchainLength = Constants::preferredSwapchainLength;
    // If we need to, clamp the swapchainData length.
    // Upper clamp only applies if maxImageCount != 0
    if (settings.capabilities.maxImageCount != 0 && swapchainLength > settings.capabilities.maxImageCount)
        swapchainLength = settings.capabilities.maxImageCount;
    if (swapchainLength < 2)
        throw std::runtime_error("Vulkan: Vulkan backend doesn't support swapchainData length of 1.");
    settings.numImages = swapchainLength;

    // Find supported formats, find the preferred present-mode
    // If not found, fallback to FIFO, it's guaranteed to be supported.
    vk::PresentModeKHR preferredPresentMode = Constants::preferredPresentMode;
    vk::PresentModeKHR presentModeToUse{};
    bool preferredPresentModeFound = false;
    for (size_t i = 0; i < surfaceCaps.supportedPresentModes.size(); i++)
    {
        if (surfaceCaps.supportedPresentModes[i] == preferredPresentMode)
        {
            preferredPresentModeFound = true;
            presentModeToUse = surfaceCaps.supportedPresentModes[i];
            break;
        }
    }
    // FIFO is guaranteed to exist, so we fallback to that one if we didn't find the one we wanted.
    if (preferredPresentModeFound == false)
        presentModeToUse = vk::PresentModeKHR::eFifo;
    settings.presentMode = presentModeToUse;

    // Handle formats
    vk::SurfaceFormatKHR formatToUse = vk::SurfaceFormatKHR{};
    bool foundPreferredFormat = false;
    for (const auto& preferredFormat : Constants::preferredSurfaceFormats)
    {
        for (const auto& availableFormat : surfaceCaps.supportedSurfaceFormats)
        {
            if (availableFormat == preferredFormat)
            {
                formatToUse = preferredFormat;
                foundPreferredFormat = true;
                break;
            }
        }
        if (foundPreferredFormat)
            break;
    }
    if (!foundPreferredFormat)
        throw std::runtime_error("Vulkan: Found no suitable surface format when querying VkSurfaceKHR.");
    settings.surfaceFormat = formatToUse;

    return settings;
}

vk::Device DEngine::Gfx::Vk::Init::CreateDevice(
    InstanceDispatch const& instance,
    PhysDeviceInfo const& physDevice)
{
    vk::Result vkResult{};

    // Create logical physDevice
    vk::DeviceCreateInfo createInfo{};

    // Feature configuration
    vk::PhysicalDeviceFeatures physDeviceFeatures = instance.getPhysicalDeviceFeatures(physDevice.handle);

    vk::PhysicalDeviceFeatures featuresToUse{};
    if (physDeviceFeatures.robustBufferAccess == 1)
        featuresToUse.robustBufferAccess = true;
    //if (physDeviceFeatures.sampleRateShading == 1)
        //featuresToUse.sampleRateShading = true;

    createInfo.pEnabledFeatures = &featuresToUse;

    // Queue configuration
    f32 priority[3] = { 1.f, 1.f, 1.f };
    Cont::FixedVector<vk::DeviceQueueCreateInfo, 10> queueCreateInfos{};

    vk::DeviceQueueCreateInfo tempQueueCreateInfo{};

    // Add graphics queue
    tempQueueCreateInfo.pQueuePriorities = priority;
    tempQueueCreateInfo.queueCount = 1;
    tempQueueCreateInfo.queueFamilyIndex = physDevice.preferredQueues.graphics.familyIndex;
    queueCreateInfos.pushBack(tempQueueCreateInfo);

    // Add transfer queue if there is a separate one from graphics queue
    if (physDevice.preferredQueues.dedicatedTransfer())
    {
        tempQueueCreateInfo = vk::DeviceQueueCreateInfo{};
        tempQueueCreateInfo.pQueuePriorities = priority;
        tempQueueCreateInfo.queueCount = 1;
        tempQueueCreateInfo.queueFamilyIndex = physDevice.preferredQueues.transfer.familyIndex;

        queueCreateInfos.pushBack(tempQueueCreateInfo);
    }

    createInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    u32 deviceExtensionCount = 0;
    vkResult = instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, nullptr);
    if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
        throw std::runtime_error("Vulkan: Unable to enumerate device extensions.");
    std::vector<vk::ExtensionProperties> availableExtensions(deviceExtensionCount);
    instance.enumeratePhysicalDeviceExtensionProperties(physDevice.handle, &deviceExtensionCount, availableExtensions.data());
    // Check if all required extensions are present
    for (const char* required : Constants::requiredDeviceExtensions)
    {
        bool foundExtension = false;
        for (const auto& available : availableExtensions)
        {
            if (std::strcmp(required, available.extensionName) == 0)
            {
                foundExtension = true;
                break;
            }
        }
        if (foundExtension == false)
            throw std::runtime_error("Not all required physDevice extensions were available during Vulkan initialization.");
    }

    createInfo.ppEnabledExtensionNames = Constants::requiredDeviceExtensions.data();
    createInfo.enabledExtensionCount = u32(Constants::requiredDeviceExtensions.size());

    vk::Device vkDevice = instance.createDevice(physDevice.handle, createInfo);
    return vkDevice;
}

DEngine::Gfx::Vk::SwapchainData DEngine::Gfx::Vk::Init::CreateSwapchain(
    Vk::DeviceDispatch const& device,
    vk::Queue vkQueue,
    SwapchainSettings const& settings,
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};

    SwapchainData swapchain;
    swapchain.uid = 0;
    swapchain.extents = settings.extents;
    swapchain.surfaceFormat = settings.surfaceFormat;
    swapchain.presentMode = settings.presentMode;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageExtent = settings.extents;
    swapchainCreateInfo.imageFormat = settings.surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = settings.surfaceFormat.colorSpace;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCreateInfo.presentMode = settings.presentMode;
    swapchainCreateInfo.surface = settings.surface;
    swapchainCreateInfo.preTransform = settings.capabilities.currentTransform;
    swapchainCreateInfo.clipped = 1;
    swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.minImageCount = settings.numImages;
    swapchainCreateInfo.oldSwapchain = vk::SwapchainKHR{};

    swapchain.handle = device.createSwapchainKHR(swapchainCreateInfo);
    // Make name for the swapchainData
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkSwapchainKHR)swapchain.handle;
        nameInfo.objectType = swapchain.handle.objectType;
        std::string objectName = std::string("Swapchain #0") + std::to_string(0);
        nameInfo.pObjectName = objectName.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    u32 swapchainImageCount = 0;
    vkResult = device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, (vk::Image*)nullptr);
    if (vkResult != vk::Result::eSuccess && vkResult != vk::Result::eIncomplete)
        throw std::runtime_error("Unable to grab swapchainData images from VkSwapchainKHR object.");
    DENGINE_GFX_ASSERT(swapchainImageCount != 0);
    DENGINE_GFX_ASSERT(swapchainImageCount == settings.numImages);
    if (swapchainImageCount >= swapchain.images.maxSize())
        throw std::runtime_error("Unable to fit swapchainData image handles in allocated memory.");
    swapchain.images.resize(swapchainImageCount);
    device.getSwapchainImagesKHR(swapchain.handle, &swapchainImageCount, swapchain.images.data());

    // Make names for the swapchainData images
    if (debugUtils != nullptr)
    {
        for (uSize i = 0; i < swapchain.images.size(); i++)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImage)swapchain.images[i];
            nameInfo.objectType = swapchain.images[i].objectType;
            std::string objectName = std::string("Swapchain #0 - Image #0") + std::to_string(i);
            nameInfo.pObjectName = objectName.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    // Transition new swapchainData images
    TransitionSwapchainImages(device, vkQueue, swapchain.images);

    vk::CommandPoolCreateInfo cmdPoolInfo{};
    swapchain.cmdPool = device.createCommandPool(cmdPoolInfo);
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkCommandPool)swapchain.cmdPool;
        nameInfo.objectType = swapchain.cmdPool.objectType;
        std::string objectName = std::string("Swapchain #00 - Copy image CmdPool");
        nameInfo.pObjectName = objectName.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }
    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandPool = swapchain.cmdPool;
    cmdBufferAllocInfo.commandBufferCount = (u32)swapchain.images.size();
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    swapchain.cmdBuffers.resize(swapchain.images.size());
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, swapchain.cmdBuffers.data());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Failed to allocate VkCommandBuffers for copy-to-swapchainData.");
    // Give names to swapchainData command buffers
    if (debugUtils != nullptr)
    {
        for (uSize i = 0; i < swapchain.cmdBuffers.size(); i++)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkCommandBuffer)swapchain.cmdBuffers[i];
            nameInfo.objectType = swapchain.cmdBuffers[i].objectType;
            std::string objectName = std::string("Swapchain #0 - Copy cmd buffer #0") + std::to_string(i);
            nameInfo.pObjectName = objectName.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    vk::SemaphoreCreateInfo semaphoreInfo{};
    swapchain.imageAvailableSemaphore = device.createSemaphore(semaphoreInfo);
    // Give name to the semaphore
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkSemaphore)swapchain.imageAvailableSemaphore;
        nameInfo.objectType = swapchain.imageAvailableSemaphore.objectType;
        std::string objectName = std::string("Swapchain #0 - Copy done semaphore");
        nameInfo.pObjectName = objectName.data();
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }
    
    return swapchain;
}

bool DEngine::Gfx::Vk::Init::TransitionSwapchainImages(
    DeviceDispatch const& device, 
    vk::Queue vkQueue, 
    Cont::Span<const vk::Image> images)
{
    vk::Result vkResult{};

    vk::CommandPoolCreateInfo cmdPoolInfo{};
    vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);

    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandBufferCount = 1;
    cmdBufferAllocInfo.commandPool = cmdPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    vk::CommandBuffer cmdBuffer{};
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Unable to allocate command buffer when transitioning swapchainData images.");

    // Record commandbuffer
    {
        vk::CommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        device.beginCommandBuffer(cmdBuffer, cmdBeginInfo);

        for (size_t i = 0; i < images.size(); i++)
        {
            vk::ImageMemoryBarrier imgBarrier{};
            imgBarrier.image = images[i];
            imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imgBarrier.subresourceRange.layerCount = 1;
            imgBarrier.subresourceRange.levelCount = 1;
            imgBarrier.oldLayout = vk::ImageLayout::eUndefined;
            imgBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;

            device.cmdPipelineBarrier(
                cmdBuffer,
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::DependencyFlags{},
                0, nullptr,
                0, nullptr,
                1, &imgBarrier);
        }

        device.endCommandBuffer(cmdBuffer);
    }

    vk::FenceCreateInfo fenceInfo{};
    vk::Fence fence = device.createFence(fenceInfo);

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    device.queueSubmit(vkQueue, { submitInfo }, fence);

    vkResult = device.waitForFences({ fence }, true, std::numeric_limits<uint64_t>::max());
    DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

    device.destroyFence(fence);
    device.destroyCommandPool(cmdPool);

    return true;
}

DEngine::Gfx::Vk::GUIRenderTarget DEngine::Gfx::Vk::Init::CreateGUIRenderTarget(
    DeviceDispatch const& device,
    vk::Queue vkQueue,
    vk::Extent2D swapchainDimensions,
    vk::Format swapchainFormat,
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};

    GUIRenderTarget returnVal{};
    returnVal.format = swapchainFormat;
    returnVal.extent = swapchainDimensions;

    // Create the renderpass for imgui
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
        nameInfo.pObjectName = "GUI render-pass";
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }


    // Allocate the rendertarget
    {
        vk::ImageCreateInfo imgInfo{};
        imgInfo.arrayLayers = 1;
        imgInfo.extent = vk::Extent3D{ swapchainDimensions.width, swapchainDimensions.height, 1 };
        imgInfo.flags = vk::ImageCreateFlagBits{};
        imgInfo.format = swapchainFormat;
        imgInfo.imageType = vk::ImageType::e2D;
        imgInfo.initialLayout = vk::ImageLayout::eUndefined;
        imgInfo.mipLevels = 1;
        imgInfo.samples = vk::SampleCountFlagBits::e1;
        imgInfo.sharingMode = vk::SharingMode::eExclusive;
        imgInfo.tiling = vk::ImageTiling::eOptimal;
        imgInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

        returnVal.img = device.createImage(imgInfo);
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImage)returnVal.img;
            nameInfo.objectType = returnVal.img.objectType;
            nameInfo.pObjectName = "GUI Render target";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(returnVal.img);
        vk::MemoryAllocateInfo memAllocInfo{};
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = 0;
        returnVal.memory = device.allocateMemory(memAllocInfo);
        returnVal.memorySize = memAllocInfo.allocationSize;
        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkDeviceMemory)returnVal.memory;
            nameInfo.objectType = returnVal.memory.objectType;
            nameInfo.pObjectName = "GUI Image-memory";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        device.bindImageMemory(returnVal.img, returnVal.memory, 0);

        vk::ImageViewCreateInfo imgViewInfo{};
        imgViewInfo.format = swapchainFormat;
        imgViewInfo.image = returnVal.img;
        imgViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewInfo.subresourceRange.baseArrayLayer = 0;
        imgViewInfo.subresourceRange.baseMipLevel = 0;
        imgViewInfo.subresourceRange.layerCount = 1;
        imgViewInfo.subresourceRange.levelCount = 1;
        imgViewInfo.viewType = vk::ImageViewType::e2D;

        returnVal.imgView = device.createImageView(imgViewInfo);

        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkImageView)returnVal.imgView;
            nameInfo.objectType = returnVal.imgView.objectType;
            nameInfo.pObjectName = "GUI Render target imageview";
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    vk::FramebufferCreateInfo fbInfo{};
    fbInfo.attachmentCount = 1;
    fbInfo.width = swapchainDimensions.width;
    fbInfo.height = swapchainDimensions.height;
    fbInfo.layers = 1;
    fbInfo.pAttachments = &returnVal.imgView;
    fbInfo.renderPass = returnVal.renderPass;
    returnVal.framebuffer = device.createFramebuffer(fbInfo);
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkFramebuffer)returnVal.framebuffer;
        nameInfo.objectType = returnVal.framebuffer.objectType;
        nameInfo.pObjectName = "GUI Framebuffer";
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }


    // Transition the image
    vk::CommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.queueFamilyIndex = 0;
    vk::CommandPool cmdPool = device.createCommandPool(cmdPoolInfo);
    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandPool = cmdPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdBufferAllocInfo.commandBufferCount = 1;
    vk::CommandBuffer cmdBuffer{};
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, &cmdBuffer);
    DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);

    vk::CommandBufferBeginInfo cmdBufferBeginInfo{};
    cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    device.beginCommandBuffer(cmdBuffer, cmdBufferBeginInfo);

    vk::ImageMemoryBarrier imgMemoryBarrier{};
    imgMemoryBarrier.image = returnVal.img;
    imgMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
    imgMemoryBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    imgMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgMemoryBarrier.subresourceRange.layerCount = 1;
    imgMemoryBarrier.subresourceRange.levelCount = 1;
    imgMemoryBarrier.srcAccessMask = vk::AccessFlagBits{};
    imgMemoryBarrier.dstAccessMask = vk::AccessFlagBits{};

    device.cmdPipelineBarrier(
        cmdBuffer,
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::DependencyFlags{},
        {},
        {},
        { imgMemoryBarrier });

    device.endCommandBuffer(cmdBuffer);

    vk::FenceCreateInfo fenceInfo{};
    vk::Fence tempFence = device.createFence(fenceInfo);

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    device.queueSubmit(vkQueue, { submitInfo }, tempFence);

    vkResult = device.waitForFences({ tempFence }, true, std::numeric_limits<uint64_t>::max());
    DENGINE_GFX_ASSERT(vkResult == vk::Result::eSuccess);
    
    device.destroyFence(tempFence, nullptr);
    device.destroyCommandPool(cmdPool, nullptr);

    return returnVal;
}

void DEngine::Gfx::Vk::Init::RecordSwapchainCmdBuffers(
    DeviceDispatch const& device,
    SwapchainData const& swapchainData, 
    vk::Image srcImage)
{
    vk::Result vkResult{};

    for (uSize i = 0; i < swapchainData.cmdBuffers.size(); i++)
    {
        vk::CommandBuffer cmdBuffer = swapchainData.cmdBuffers[i];
        vk::Image dstImage = swapchainData.images[i];

        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        device.beginCommandBuffer(cmdBuffer, beginInfo);

        // The renderpass handles transitioning the rendered image into transfer-src-optimal
        vk::ImageMemoryBarrier imguiPreTransferImgBarrier{};
        imguiPreTransferImgBarrier.image = srcImage;
        imguiPreTransferImgBarrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        imguiPreTransferImgBarrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        imguiPreTransferImgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imguiPreTransferImgBarrier.subresourceRange.layerCount = 1;
        imguiPreTransferImgBarrier.subresourceRange.levelCount = 1;
        imguiPreTransferImgBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        imguiPreTransferImgBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        device.cmdPipelineBarrier(
            cmdBuffer,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags{},
            {},
            {},
            { imguiPreTransferImgBarrier });

        vk::ImageMemoryBarrier preTransferSwapchainImageBarrier{};
        preTransferSwapchainImageBarrier.image = dstImage;
        preTransferSwapchainImageBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
        preTransferSwapchainImageBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        preTransferSwapchainImageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        preTransferSwapchainImageBarrier.subresourceRange.layerCount = 1;
        preTransferSwapchainImageBarrier.subresourceRange.levelCount = 1;
        preTransferSwapchainImageBarrier.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
        preTransferSwapchainImageBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        device.cmdPipelineBarrier(
            cmdBuffer,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags{},
            {},
            {},
            { preTransferSwapchainImageBarrier });

        vk::ImageCopy copyRegion{};
        copyRegion.extent = vk::Extent3D{ swapchainData.extents.width, swapchainData.extents.height, 1 };
        copyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copyRegion.dstSubresource.layerCount = 1;
        device.cmdCopyImage(
            cmdBuffer,
            srcImage,
            vk::ImageLayout::eTransferSrcOptimal,
            dstImage,
            vk::ImageLayout::eTransferDstOptimal,
            { copyRegion });


        vk::ImageMemoryBarrier postTransferSwapchainImageBarrier{};
        postTransferSwapchainImageBarrier.image = dstImage;
        postTransferSwapchainImageBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        postTransferSwapchainImageBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
        postTransferSwapchainImageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        postTransferSwapchainImageBarrier.subresourceRange.layerCount = 1;
        postTransferSwapchainImageBarrier.subresourceRange.levelCount = 1;
        postTransferSwapchainImageBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        postTransferSwapchainImageBarrier.dstAccessMask = vk::AccessFlags{};
        device.cmdPipelineBarrier(
            cmdBuffer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::DependencyFlags{},
            {},
            {},
            { postTransferSwapchainImageBarrier });

        // RenderPass handles transitioning rendered image back to color attachment output

        device.endCommandBuffer(cmdBuffer);
    }
}

DEngine::Gfx::Vk::GUIRenderCmdBuffers DEngine::Gfx::Vk::Init::MakeImguiRenderCmdBuffers(
    DeviceDispatch const& device,
    u32 resourceSetCount,
    DebugUtilsDispatch const* debugUtils)
{
    vk::Result vkResult{};

    GUIRenderCmdBuffers returnVal{};

    vk::CommandPoolCreateInfo imguiRenderCmdPoolInfo{};
    imguiRenderCmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    returnVal.cmdPool = device.createCommandPool(imguiRenderCmdPoolInfo);
    if (debugUtils != nullptr)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectHandle = (u64)(VkCommandPool)returnVal.cmdPool;
        nameInfo.objectType = returnVal.cmdPool.objectType;
        nameInfo.pObjectName = "GUI Rendering CmdPool";
        debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
    }

    Cont::FixedVector<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
    vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.commandPool = returnVal.cmdPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdBufferAllocInfo.commandBufferCount = resourceSetCount;
    cmdBuffers.resize(cmdBufferAllocInfo.commandBufferCount);
    vkResult = device.allocateCommandBuffers(&cmdBufferAllocInfo, cmdBuffers.data());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Vulkan: Unable to allocate command buffers for GUI rendering.");


    returnVal.cmdBuffers.resize(cmdBuffers.size());
    // Make fences and assign cmdBuffer handles
    for (uSize i = 0; i < returnVal.cmdBuffers.size(); i++)
    {
        auto& fencedCmdBuffer = returnVal.cmdBuffers[i];

        fencedCmdBuffer.handle = cmdBuffers[i];

        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
        fencedCmdBuffer.fence = device.createFence(fenceInfo);

        if (debugUtils != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkFence)fencedCmdBuffer.fence;
            nameInfo.objectType = fencedCmdBuffer.fence.objectType;
            std::string objectName = std::string("GUI - Render CmdBuffer #0") + std::to_string(i) + " - Fence";
            nameInfo.pObjectName = objectName.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }
    }

    return returnVal;
}

#include <fstream>

void DEngine::Gfx::Vk::Init::Test(APIData& apiData)
{
    vk::Result vkResult{};

    vk::DescriptorSetLayoutCreateInfo descrLayoutInfo{};
    vk::DescriptorSetLayout descrLayout = apiData.device.createDescriptorSetLayout(descrLayoutInfo);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descrLayout;
    apiData.testPipelineLayout = apiData.device.createPipelineLayout(pipelineLayoutInfo);

    // Create vertex shader module
    std::ifstream vertFile("data/vert.spv", std::ios::ate | std::ios::binary);
    DENGINE_GFX_ASSERT_MSG(vertFile.is_open(), "Could not open vertex shader");
    std::vector<u8> vertCode(vertFile.tellg());
    vertFile.seekg(0);
    vertFile.read(reinterpret_cast<char*>(vertCode.data()), vertCode.size());
    vertFile.close();

    vk::ShaderModuleCreateInfo vertModCreateInfo{};
    vertModCreateInfo.codeSize = vertCode.size();
    vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());
    vk::ShaderModule vertModule = apiData.device.createShaderModule(vertModCreateInfo);
    vk::PipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertStageInfo.module = vertModule;
    vertStageInfo.pName = "main";


    std::ifstream fragFile("data/frag.spv", std::ios::ate | std::ios::binary);
    DENGINE_GFX_ASSERT_MSG(fragFile.is_open(), "Could not open fragment shader");
    std::vector<u8> fragCode(fragFile.tellg());
    fragFile.seekg(0);
    fragFile.read(reinterpret_cast<char*>(fragCode.data()), fragCode.size());
    fragFile.close();
    vk::ShaderModuleCreateInfo fragModInfo{};
    fragModInfo.codeSize = fragCode.size();
    fragModInfo.pCode = reinterpret_cast<const u32*>(fragCode.data());
    vk::ShaderModule fragModule = apiData.device.createShaderModule(fragModInfo);
    vk::PipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragStageInfo.module = fragModule;
    fragStageInfo.pName = "main";

    Cont::Array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertStageInfo, fragStageInfo };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)0.f;
    viewport.height = (f32)0.f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{ 0, 0 };
    scissor.extent = vk::Extent2D{ 100000, 1000000 };
    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.lineWidth = 1.f;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    //rasterizer.rasterizerDiscardEnable = 0;

    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.depthTestEnable = 0;
    depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilInfo.stencilTestEnable = 0;
    depthStencilInfo.depthWriteEnable = 0;
    depthStencilInfo.minDepthBounds = 0.f;
    depthStencilInfo.maxDepthBounds = 1.f;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable = 0;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    vk::DynamicState temp = vk::DynamicState::eViewport;
    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.dynamicStateCount = 1;
    dynamicState.pDynamicStates = &temp;

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.layout = apiData.testPipelineLayout;
    pipelineInfo.renderPass = apiData.guiRenderTarget.renderPass;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.stageCount = (u32)shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();

    apiData.testPipeline = apiData.device.createGraphicsPipeline(vk::PipelineCache{}, pipelineInfo);

    apiData.device.destroyShaderModule(vertModule, nullptr);
    apiData.device.destroyShaderModule(fragModule, nullptr);
}