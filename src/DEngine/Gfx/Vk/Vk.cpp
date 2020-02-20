#include "Vk.hpp"
#include "../VkInterface.hpp"
#include "DEngine/Gfx/Assert.hpp"
#include "Init.hpp"

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Span.hpp"
#include "DEngine/Containers/Array.hpp"
#include "DEngine/Containers/FixedVector.hpp"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_vulkan.h"

#include <string>

//vk::DispatchLoaderDynamic vk::defaultDispatchLoaderDynamic;

namespace DEngine::Gfx::Vk
{
    void windowResizeEvent(APIData& apiData)
    {
        Init::RecreateSwapchain(
            apiData.instance,
            apiData.device,
            apiData.DebugUtilsPtr(),
            apiData.renderQueue,
            apiData.physDevice.handle,
            apiData.surface,
            apiData.swapchain);

        Init::RecreateGUIRenderTarget(
            apiData.device,
            apiData.swapchain.extents,
            apiData.renderQueue,
            apiData.guiData.renderPass,
            apiData.guiData.renderTarget,
            apiData.DebugUtilsPtr());

        Init::RecordSwapchainCmdBuffers(apiData.device, apiData.swapchain, apiData.guiData.renderTarget.img);
    }

    void RecordGUICmdBuffer(
        DeviceDispatch const& device, 
        vk::CommandBuffer cmdBuffer,
        GUIRenderTarget const& renderTarget,
        vk::RenderPass renderPass,
        std::uint_least8_t resourceSetIndex,
        DebugUtilsDispatch const* debugUtils)
    {
        vk::Result vkResult{};

        // We need to name the command buffer every time we reset it
        if (debugUtils)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkCommandBuffer)cmdBuffer;
            nameInfo.objectType = cmdBuffer.objectType;
            std::string name = std::string("GUI CmdBuffer #") + std::to_string(resourceSetIndex);
            nameInfo.pObjectName = name.data();
            debugUtils->setDebugUtilsObjectNameEXT(device.handle, nameInfo);
        }

        {
            vk::CommandBufferBeginInfo beginInfo{};
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            device.beginCommandBuffer(cmdBuffer, beginInfo);

            vk::RenderPassBeginInfo rpBegin{};
            rpBegin.framebuffer = renderTarget.framebuffer;
            rpBegin.renderPass = renderPass;
            rpBegin.renderArea.extent = renderTarget.extent;
            rpBegin.clearValueCount = 1;
            vk::ClearColorValue clearVal{};
            clearVal.setFloat32({ 0.5f, 0.f, 0.f, 1.f });
            vk::ClearValue test = clearVal;
            rpBegin.pClearValues = &test;

            device.cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);
            {
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VkCommandBuffer(cmdBuffer));
            }
            device.cmdEndRenderPass(cmdBuffer);

            device.endCommandBuffer(cmdBuffer);
        }
    }

    void PresentShit(APIData& apiData)
    {
        vk::Result vkResult{};

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
            throw std::runtime_error("Vulkan: Encountered suboptimal surface.");
        else
            throw std::runtime_error("Vulkan: vkAcquireNextImageKHR failed.");
    }

    struct RecordGraphicsCmdBuffer_Params
    {
        DevDispatch const* device = nullptr;
        GfxViewportData const* gfxViewportData = nullptr;
        std::uint8_t viewportID = 255;
        std::uint8_t resourceSetIndex = 255;
        vk::RenderPass renderPass{};
        bool useEditorPipeline = false;
        vk::Pipeline testPipeline{};
        DebugUtilsDispatch const* debugUtils = nullptr;;
    };
    void RecordGraphicsCmdBuffer(RecordGraphicsCmdBuffer_Params params)
    {
        vk::CommandBuffer cmdBuffer = params.gfxViewportData->cmdBuffers[params.resourceSetIndex];

        // We need to rename the command buffer every time we record it
        if (params.debugUtils)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkCommandBuffer)cmdBuffer;
            nameInfo.objectType = cmdBuffer.objectType;
            std::string name = std::string("Graphics viewport #") + std::to_string(params.viewportID) + std::string(" - CmdBuffer #") + std::to_string(params.resourceSetIndex);
            nameInfo.pObjectName = name.data();
            params.debugUtils->setDebugUtilsObjectNameEXT(params.device->handle, nameInfo);
        }


        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        params.device->beginCommandBuffer(cmdBuffer, beginInfo);

            vk::RenderPassBeginInfo rpBegin{};
            rpBegin.framebuffer = params.gfxViewportData->renderTarget.framebuffer;
            rpBegin.renderPass = params.renderPass;
            rpBegin.renderArea.extent = params.gfxViewportData->renderTarget.extent;
            rpBegin.clearValueCount = 1;
            vk::ClearColorValue clearVal{};
            clearVal.setFloat32({ 0.f, 0.f, 0.5f, 1.f });
            vk::ClearValue test = clearVal;
            rpBegin.pClearValues = &test;
            params.device->cmdBeginRenderPass(cmdBuffer, rpBegin, vk::SubpassContents::eInline);

                vk::Viewport viewport{};
                viewport.width = static_cast<float>(params.gfxViewportData->renderTarget.extent.width);
                viewport.height = static_cast<float>(params.gfxViewportData->renderTarget.extent.height);
                params.device->cmdSetViewport(cmdBuffer, 0, { viewport });
                params.device->cmdBindPipeline(cmdBuffer, vk::PipelineBindPoint::eGraphics, params.testPipeline);
                params.device->cmdDraw(cmdBuffer, 3, 1, 0, 0);

            params.device->cmdEndRenderPass(cmdBuffer);

            // We need a barrier for the render-target if we are going to sample from it in the gui pass
            if (params.useEditorPipeline)
            {
                vk::ImageMemoryBarrier imgBarrier{};
                imgBarrier.image = params.gfxViewportData->renderTarget.img;
                imgBarrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                imgBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                imgBarrier.subresourceRange.baseArrayLayer = 0;
                imgBarrier.subresourceRange.baseMipLevel = 0;
                imgBarrier.subresourceRange.layerCount = 1;
                imgBarrier.subresourceRange.levelCount = 1;
                imgBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
                imgBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

                params.device->cmdPipelineBarrier(
                    cmdBuffer,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput,
                    vk::PipelineStageFlagBits::eFragmentShader,
                    vk::DependencyFlagBits(),
                    {},
                    {},
                    { imgBarrier });
            }

        params.device->endCommandBuffer(cmdBuffer);
    }

    void NewViewport(void* apiDataBuffer, u8& viewportID, void*& imguiTexID)
    {
        vk::Result vkResult{};
        APIData& apiData = *reinterpret_cast<APIData*>(apiDataBuffer);

        viewportID = (u8)apiData.viewportDatas.size();

        GfxViewportData viewportData{};
        viewportData.viewportID = viewportID;
        viewportData.renderTarget = Init::InitializeGfxViewport(
            apiData.device,
            viewportID,
            apiData.physDevice.memInfo.deviceLocal,
            { 250, 250 },
            apiData.gfxRenderPass,
            apiData.renderQueue,
            true,
            apiData.DebugUtilsPtr());

        imguiTexID = ImGui_ImplVulkan_AddTexture(
            (VkImageView)viewportData.renderTarget.imgView,
            (VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal);

        viewportData.imguiTextureID = imguiTexID;
        
        // Make the command pool
        vk::CommandPoolCreateInfo cmdPoolInfo{};
        cmdPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        viewportData.cmdPool = apiData.device.createCommandPool(cmdPoolInfo);
        if (apiData.DebugUtilsPtr() != nullptr)
        {
            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkCommandPool)viewportData.cmdPool;
            nameInfo.objectType = viewportData.cmdPool.objectType;
            std::string name = std::string("Graphics viewport #") + std::to_string(viewportID) + " - CmdPool";
            nameInfo.pObjectName = name.data();
            apiData.DebugUtilsPtr()->setDebugUtilsObjectNameEXT(apiData.device.handle, nameInfo);
        }

        vk::CommandBufferAllocateInfo cmdBufferAllocInfo{};
        cmdBufferAllocInfo.commandPool = viewportData.cmdPool;
        cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
        cmdBufferAllocInfo.commandBufferCount = apiData.resourceSetCount;
        viewportData.cmdBuffers.resize(cmdBufferAllocInfo.commandBufferCount);
        vkResult = apiData.device.allocateCommandBuffers(&cmdBufferAllocInfo, viewportData.cmdBuffers.data());
        if (vkResult != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan: Unable to allocate command cmdBuffers for GUI rendering.");
        

        apiData.viewportDatas.pushBack(std::move(viewportData));
    }

    namespace Init
    {
        void Test(APIData& apiData);
    }
}

