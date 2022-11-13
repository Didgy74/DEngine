#pragma once

#include "VulkanIncluder.hpp"
#include "VMAIncluder.hpp"

namespace DEngine::Gfx::Vk
{
	class BoxVkBuffer
	{
	public:
		constexpr BoxVkBuffer() noexcept = default;

		BoxVkBuffer(BoxVkBuffer const&) = delete;
		BoxVkBuffer& operator=(BoxVkBuffer const&) = delete;

		BoxVkBuffer(BoxVkBuffer&& in) noexcept:
			vma{ in.vma },
			handle{ in.handle },
			alloc{ in.alloc }
		{
			in.Nullify();
		}

		BoxVkBuffer& operator=(BoxVkBuffer&& in) noexcept {
			if (this == &in)
				return *this;
			Destroy();
			vma = in.vma;
			handle = in.handle;
			alloc = in.alloc;
			in.Nullify();
			return *this;
		}

		VmaAllocator vma = {};
		vk::Buffer handle = {};
		VmaAllocation alloc = {};

		static BoxVkBuffer Adopt(
			VmaAllocator vma,
			vk::Buffer handle,
			VmaAllocation alloc)
		{
			BoxVkBuffer returnVal{};
			returnVal.vma = vma;
			returnVal.handle = handle;
			returnVal.alloc = alloc;
			return returnVal;
		}

		struct Release_ReturnT {
			vk::Buffer handle;
			VmaAllocator vma;
			VmaAllocation alloc;
		};

		[[nodiscard]] Release_ReturnT Release() noexcept {
			Release_ReturnT returnValue;
			returnValue.handle = handle;
			returnValue.alloc = alloc;
			returnValue.vma = vma;

			handle = vk::Buffer{};

			return returnValue;
		}

		void Destroy() noexcept {
			if (handle != vk::Buffer{})
				vmaDestroyBuffer(vma, (VkBuffer) handle, alloc);
		}

		~BoxVkBuffer() noexcept {
			Destroy();
		}

	protected:
		void Nullify() {
			handle = vk::Buffer{};
		}
	};

	class BoxVkImg
	{
	public:
		constexpr BoxVkImg() noexcept = default;

		BoxVkImg(BoxVkImg const&) = delete;
		BoxVkImg& operator=(BoxVkImg const&) = delete;

		BoxVkImg(BoxVkImg&& in) noexcept:
			vma{ in.vma },
			handle{ in.handle },
			alloc{ in.alloc }
		{
			in.Nullify();
		}

		BoxVkImg& operator=(BoxVkImg&& in) noexcept {
			if (this == &in)
				return *this;
			Destroy();
			vma = in.vma;
			handle = in.handle;
			alloc = in.alloc;
			in.Nullify();
			return *this;
		}

		VmaAllocator vma = {};
		vk::Image handle = {};
		VmaAllocation alloc = {};

		static BoxVkImg Adopt(
			VmaAllocator vma,
			vk::Image handle,
			VmaAllocation alloc)
		{
			BoxVkImg returnVal{};
			returnVal.vma = vma;
			returnVal.handle = handle;
			returnVal.alloc = alloc;
			return returnVal;
		}

		struct Release_ReturnT {
			vk::Image handle;
			VmaAllocator vma;
			VmaAllocation alloc;
		};

		[[nodiscard]] Release_ReturnT Release() noexcept {
			Release_ReturnT returnValue;
			returnValue.handle = handle;
			returnValue.alloc = alloc;
			returnValue.vma = vma;

			handle = vk::Image{};

			return returnValue;
		}

		void Destroy() noexcept {
			if (handle != vk::Image{})
				vmaDestroyImage(vma, (VkImage)handle, alloc);
		}

		~BoxVkImg() noexcept {
			Destroy();
		}

	protected:
		void Nullify() {
			handle = vk::Image{};
		}
	};
}