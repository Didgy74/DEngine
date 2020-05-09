#pragma once

#include "DEngine/Gfx/Gfx.hpp"

#include "DEngine/FixedWidthTypes.hpp"

#include "VulkanIncluder.hpp"
#include "PhysDeviceInfo.hpp"
#include "DynamicDispatch.hpp"
#include "QueueData.hpp"
#include "VMAIncluder.hpp"
#include "DeletionQueue.hpp"

#include <mutex>

namespace DEngine::Gfx::Vk
{
	// Everything here is thread-safe to use and access!!
	struct GlobUtils
	{
		GlobUtils();
		GlobUtils(GlobUtils const&) = delete;
		GlobUtils(GlobUtils&&) = delete;

		Gfx::ILog* logger = nullptr;

		InstanceDispatch instance{};

		DebugUtilsDispatch debugUtils{};
		vk::DebugUtilsMessengerEXT debugMessenger{};
		bool UsingDebugUtils() const {
			return debugUtils.raw.vkCmdBeginDebugUtilsLabelEXT != nullptr;
		}
		DebugUtilsDispatch const* DebugUtilsPtr() const {
			return debugUtils.raw.vkCmdBeginDebugUtilsLabelEXT != nullptr ? &debugUtils : nullptr;
		}

		PhysDeviceInfo physDevice{};
		DeviceDispatch device{};

		QueueData queues{};

		VmaAllocator vma{};

		DeletionQueue deletionQueue;

		u8 resourceSetCount = 0;
		u8 CurrentResourceSetIndex_Async();

		bool useEditorPipeline = false;
		vk::RenderPass guiRenderPass{};

		vk::RenderPass gfxRenderPass{};

	private:
		void SetCurrentResourceSetIndex(u8 index);
		std::mutex currentResourceSetIndex_Lock{};
		u8 currentResourceSetIndex_Var{};

		friend struct APIData;
	};
}