void DEngine::Gfx::Vk::Draw(Data& gfxData, Draw_Params const& drawParams, void* apiDataBuffer)
{
    vk::Result vkResult{};
    APIData& apiData = *(reinterpret_cast<APIData*>(apiDataBuffer));

    if (drawParams.viewportResizeEvents.size() > 0)
    {
        vkResult = apiData.device.waitForFences({ (u32)apiData.mainFences.size(), apiData.mainFences.data() }, true, std::numeric_limits<std::uint64_t>::max());
        if (vkResult != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence.");

        for (auto& viewportResizeEvent : drawParams.viewportResizeEvents)
        {
            GfxViewportData& viewport = apiData.viewportDatas[viewportResizeEvent.viewportID];
            Init::ResizeGfxViewport(
                apiData.device,
                apiData.renderQueue,
                apiData.gfxRenderPass,
                viewport.viewportID,
                viewport.imguiTextureID,
                { viewportResizeEvent.width, viewportResizeEvent.width },
                true,
                viewport.renderTarget,
                apiData.DebugUtilsPtr());
        }
    }
    {
        // Handles events the renderer must handle.
        // TODO: Move this into a function
        if (gfxData.rebuildVkSurface)
        {
            apiData.device.waitIdle();
            apiData.device.destroySwapchainKHR(apiData.swapchain.handle);
            apiData.swapchain.handle = vk::SwapchainKHR();

            u64 tempNewSurface = 0;
            vk::Result result = (vk::Result)apiData.wsiInterface->createVkSurface((u64)(VkInstance)apiData.instance.handle, nullptr, &tempNewSurface);
            if (result != vk::Result::eSuccess)
                throw std::runtime_error("Vulkan: Couldn't recreate the VkSurfaceKHR");
            vk::SurfaceKHR newSurface = *reinterpret_cast<vk::SurfaceKHR const*>(&tempNewSurface);

            Init::RebuildSurfaceInfo(apiData.instance, apiData.physDevice, newSurface, apiData.surface);

            gfxData.resizeEvent = true;
        }
        if (gfxData.resizeEvent)
        {
            apiData.device.waitIdle();
            windowResizeEvent(apiData);
        }
    }


    // I did it like this because I don't want the increment operation at the bottom of the function.
    std::uint_least8_t currentResourceSet = apiData.currentResourceSet;
    apiData.currentResourceSet = (apiData.currentResourceSet + 1) % apiData.resourceSetCount;

    vkResult = apiData.device.waitForFences({ apiData.mainFences[currentResourceSet] }, true, std::numeric_limits<std::uint64_t>::max());
    if (vkResult != vk::Result::eSuccess)
        throw std::runtime_error("Vulkan: Failed to wait for cmd buffer fence.");
    apiData.device.resetFences({ apiData.mainFences[currentResourceSet] });

    Cont::FixedVector<vk::CommandBuffer, 10> cmdBuffersToSubmit{};

    // Build all graphics command buffers
    for (std::size_t i = 0; i < apiData.viewportDatas.size(); i += 1)
    {
        auto const& item = apiData.viewportDatas[i];

        RecordGraphicsCmdBuffer_Params params{};
        params.debugUtils = apiData.DebugUtilsPtr();
        params.device = &apiData.device;
        params.gfxViewportData = &item;
        params.renderPass = apiData.gfxRenderPass;
        params.resourceSetIndex = currentResourceSet;
        params.testPipeline = apiData.testPipeline;
        params.useEditorPipeline = true;
        params.viewportID = item.viewportID;
        RecordGraphicsCmdBuffer(params);

        cmdBuffersToSubmit.pushBack(apiData.viewportDatas[i].cmdBuffers[currentResourceSet]);
    }

    // First we re-record the GUI cmd buffer and submit it.
    RecordGUICmdBuffer(
        apiData.device,
        apiData.guiData.cmdBuffers[currentResourceSet],
        apiData.guiData.renderTarget,
        apiData.guiData.renderPass,
        currentResourceSet,
        apiData.DebugUtilsPtr());
    cmdBuffersToSubmit.pushBack(apiData.guiData.cmdBuffers[currentResourceSet]);


    // Submit the command cmdBuffers
    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = (u32)cmdBuffersToSubmit.size();
    submitInfo.pCommandBuffers = cmdBuffersToSubmit.data();
    apiData.device.queueSubmit(apiData.renderQueue, { submitInfo }, apiData.mainFences[currentResourceSet]);

    // We query the next available swapchain image, so we know which image we need to copy to
    PresentShit(apiData);
}

bool DEngine::Gfx::Vk::InitializeBackend(Data& gfxData, InitInfo const& initInfo, void*& apiDataBuffer)
{
    apiDataBuffer = new APIData;
    APIData& apiData = *static_cast<APIData*>(apiDataBuffer);

    apiData.logger = initInfo.optional_iLog;
    apiData.wsiInterface = initInfo.iWsi;

    apiData.resourceSetCount = 2;
    apiData.currentResourceSet = 0;

    vk::Result vkResult{};

    PFN_vkGetInstanceProcAddr instanceProcAddr = Vk::loadInstanceProcAddressPFN();

    BaseDispatch baseDispatch = BaseDispatch::Build(instanceProcAddr);

    Init::CreateVkInstance_Return createVkInstanceResult = Init::CreateVkInstance(initInfo.requiredVkInstanceExtensions, true, baseDispatch, apiData.logger);
    apiData.instance.handle = createVkInstanceResult.instanceHandle;
    apiData.instance = InstanceDispatch::Build(apiData.instance.handle, instanceProcAddr);

    // If we enabled debug utils, we build the debug utils dispatch table and the debug utils messenger.
    if (createVkInstanceResult.debugUtilsEnabled)
    {
        apiData.debugUtils = DebugUtilsDispatch::Build(apiData.instance.handle, instanceProcAddr);
        apiData.debugMessenger = Init::CreateLayerMessenger(apiData.instance.handle, apiData.DebugUtilsPtr(), initInfo.optional_iLog);
    }

    // TODO: I don't think I like this code
    // Create the VkSurface using the callback
    vk::SurfaceKHR surface{};
    vk::Result surfaceCreateResult = (vk::Result)apiData.wsiInterface->createVkSurface((u64)(VkInstance)apiData.instance.handle, nullptr, reinterpret_cast<u64*>(&surface));
    if (surfaceCreateResult != vk::Result::eSuccess)
        throw std::runtime_error("Unable to create VkSurfaceKHR object during initialization.");


    // Pick our phys device and load the info for it
    apiData.physDevice = Init::LoadPhysDevice(apiData.instance, surface);

    // Build the surface info now that we have our physical device.
    apiData.surface = Init::BuildSurfaceInfo(apiData.instance, apiData.physDevice.handle, surface, apiData.logger);

    // Build the settings we will use to build the swapchain.
    SwapchainSettings swapchainSettings = Init::BuildSwapchainSettings(apiData.instance, apiData.physDevice.handle, apiData.surface, apiData.logger);

    PFN_vkGetDeviceProcAddr deviceProcAddr = (PFN_vkGetDeviceProcAddr)instanceProcAddr((VkInstance)apiData.instance.handle, "vkGetDeviceProcAddr");

    // Create the device and the dispatch table for it.
    vk::Device deviceHandle = Init::CreateDevice(apiData.instance, apiData.physDevice);
    apiData.device.copy(DeviceDispatch::Build(deviceHandle, deviceProcAddr));

    // TEMP! We grab the render-queue for our device.
    // TODO: Fix so queues are tied to the device, and load all of the ones we want, including graphics, transfer, compute
    apiData.renderQueue = apiData.device.getQueue(0, 0);

    // Build our swapchain on our device
    apiData.swapchain = Init::CreateSwapchain(apiData.device, apiData.renderQueue, swapchainSettings, apiData.DebugUtilsPtr());


    // Create our main fences
    apiData.mainFences.resize(apiData.resourceSetCount);
    for (uSize i = 0; i < apiData.mainFences.size(); i += 1)
    {
        vk::FenceCreateInfo info{};
        info.flags = vk::FenceCreateFlagBits::eSignaled;
        apiData.mainFences[i] = apiData.device.createFence(info);

        if (apiData.DebugUtilsPtr())
        {
            std::string name = std::string("Main Fence #") + std::to_string(i);

            vk::DebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.objectHandle = (u64)(VkFence)apiData.mainFences[i];
            nameInfo.objectType = apiData.mainFences[i].objectType;
            nameInfo.pObjectName = name.data();

            apiData.debugUtils.setDebugUtilsObjectNameEXT(apiData.device.handle, nameInfo);
        }
    }


    // Create the resources for rendering GUI
    apiData.guiData = Init::CreateGUIData(
        apiData.device,
        apiData.swapchain.surfaceFormat.format,
        apiData.resourceSetCount,
        apiData.swapchain.extents,
        apiData.renderQueue,
        apiData.DebugUtilsPtr());

    // Record the copy-image command cmdBuffers that go from render-target to swapchain
    Init::RecordSwapchainCmdBuffers(apiData.device, apiData.swapchain, apiData.guiData.renderTarget.img);

    // Initialize ImGui stuff
    Init::InitializeImGui(apiData, apiData.device, instanceProcAddr, apiData.DebugUtilsPtr());

    // Create the main render stuff
    apiData.gfxRenderPass = Init::BuildMainGfxRenderPass(apiData.device, true, apiData.DebugUtilsPtr());

    Init::Test(apiData);

    return true;
}

#include <fstream>

#include "SDL2/SDL.h"

void DEngine::Gfx::Vk::Init::Test(APIData& apiData)
{
    vk::Result vkResult{};

    vk::DescriptorSetLayoutCreateInfo descrLayoutInfo{};
    vk::DescriptorSetLayout descrLayout = apiData.device.createDescriptorSetLayout(descrLayoutInfo);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descrLayout;
    apiData.testPipelineLayout = apiData.device.createPipelineLayout(pipelineLayoutInfo);

    SDL_RWops* vertFile = SDL_RWFromFile("data/vert.spv", "rb");
    if (vertFile == nullptr)
        throw std::runtime_error("Could not open vertex shader file");
    Sint64 vertFileLength = SDL_RWsize(vertFile);
    if (vertFileLength <= 0)
        throw std::runtime_error("Could not grab size of vertex shader file");
    // Create vertex shader module
    std::vector<u8> vertCode(vertFileLength);
    SDL_RWread(vertFile, vertCode.data(), 1, vertFileLength);
    SDL_RWclose(vertFile);

    vk::ShaderModuleCreateInfo vertModCreateInfo{};
    vertModCreateInfo.codeSize = vertCode.size();
    vertModCreateInfo.pCode = reinterpret_cast<const u32*>(vertCode.data());
    vk::ShaderModule vertModule = apiData.device.createShaderModule(vertModCreateInfo);
    vk::PipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertStageInfo.module = vertModule;
    vertStageInfo.pName = "main";

    SDL_RWops* fragFile = SDL_RWFromFile("data/frag.spv", "rb");
    if (fragFile == nullptr)
        throw std::runtime_error("Could not open fragment shader file");
    Sint64 fragFileLength = SDL_RWsize(fragFile);
    if (fragFileLength <= 0)
        throw std::runtime_error("Could not grab size of fragment shader file");
    std::vector<u8> fragCode(fragFileLength);
    SDL_RWread(fragFile, fragCode.data(), 1, fragFileLength);
    SDL_RWclose(fragFile);

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
    scissor.extent = vk::Extent2D{ 8192, 8192 };
    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.lineWidth = 1.f;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.rasterizerDiscardEnable = 0;

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
    pipelineInfo.renderPass = apiData.gfxRenderPass;
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