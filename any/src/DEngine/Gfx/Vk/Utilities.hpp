#pragma once

#include "RaiiHandles.hpp"
#include "VulkanIncluder.hpp"

import DEngine.Std.Span;
import DEngine.Std.Trait;

namespace DEngine::Gfx::Vk::Util {

	template<typename T>
	concept VkHandleConcept = requires(T a) {
		Std::Trait::existsInPack<
			vk::Buffer,
			vk::DescriptorPool>;
	};

	namespace impl {
		template<class T>
		[[nodiscard]] bool CheckHandleNotNull(T const& in) { return in != T{}; }

		template<class T>
		[[nodiscard]] bool CheckSpanItemsNotNull(Std::Span<T> const& in) {
			if (in.Size() == 0)
				return false;
			for (auto const& item : in) {
				if (item == T{})
					return false;
			}
			return true;
		}
	}

	template<VkHandleConcept T>
	[[nodiscard]] bool CheckNotNull(T const& in) { return impl::CheckHandleNotNull(in); }
	template<VkHandleConcept T>
	[[nodiscard]] bool CheckNotNull(Std::Span<T const> const& in) { return impl::CheckSpanItemsNotNull(in); }

	/*
	// TODO: Would be nice with a concept for a all vk handle types
	[[nodiscard]] inline bool CheckNotNull(vk::Buffer const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::Buffer const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::DescriptorPool const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::DescriptorPool const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::DescriptorSetLayout const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::DescriptorSetLayout const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::Image const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::Image const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::ImageView const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::ImageView const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::Pipeline const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::Pipeline const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::PipelineLayout const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::PipelineLayout const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::RenderPass const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::RenderPass const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::Sampler const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::Sampler const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::Semaphore const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::Semaphore const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(vk::ShaderModule const& in) { return impl::CheckHandleNotNull(in); }
	[[nodiscard]] inline bool CheckNotNull(Std::Span<vk::ShaderModule const> const& in) { return impl::CheckSpanItemsNotNull(in); }
	*/
}