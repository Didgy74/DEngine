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
#ifdef DENGINE_GFX_ENABLE_DEDICATED_THREAD
	constexpr bool enableDedicatedThread = true;
#else
	constexpr bool enableDedicatedThread = false;
#endif

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
	enum class FontFaceId : u64 { Invalid = u64(-1) };

	struct FontBitmapUploadJob {
		FontFaceId fontFaceId;
		u32 utfValue;
		u32 width;
		u32 height;
		u32 pitch;
		Std::Span<std::byte const> data;
	};

	class Context
	{
	public:
		Context(Context&&) noexcept;
		virtual ~Context();

		// Thread safe
		void AdoptNativeWindow(NativeWindowID);
		// Thread safe
		void DeleteNativeWindow(NativeWindowID);

		// Thread safe
		void NewFontFace(FontFaceId fontFaceId);

		// Thread safe
		// Guaranteed to copy the byte-data.
		void NewFontTextures(Std::Span<FontBitmapUploadJob const> const& jobs);

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

		void* apiDataBase = nullptr;
		void* GetApiData() { return apiDataBase; }
		void const* GetApiData() const { return apiDataBase; }

		friend Std::Opt<Context> Initialize(InitInfo const& initInfo);
	};

	struct TextureAssetInterface
	{
	public:
		virtual ~TextureAssetInterface() {};

		virtual char const* get(TextureID id) const = 0;
	};

	struct ViewportUpdate {
		ViewportID id;
		u32 width;
		u32 height;
		Math::Mat4 transform;
		Math::Vec4 clearColor = { 1.f, 0.f, 0.f, 1.f };

		enum class GizmoType : u8 { Translate, Rotate, Scale, COUNT };
		struct Gizmo {
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

	struct GuiVertex {
		Math::Vec2 position;
		Math::Vec2 uv;
	};

	struct GuiDrawCmd {
		enum class Type {
			Rectangle,
			FilledMesh,
			Text,
			Viewport,
			Scissor
		};
		Type type;
		struct MeshSpan {
			u32 indexCount;
			u32 vertexOffset;
			u32 indexOffset;
		};
		struct FilledMesh {
			MeshSpan mesh;
			Math::Vec4 color;
		};
		struct Text {
			uSize startIndex;
			uSize count;
			Math::Vec4 color;
			Math::Vec2 posOffset;
			FontFaceId fontFaceId;
		};
		struct Viewport {
			ViewportID id;
		};
		struct Rectangle {
			Math::Vec2 pos;
			Math::Vec2 extent;
			Math::Vec4 radius; // Relative to Y resolution
			Math::Vec4 color;
		};
		union {
			FilledMesh filledMesh;
			Text text;
			Viewport viewport;
			Rectangle rectangle;
		};
		// In the range of 0-1
		Math::Vec2 rectPosition;
		// In the range of 0-1
		Math::Vec2 rectExtent;
	};

	struct NativeWindowUpdate {
		NativeWindowID id;
		NativeWindowEvent event;
		Math::Vec4 clearColor;
		u32 drawCmdOffset;
		u32 drawCmdCount;
	};

	struct LineDrawCmd {
		Math::Vec4 color;
		u32 vertCount;
	};

	struct GlyphRect {
		Math::Vec2 pos;
		// This MUST be set to zero if the associated glyph does not have any corresponding bitmap.
		Math::Vec2 extent;
	};

	struct DrawParams {
		// Scene specific stuff, this is WIP
		std::vector<TextureID> textureIDs;
		std::vector<Math::Mat4> transforms;

		// This is decent generic stuff
		std::vector<LineDrawCmd> lineDrawCmds;
		std::vector<Math::Vec3> lineVertices;
		std::vector<ViewportUpdate> viewportUpdates;

		// This API is fine, but shouldn't use vectors directly in the future.
		std::vector<GuiVertex> guiVertices;
		std::vector<u32> guiIndices;
		std::vector<u32> guiUtfValues;
		std::vector<GlyphRect> guiTextGlyphRects;
		std::vector<GuiDrawCmd> guiDrawCmds;
		std::vector<NativeWindowUpdate> nativeWindowUpdates;
	};

	struct InitInfo {
		NativeWindowID initialWindow = {};
		WsiInterface* wsiConnection = nullptr;

		LogInterface* optional_logger = nullptr;
		
		TextureAssetInterface const* texAssetInterface = nullptr;
		Std::Span<char const*> requiredVkInstanceExtensions;

		std::vector<Math::Vec3> gizmoArrowMesh;
		std::vector<Math::Vec3> gizmoCircleLineMesh;
		std::vector<Math::Vec3> gizmoArrowScaleMesh2d;
	};

	class LogInterface {
	public:
		virtual ~LogInterface() {};

		enum class Level {
			Info,
			Fatal
		};
		virtual void Log(Level level, Std::Span<char const> msg) = 0;
	};

	class WsiInterface {
	public:
		virtual ~WsiInterface() = default;

		struct CreateVkSurface_ReturnT {
			u32 vkResult;
			u64 vkSurface;
		};
		virtual CreateVkSurface_ReturnT CreateVkSurface(
			NativeWindowID windowId,
			uSize vkInstance,
			void const* allocCallbacks) noexcept = 0;
	};

	class ViewportRef {
	public:
		ViewportRef() = default;

		[[nodiscard]] bool IsValid() const noexcept { return viewportID != ViewportID::Invalid; }
		[[nodiscard]] Gfx::ViewportID ViewportID() const noexcept { return viewportID; }

	private:
		Gfx::ViewportID viewportID = Gfx::ViewportID::Invalid;
		friend class Context;
	};
	
	Std::Opt<Context> Initialize(InitInfo const& initInfo);

	// Rotation happens clock-wise.
	enum class WindowRotation {
		e0,
		e90,
		e180,
		e270,
	};
}

enum class DEngine::Gfx::NativeWindowEvent : DEngine::u32
{
	None,
	Resize,
	Restore
};