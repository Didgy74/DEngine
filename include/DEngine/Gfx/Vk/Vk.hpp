#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISABLE_ENHANCED_MODE

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Array.hpp"

#include <limits>
#include <vector>

#include "vulkan/vulkan.h"
#include "vulkan/vulkan.hpp"

#undef max

namespace DEngine::Gfx::Vk
{
	namespace Constants
	{
		constexpr u32 maxResourceSets = 3;
		constexpr u32 preferredResourceSetCount = 2;
		constexpr u32 maxSwapchainLength = 4;
		constexpr u32 preferredSwapchainLength = 3;
		constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;

		constexpr Cont::Array<const char*, 1> preferredValidationLayers
		{
			"VK_LAYER_KHRONOS_validation"
		};

		constexpr Cont::Array<const char*, 1> requiredInstanceExtensions
		{
			"VK_KHR_surface"
		};

		constexpr bool enableDebugUtils = true;
		constexpr std::string_view debugUtilsExtensionName
		{
			"VK_EXT_debug_utils"
		};

		constexpr Cont::Array<const char*, 1> requiredDeviceExtensions
		{
			"VK_KHR_swapchain"
		};

		constexpr Cont::Array<vk::Format, 2> preferredSurfaceFormats
		{
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eB8G8R8A8Unorm
		};

		constexpr u32 invalidIndex = std::numeric_limits<u32>::max();
	}

	struct QueueInfo
	{
		static constexpr u32 invalidIndex = std::numeric_limits<u32>::max();

		struct Queue
		{
			vk::Queue handle = nullptr;
			u32 familyIndex = invalidIndex;
			u32 queueIndex = invalidIndex;
		};

		Queue graphics{};
		Queue transfer{};

		inline bool TransferIsSeparate() const { return transfer.familyIndex != graphics.familyIndex; }
	};

	struct MemoryTypes
	{
		static constexpr u32 invalidIndex = std::numeric_limits<u32>::max();

		u32 deviceLocal = invalidIndex;
		u32 hostVisible = invalidIndex;
		u32 deviceLocalAndHostVisible = invalidIndex;

		inline bool UnifiedMemory() const { return deviceLocal == hostVisible && deviceLocal != invalidIndex && hostVisible != invalidIndex; }
	};

	struct SwapchainSettings
	{
		vk::SurfaceKHR surface{};
		vk::SurfaceCapabilitiesKHR capabilities{};
		vk::PresentModeKHR presentMode{};
		vk::SurfaceFormatKHR surfaceFormat{};
		vk::Extent2D extents{};
		u32 numImages{};
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

	struct Device
	{
		vk::Device handle{};
		vk::PhysicalDevice physDeviceHandle{};
	};

	struct ImGuiRenderTarget
	{
		vk::DeviceMemory memory{};
		vk::Format format{};
		vk::Image img{};
		vk::ImageView imgView{};
		vk::RenderPass renderPass{};
		vk::Framebuffer framebuffer{};
		vk::Semaphore imguiRenderFinished{};
	};

	struct SwapchainData
	{
		vk::SwapchainKHR handle{};

		vk::Extent2D extents{};
		u32 imageCount = 0;

		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

		vk::SurfaceFormatKHR surfaceFormat{};
		vk::SurfaceKHR surface{};

		u32 currentImageIndex = Constants::invalidIndex;
		Cont::Array<vk::Image, Constants::maxSwapchainLength> images{};

		vk::CommandPool cmdPool{};
		Cont::Array<vk::CommandBuffer, Constants::maxSwapchainLength> cmdBuffers{};
		vk::Semaphore imgCopyDoneSemaphore{};
	};

	struct SurfaceCapabilities
	{
		vk::SurfaceKHR surfaceHandle{};

		Cont::Array<vk::PresentModeKHR, 8> supportedPresentModes{
			// I didn't bother doing something more elegant.
			(vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max(),
			(vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max(),
			(vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max(),
			(vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max(),
			(vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max(),
			(vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max(),
			(vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max(),
			(vk::PresentModeKHR)std::numeric_limits<std::underlying_type_t<vk::PresentModeKHR>>::max(),
		};
	};

	struct ImguiRenderCmdBuffers
	{
		vk::CommandPool cmdPool{};
		// Has length of resource sets
		Cont::Array<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
		// Has length of resource sets
		Cont::Array<vk::Fence, Constants::maxResourceSets> fences{};
	};

	struct APIData
	{
		vk::Instance instance{};
		bool debugUtilsEnabled = false;
		vk::DebugUtilsMessengerEXT debugMessenger{};
		vk::SurfaceKHR surface{};

		u8 resourceSetCount = 0;
		u8 currentResourceSet = 0;

		PhysDeviceInfo physDeviceInfo{};

		vk::Device device{};

		vk::Queue tempQueue{};

		SwapchainData swapchain{};

		ImGuiRenderTarget imguiRenderTarget{};

		ImguiRenderCmdBuffers imguiRenderCmdBuffers{};

		vk::Sampler testSampler{};

		vk::PipelineLayout testPipelineLayout{};
		vk::Pipeline testPipeline{};
		Cont::Array<vk::DescriptorSet, Constants::maxResourceSets> descrSets{};
		void* bufferData{};
		vk::DeviceMemory testMem{};
		vk::Image testImg{};
		vk::ImageView testImgView{};
	};
}