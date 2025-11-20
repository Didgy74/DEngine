#pragma once

#include "VulkanIncluder.hpp"
#include "DynamicDispatchRaw.hpp"
#include "Utilities.hpp"
#include "VMAIncluder.hpp"

import DEngine.FixedWidthTypes;

namespace DEngine::Gfx::Vk
{
	class DeviceDispatch;

	class BoxVmaBuffer
	{
	public:
		constexpr BoxVmaBuffer() noexcept = default;

		BoxVmaBuffer(BoxVmaBuffer const&) = delete;
		BoxVmaBuffer& operator=(BoxVmaBuffer const&) = delete;

		BoxVmaBuffer(BoxVmaBuffer&& other) noexcept:
			m_vma{ other.m_vma },
			m_handle{ other.m_handle },
			m_alloc{ other.m_alloc }
		{
			other.Nullify();
		}

		BoxVmaBuffer& operator=(BoxVmaBuffer&& other) noexcept {
			if (this == &other)
				return *this;
			Destroy();
			m_vma = other.m_vma;
			m_handle = other.m_handle;
			m_alloc = other.m_alloc;
			other.Nullify();
			return *this;
		}

		[[nodiscard]] vk::Buffer Handle() const noexcept { return m_handle; }
		[[nodiscard]] VmaAllocation Alloc() const noexcept { return m_alloc;}

		[[nodiscard]] static BoxVmaBuffer Adopt(
			VmaAllocator vma,
			vk::Buffer handle,
			VmaAllocation alloc)
		{
			DENGINE_IMPL_GFX_ASSERT(vma != nullptr);
			DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(handle));
			BoxVmaBuffer returnVal = {};
			returnVal.m_vma = vma;
			returnVal.m_handle = handle;
			returnVal.m_alloc = alloc;
			return returnVal;
		}

		struct Release_ReturnT {
			vk::Buffer handle = {};
			VmaAllocation alloc = {};
			VmaAllocator vma = {};
		};

		[[nodiscard]] Release_ReturnT Release() noexcept {
			DENGINE_IMPL_GFX_ASSERT(!IsNull());
			Release_ReturnT returnValue;
			returnValue.handle = m_handle;
			returnValue.alloc = m_alloc;
			returnValue.vma = m_vma;
			Nullify();
			return returnValue;
		}

		[[nodiscard]] bool IsNull() const noexcept { return m_handle == vk::Buffer{}; }

		void Flush(u64 offset, u64 size) {
			DENGINE_IMPL_GFX_ASSERT(!IsNull());
			auto result = (vk::Result)vmaFlushAllocation(m_vma, m_alloc, offset, size);
			if (result != vk::Result::eSuccess)
				throw std::runtime_error("Unable to flush VMA allocation");
		}

		void Destroy() noexcept {
			if (!IsNull())
				vmaDestroyBuffer(m_vma, (VkBuffer)m_handle, m_alloc);
			Nullify();
		}

		~BoxVmaBuffer() noexcept {
			Destroy();
		}

	protected:
		void Nullify() {
			m_vma = {};
			m_handle = vk::Buffer{};
			m_alloc = {};
		}

