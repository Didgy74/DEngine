#pragma once

#include "DEngine/Int.hpp"
#include "DEngine/Containers/Span.hpp"
#include "DEngine/Containers/Optional.hpp"

namespace DEngine::Gfx
{
	namespace Constants
	{
		constexpr u8 maxViewportCount = 8;
	}

	class IWSIInterfacer;
	class ILog;
	struct InitInfo;
	class ViewportRef;

	class Data
	{
	public:
		Data(Data&&) noexcept = default;;

		bool resizeEvent = false;
		bool rebuildVkSurface = false;

		ViewportRef NewViewport();
		u8 GetViewportCount();

	private:
		Data() = default;
		Data(Data const&) = delete;

		const ILog* iLog = nullptr;

		void* apiDataBuffer{};

		friend Cont::Opt<Data> Initialize(const InitInfo&);
		friend void Draw(Data&);
	};

	struct InitInfo
	{
		u32 maxWidth = 0;
		u32 maxHeight = 0;

		bool (*createVkSurfacePFN)(u64 vkInstance, void* userData, u64* vkSurface) = nullptr;
		void* createVkSurfaceUserData = nullptr;

		ILog* iLog = nullptr;
		IWSIInterfacer* iWsiInterface = nullptr;
		Cont::Span<char const*> requiredVkInstanceExtensions{};
	};

	class ILog
	{
	public:
		virtual ~ILog() {};

		virtual void log(char const* msg) = 0;
	};

	class IWSIInterfacer
	{
	public:
		virtual ~IWSIInterfacer() {};

		// Return type is VkResult
		//
		// Argument #1: VkInstance - The Vulkan instance handle
		// Argument #2: VkAllocationCallbacks const* - Allocation callbacks for surface creation.
		// Argument #3: VkSurfaceKHR* - The output surface handle
		virtual i32 createVkSurface(u64 vkInstance, void const* allocCallbacks, u64* outSurface) = 0;
	};

	class ViewportRef
	{
	public:
		u8 GetViewportID() const { return viewportID; }
		void* GetImGuiTexID() const { return imguiTexID; }

	private:
		ViewportRef() = default;
		u8 viewportID = 255;
		void* imguiTexID = nullptr;

		friend class Data;
	};

	Cont::Opt<Data> Initialize(const InitInfo& initInfo);

	void Draw(Data& data);
}