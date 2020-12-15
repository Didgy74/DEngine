#pragma once

#include "DEngine/Gfx/Gfx.hpp"

#include <DEngine/FixedWidthTypes.hpp>

#include "DeletionQueue.hpp"
#include "DynamicDispatch.hpp"
#include "PhysDeviceInfo.hpp"
#include "QueueData.hpp"
#include "SurfaceInfo.hpp"
#include "VMAIncluder.hpp"
#include "VulkanIncluder.hpp"

#include <mutex>

namespace DEngine::Gfx::Vk
{
	// Everything here is thread-safe to use and access!!
	class GlobUtils
	{
	public:
		GlobUtils(GlobUtils const&) = delete;
		GlobUtils(GlobUtils&&) = delete;

		Gfx::LogInterface* logger = nullptr;

		InstanceDispatch instance{};

		DebugUtilsDispatch debugUtils{};
		vk::DebugUtilsMessengerEXT debugMessenger{};
		bool UsingDebugUtils() const { return debugUtils.raw.vkCreateDebugUtilsMessengerEXT != nullptr; }
		DebugUtilsDispatch const* DebugUtilsPtr() const { return UsingDebugUtils() ? &debugUtils : nullptr; }

		PhysDeviceInfo physDevice{};
		DeviceDispatch device{};

		QueueData queues{};

		VmaAllocator vma{};

		DeletionQueue deletionQueue;

		u8 inFlightCount = 0;

		SurfaceInfo surfaceInfo{};

		bool editorMode = false;
		vk::RenderPass guiRenderPass{};
		vk::RenderPass gfxRenderPass{};

	private:
		GlobUtils();
		friend class APIData;
	};
}