#include <iostream>

#include <vector>
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
//#include "imgui.h"
//#include "imgui_impl_sdl.h"
//#include "imgui_impl_vulkan.h"

#undef max

namespace Test
{
	namespace Constants
	{
		constexpr std::uint32_t maxResourceSets = 3;
		constexpr std::uint32_t preferredResourceSetCount = 2;
		constexpr std::uint32_t maxSwapchainLength = 4;
		constexpr std::uint32_t preferredSwapchainLength = 3;
		constexpr vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;

		constexpr std::array<const char*, 1> preferredValidationLayers
		{
			"VK_LAYER_KHRONOS_validation"
		};

		constexpr std::array<const char*, 1> requiredInstanceExtensions
		{
			"VK_KHR_surface"
		};

		constexpr bool enableDebugUtils = true;
		constexpr std::string_view debugUtilsExtensionName
		{
			"VK_EXT_debug_utils"
		};

		constexpr std::array<const char*, 1> requiredDeviceExtensions
		{
			"VK_KHR_swapchain"
		};

		constexpr std::array<vk::Format, 2> preferredSurfaceFormats
		{
			vk::Format::eR8G8B8A8Unorm,
			vk::Format::eB8G8R8A8Unorm
		};

		constexpr std::uint32_t invalidIndex = std::numeric_limits<std::uint32_t>::max();
	}

	struct Instance
	{
		vk::Instance handle{};
		bool debugUtilsEnabled = false;
	};

	struct QueueInfo
	{
		static constexpr std::uint32_t invalidIndex = std::numeric_limits<std::uint32_t>::max();

		struct Queue
		{
			vk::Queue handle = nullptr;
			std::uint32_t familyIndex = invalidIndex;
			std::uint32_t queueIndex = invalidIndex;
		};

		Queue graphics{};
		Queue transfer{};

		inline bool TransferIsSeparate() const { return transfer.familyIndex != graphics.familyIndex; }
	};

	struct MemoryTypes
	{
		static constexpr std::uint32_t invalidIndex = std::numeric_limits<std::uint32_t>::max();

		std::uint32_t deviceLocal = invalidIndex;
		std::uint32_t hostVisible = invalidIndex;
		std::uint32_t deviceLocalAndHostVisible = invalidIndex;

		inline bool DeviceLocalIsHostVisible() const { return deviceLocal == hostVisible && deviceLocal != invalidIndex; }
	};

	struct SwapchainSettings
	{
		vk::SurfaceKHR surface{};
		vk::SurfaceCapabilitiesKHR capabilities{};
		vk::PresentModeKHR presentMode{};
		vk::SurfaceFormatKHR surfaceFormat{};
		vk::Extent2D extents{};
		std::uint32_t numImages{};
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
		std::uint32_t imageCount = 0;

		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

		vk::SurfaceFormatKHR surfaceFormat{};
		vk::SurfaceKHR surface{};

		std::uint32_t currentImageIndex = Constants::invalidIndex;
		std::array<vk::Image, Constants::maxSwapchainLength> images{};

		vk::CommandPool cmdPool{};
		std::array<vk::CommandBuffer, Constants::maxSwapchainLength> cmdBuffers{};
		vk::Semaphore imgCopyDoneSemaphore{};
	};

	struct SurfaceCapabilities
	{
		vk::SurfaceKHR surfaceHandle{};

		std::array<vk::PresentModeKHR, 8> supportedPresentModes{
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
		std::array<vk::CommandBuffer, Constants::maxResourceSets> cmdBuffers{};
		// Has length of resource sets
		std::array<vk::Fence, Constants::maxResourceSets> fences{};
	};

	template<typename T>
	class Span
	{
	private:
		T* data = nullptr;
		std::size_t size = 0;

	public:
		inline Span() {}

		inline Span(T* data, std::size_t size) :
			data(data),
			size(size)
		{

		}

		operator Span<const T>() const { return Span<const T>(data, size); }

		inline T* Data() const { return data; }

		inline std::size_t Size() const { return size; }

		inline T& operator[](size_t i) const
		{
			assert(i < size);
			return *(data + i);
		}

		inline T* begin() { return data; }

		inline const T* begin() const { return data; }

		inline T* end() { return data + size; }

		inline const T* end() const { return data + size; }
	};

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void AssertVkResult(VkResult result);

	Instance CreateVkInstance(Span<const char*> requiredInstanceExtensions);

	vk::DebugUtilsMessengerEXT CreateVkDebugUtilsMessenger(vk::Instance instanceHandle);

	PhysDeviceInfo LoadPhysDevice(
		vk::Instance instance,
		vk::SurfaceKHR surface);

	vk::Device CreateDevice(const PhysDeviceInfo& physDevice);

	// Get settings for building swapchain
	SwapchainSettings GetSwapchainSettings(vk::PhysicalDevice device, vk::SurfaceKHR surface, SurfaceCapabilities* surfaceCaps);

	bool TransitionSwapchainImages(vk::Device vkDevice, vk::Queue vkQueue, Span<const vk::Image> images);

	void RecordCopyRenderedImgCmdBuffers(const SwapchainData& swapchainData, vk::Image srcImg);

	bool CreateSwapchain2(SwapchainData& swapchainData);

	SwapchainData CreateSwapchain(vk::Device vkDevice, vk::Queue vkQueue, const SwapchainSettings& settings);

	bool RecreateSwapchain(SwapchainData& swapchainData, vk::Device vkDevice, vk::Queue vkQueue, SwapchainSettings settings);

	ImGuiRenderTarget CreateImGuiRenderTarget(
		vk::Device vkDevice,
		vk::Queue vkQueue,
		vk::Extent2D swapchainDimensions,
		vk::Format swapchainFormat);

	void RecreateImGuiRenderTarget(ImGuiRenderTarget& data, vk::Device vkDevice, vk::Queue vkQueue, vk::Extent2D newExtents);

	vk::DescriptorPool CreateDescrPoolForImgui(vk::Device vkDevice);

	void InitImguiStuff(
		SDL_Window* sdlWindow,
		vk::Instance vkInstance,
		vk::PhysicalDevice physDevice,
		vk::Device vkDevice,
		vk::Queue vkQueue,
		vk::RenderPass rPass);

	ImguiRenderCmdBuffers MakeImguiRenderCmdBuffers(vk::Device vkDevice, uint32_t resourceSetCount);
}