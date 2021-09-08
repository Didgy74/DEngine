#pragma once

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

#include <DEngine/Math/Matrix.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Math/Common.hpp>

#include <vector>
#include <cstddef>

namespace DEngine::Gfx
{
	class WsiInterface;
	class LogInterface;
	struct TextureAssetInterface;
	struct InitInfo;
	class ViewportRef;
	struct DrawParams;
	enum class TextureID : u64 { Invalid = u64(-1) };
	enum class ViewportID : u64 { Invalid = u64(-1) };
	enum class NativeWindowID : u64 { Invalid = u64(-1) };
	enum class NativeWindowEvent : u32;

	class Context
	{
	public:
		Context(Context&&) noexcept;
		virtual ~Context();

		// Thread safe
		NativeWindowID NewNativeWindow(WsiInterface& wsiConnection);
		void DeleteNativeWindow(NativeWindowID);

		// Thread safe
		void NewFontTexture(
			u32 id,
			u32 width,
			u32 height,
			u32 pitch,
			Std::Span<std::byte const> data);

		// Thread safe
		ViewportRef NewViewport();
		// Thread safe
		void DeleteViewport(ViewportID viewportID);

		void Draw(DrawParams const& params);

	private:
		Context() = default;
		Context(Context const&) = delete;

		LogInterface* logger = nullptr;
		TextureAssetInterface const* texAssetInterface = nullptr;

		void* apiDataBuffer = nullptr;

		friend Std::Opt<Context> Initialize(InitInfo const& initInfo);
	};

	struct TextureAssetInterface
	{
	public:
		virtual ~TextureAssetInterface() {};

		virtual char const* get(TextureID id) const = 0;
	};

	struct ViewportUpdate
	{
		ViewportID id;
		u32 width;
		u32 height;
		Math::Mat4 transform;

		enum class GizmoType : u8 { Translate, Rotate, Scale, COUNT };
		struct Gizmo
		{
			// In world space
			Math::Vec3 position;
			f32 rotation;
			f32 scale;
			GizmoType type;
			f32 quadOffset;
			f32 quadScale;
		};
		Std::Opt<Gizmo> gizmoOpt;
	};

	struct GuiVertex
	{
		Math::Vec2 position;
		Math::Vec2 uv;
	};

	struct GuiDrawCmd
	{
		enum class Type
		{
			FilledMesh,
			TextGlyph,
			Viewport,
			Scissor
		};
		Type type;
		struct MeshSpan
		{
			u32 indexCount;
			u32 vertexOffset;
			u32 indexOffset;
		};
		struct FilledMesh
		{
			MeshSpan mesh;
			Math::Vec4 color;
		};
		struct TextGlyph
		{
			u32 utfValue;
			Math::Vec4 color;
		};
		struct Viewport
		{
			ViewportID id;
		};
		union
		{
			FilledMesh filledMesh;
			TextGlyph textGlyph;
			Viewport viewport;
		};
		// In the range of 0-1
		Math::Vec2 rectPosition;
		// In the range of 0-1
		Math::Vec2 rectExtent;
	};

	struct NativeWindowUpdate
	{
		NativeWindowID id;
		NativeWindowEvent event;
		Math::Vec4 clearColor;
		u32 drawCmdOffset;
		u32 drawCmdCount;
	};

	struct LineDrawCmd
	{
		Math::Vec4 color;
		u32 vertCount;
	};

	struct DrawParams
	{
		// Scene specific stuff, this is WIP
		std::vector<TextureID> textureIDs;
		std::vector<Math::Mat4> transforms;

		// This is decent generic stuff
		std::vector<LineDrawCmd> lineDrawCmds;
		std::vector<Math::Vec3> lineVertices;

		std::vector<GuiVertex> guiVertices;
		std::vector<u32> guiIndices;
		std::vector<GuiDrawCmd> guiDrawCmds;
		std::vector<ViewportUpdate> viewportUpdates;
		std::vector<NativeWindowUpdate> nativeWindowUpdates;
	};

	struct InitInfo
	{
		WsiInterface* initialWindowConnection = nullptr;

		LogInterface* optional_logger = nullptr;
		
		TextureAssetInterface const* texAssetInterface = nullptr;
		Std::Span<char const*> requiredVkInstanceExtensions{};

		std::vector<Math::Vec3> gizmoArrowMesh;
		std::vector<Math::Vec3> gizmoCircleLineMesh;
		std::vector<Math::Vec3> gizmoArrowScaleMesh2d;
	};

	class LogInterface
	{
	public:
		virtual ~LogInterface() {};

		enum class Level
		{
			Info,
			Fatal
		};
		virtual void Log(Level level, char const* msg) = 0;
	};

	class WsiInterface
	{
	public:
		virtual ~WsiInterface() {};

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

		[[nodiscard]] bool IsValid() const { return viewportID != ViewportID::Invalid; }
		[[nodiscard]] Gfx::ViewportID ViewportID() const { return viewportID; }

	private:
		Gfx::ViewportID viewportID = Gfx::ViewportID::Invalid;

		friend class Context;
	};
	
	Std::Opt<Context> Initialize(InitInfo const& initInfo);
}

enum class DEngine::Gfx::NativeWindowEvent : DEngine::u32
{
	None,
	Resize,
	Restore
};