		VmaAllocator m_vma = {};
		vk::Buffer m_handle = {};
		VmaAllocation m_alloc = {};
	};

	class BoxVmaImg
	{
	public:
		constexpr BoxVmaImg() noexcept = default;

		BoxVmaImg(BoxVmaImg const&) = delete;
		BoxVmaImg& operator=(BoxVmaImg const&) = delete;

		BoxVmaImg(BoxVmaImg&& other) noexcept:
			m_vma{ other.m_vma },
			m_handle{ other.m_handle },
			m_alloc{ other.m_alloc }
		{
			other.Nullify();
		}

		BoxVmaImg& operator=(BoxVmaImg&& other) noexcept {
			if (this == &other)
				return *this;
			Destroy();
			m_vma = other.m_vma;
			m_handle = other.m_handle;
			m_alloc = other.m_alloc;
			other.Nullify();
			return *this;
		}

		[[nodiscard]] vk::Image Handle() const noexcept { return m_handle; }
		[[nodiscard]] VmaAllocation Alloc() const noexcept { return m_alloc;}

		[[nodiscard]] static BoxVmaImg Adopt(
			VmaAllocator vma,
			vk::Image handle,
			VmaAllocation alloc)
		{
			DENGINE_IMPL_GFX_ASSERT(vma != nullptr);
			DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(handle));
			BoxVmaImg returnVal = {};
			returnVal.m_vma = vma;
			returnVal.m_handle = handle;
			returnVal.m_alloc = alloc;
			return returnVal;
		}

		struct Release_ReturnT {
			vk::Image handle = {};
			VmaAllocation alloc = {};
			VmaAllocator vma = {};
		};

		[[nodiscard]] Release_ReturnT Release() noexcept {
			DENGINE_IMPL_GFX_ASSERT(!IsNull());
			Release_ReturnT returnValue;
			returnValue.handle = m_handle;
			returnValue.alloc = m_alloc;
			returnValue.vma = m_vma;
			Nullify();
			return returnValue;
		}

		[[nodiscard]] bool IsNull() const noexcept { return m_handle == vk::Image{}; }

		void Destroy() noexcept {
			if (!IsNull())
				vmaDestroyImage(m_vma, (VkImage)m_handle, m_alloc);
			Nullify();
		}

		~BoxVmaImg() noexcept {
			Destroy();
		}

	protected:
		void Nullify() {
			m_vma = {};
			m_handle = vk::Image{};
			m_alloc = {};
		}

		VmaAllocator m_vma = {};
		vk::Image m_handle = {};
		VmaAllocation m_alloc = {};
	};

	namespace impl {
		[[nodiscard]] vk::Device BoxVkHandle_DeviceHandle(DeviceDispatch const&);
		[[nodiscard]] DeviceDispatchRaw const& BoxVkHandle_DeviceDispatchTable(DeviceDispatch const&);
	}

	// Do not use directly.
	template<class T>
	class BoxVkHandle {
	public:
		using HandleType = T;
		using CType = T::CType;

		BoxVkHandle() = default;

		BoxVkHandle(BoxVkHandle&& other) noexcept {
			m_device = other.m_device;
			m_handle = other.m_handle;
			m_vkDestroyFn = other.m_vkDestroyFn;
			other.Nullify();
		}

		BoxVkHandle& operator=(BoxVkHandle&& other) noexcept {
			if (this == &other)
				return *this;
			Destroy();
			m_device = other.m_device;
			m_handle = other.m_handle;
			m_vkDestroyFn = other.m_vkDestroyFn;
			other.Nullify();
			return *this;
		}

		BoxVkHandle(BoxVkHandle const&) = delete;
		BoxVkHandle& operator=(BoxVkHandle const&) = delete;

		[[nodiscard]] T Handle() const noexcept { return m_handle; }

		[[nodiscard]] T Release() noexcept {
			auto returnVal = m_handle;
			Nullify();
			return returnVal;
		}

		[[nodiscard]] bool IsNull() const noexcept { return m_handle == T{}; }

		void Destroy() noexcept {
			if (m_handle == T{})
				return;

			using DestroyFnT = decltype(std::declval<const DeviceDispatchRaw>().GetDestroyFn<T>());
			auto destroyFn = reinterpret_cast<DestroyFnT>(m_vkDestroyFn);
			destroyFn(static_cast<VkDevice>(m_device), static_cast<CType>(m_handle), nullptr);

			Nullify();
		}

		~BoxVkHandle() noexcept { Destroy(); }

		[[nodiscard]] static BoxVkHandle Adopt(DeviceDispatch const& device, T in) { return BoxVkHandle(device, in); }

	protected:
		void Nullify() {
			m_device = vk::Device{};
			m_handle = T{};
			m_vkDestroyFn = nullptr;
		}

		explicit BoxVkHandle(DeviceDispatch const& device, T handle) {
			DENGINE_IMPL_GFX_ASSERT(Util::CheckNotNull(handle));
			m_device = impl::BoxVkHandle_DeviceHandle(device);
			m_handle = handle;
			m_vkDestroyFn = (void*)impl::BoxVkHandle_DeviceDispatchTable(device).GetDestroyFn<T>();
		}

		vk::Device m_device = {};
		T m_handle = {};
		// Needs to be reinterpret_casted to PFN_vkDestroy type.
		void* m_vkDestroyFn = {};
	};

	using BoxVkCommandPool = BoxVkHandle<vk::CommandPool>;
	using BoxVkDescriptorSetLayout = BoxVkHandle<vk::DescriptorSetLayout>;
	using BoxVkDescriptorPool = BoxVkHandle<vk::DescriptorPool>;
	using BoxVkFence = BoxVkHandle<vk::Fence>;
	using BoxVkFramebuffer = BoxVkHandle<vk::Framebuffer>;
	using BoxVkImageView = BoxVkHandle<vk::ImageView>;
	using BoxVkPipeline = BoxVkHandle<vk::Pipeline>;
	using BoxVkPipelineLayout = BoxVkHandle<vk::PipelineLayout>;
	using BoxVkSampler = BoxVkHandle<vk::Sampler>;
	using BoxVkSemaphore = BoxVkHandle<vk::Semaphore>;
	using BoxVkShaderModule = BoxVkHandle<vk::ShaderModule>;
}