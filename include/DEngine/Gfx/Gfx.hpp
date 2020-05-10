#pragma once

#include "DEngine/FixedWidthTypes.hpp"
#include "DEngine/Containers/Span.hpp"
#include "DEngine/Containers/Optional.hpp"
#include "DEngine/Containers/StaticVector.hpp"

#include "DEngine/Math/Matrix.hpp"
#include "DEngine/Math/Common.hpp"

#include <vector>

namespace DEngine::Gfx
{
	class IWsi;
	class ILog;
	struct InitInfo;
	class ViewportRef;
	struct DrawParams;
	enum class TextureID : u64 {};

	struct TextureAssetInterface
	{
	public:
		virtual ~TextureAssetInterface() {};

		virtual char const* get(TextureID id) const = 0;
	};

	struct ViewportUpdateData
	{
		uSize id = static_cast<uSize>(-1);
		u32 width = 0;
		u32 height = 0;
		Math::Mat4 transform{};
	};

	struct DrawParams
	{
		u32 swapchainWidth = 0;
		u32 swapchainHeight = 0;
		bool restoreEvent = false;

		std::vector<TextureID> textureIDs;
		std::vector<Math::Mat4> transforms;

		std::vector<ViewportUpdateData> viewportUpdates;
	};

	class Data
	{
	public:
		Data(Data&&) noexcept;
		virtual ~Data();

		ViewportRef NewViewport();
		void DeleteViewport(uSize viewportID);
		uSize GetViewportCount();

		void Draw(DrawParams const& params);

	private:
		Data() = default;
		Data(Data const&) = delete;

		ILog* iLog = nullptr;
		IWsi* iWsi = nullptr;
		TextureAssetInterface const* texAssetInterface = nullptr;

		void* apiDataBuffer = nullptr;

		friend Std::Opt<Data> Initialize(const InitInfo& initInfo);
	};

	struct InitInfo
	{
		u32 maxWidth = 0;
		u32 maxHeight = 0;

		ILog* optional_iLog = nullptr;
		IWsi* iWsi = nullptr;
		TextureAssetInterface const* texAssetInterface = nullptr;
		Std::Span<char const*> requiredVkInstanceExtensions{};
	};

	class ILog
	{
	public:
		virtual ~ILog() {};

		virtual void log(char const* msg) = 0;
	};

	class IWsi
	{
	public:
		virtual ~IWsi() {};

		// Return type is VkResult
		//
		// Argument #1: VkInstance - The Vulkan instance handle
		// Argument #2: VkAllocationCallbacks const* - Allocation callbacks for surface creation.
		// Argument #3: VkSurfaceKHR* - The output surface handle
		virtual i32 CreateVkSurface(uSize vkInstance, void const* allocCallbacks, u64& outSurface) = 0;
	};

	class ViewportRef
	{
	public:
		ViewportRef() = default;

		[[nodiscard]] bool IsValid() const { return viewportID != invalidID; }
		[[nodiscard]] uSize ViewportID() const { return viewportID; }
		[[nodiscard]] void* ImGuiTexID() const { return imguiTexID; }

	private:
		static constexpr uSize invalidID = static_cast<uSize>(-1);
		uSize viewportID = invalidID;
		void* imguiTexID = nullptr;

		friend class Data;
	};

	
	Std::Opt<Data> Initialize(const InitInfo& initInfo);
}