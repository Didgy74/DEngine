#pragma once

namespace DEngine::Gfx::Vk
{
	class APIData;
	class GlobUtils;

	class InstanceDispatch;
	class DeviceDispatch;
	class DebugUtilsDispatch;

	class NativeWinMgr;
	class NativeWinMgr_WindowData;
	class NativeWinMgr_WindowGuiData;

	class GuiResourceManager;


	class ViewportManager;
	class ViewportMgr_ViewportData;


	class DeletionQueue;
	using DelQueue = DeletionQueue;

	class QueueData;

	struct SurfaceInfo;
}