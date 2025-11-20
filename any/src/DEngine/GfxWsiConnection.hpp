#pragma once

#include <DEngine/Platform/Platform.hpp>
#include "DEngine/Gfx/Gfx.hpp"

import DEngine.FixedWidthTypes;

namespace DEngine::impl {
	struct GfxWsiConnection : public Gfx::WsiInterface
	{
		Platform::Context* appCtx = nullptr;

		virtual CreateVkSurface_ReturnT CreateVkSurface(
			Gfx::NativeWindowID windowId,
			void const* vkGetInstanceProcAddr,
			uSize vkInstance,
			void const* allocCallbacks) noexcept override
		{
			auto const result = appCtx->CreateVkSurface_ThreadSafe(
				(Platform::WindowID)windowId,
				vkGetInstanceProcAddr,
				vkInstance,
				allocCallbacks);

			CreateVkSurface_ReturnT returnValue = {};
			returnValue.vkResult = result.vkResult;
			returnValue.vkSurface = result.vkSurface;
			return returnValue;
		}
	};
}
