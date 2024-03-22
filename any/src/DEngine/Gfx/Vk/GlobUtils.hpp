#pragma once

#include "DEngine/Gfx/Gfx.hpp"

#include <DEngine/FixedWidthTypes.hpp>

#include "DynamicDispatch.hpp"
#include "PhysDeviceInfo.hpp"
#include "QueueData.hpp"
#include "SurfaceInfo.hpp"
#include "VMAIncluder.hpp"
#include "VulkanIncluder.hpp"

namespace DEngine::Gfx::Vk
{
	// Everything here is thread-safe to use and access!!
	class GlobUtils
	{
	public:
		GlobUtils(GlobUtils const&) = delete;
		GlobUtils(GlobUtils&&) = delete;

		Gfx::LogInterface* logger = nullptr;
		Gfx::WsiInterface* wsiInterface = nullptr;

		BaseDispatch baseDispatch{};
		InstanceDispatch instance{};

		DebugUtilsDispatch debugUtils{};
		vk::DebugUtilsMessengerEXT debugMessenger{};
		bool UsingDebugUtils() const { return debugUtils.raw.vkCreateDebugUtilsMessengerEXT != nullptr; }
		auto const* DebugUtilsPtr() const { return UsingDebugUtils() ? &debugUtils : nullptr; }

		PhysDeviceInfo physDevice{};
		DeviceDispatch device{};

		QueueData queues{};

		VmaAllocator vma{};

		u8 inFlightCount = 0;
		[[nodiscard]] int MainFencesCount() const { return inFlightCount - 1; }
		SurfaceInfo surfaceInfo{};

		bool editorMode = false;
		vk::RenderPass guiRenderPass{};
		vk::RenderPass gfxRenderPass{};

	private:
		GlobUtils();
		friend class APIData;
	};
}