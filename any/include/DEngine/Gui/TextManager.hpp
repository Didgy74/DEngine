#pragma once

#include <DEngine/Gui/Utility.hpp>
#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Math/Vector.hpp>

// Temporary
#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>

namespace DEngine::Gui
{
	class DrawInfo;
	enum class FontFaceId : u64 { Invalid = u64(-1) };

	struct TextSizeInfo {
		f32 scale;
		f32 dpiX;
		f32 dpiY;
	};

	// Here there be no null-terminated strings allowed
	class TextManager
	{
	public:
		void* m_implData = nullptr;
		[[nodiscard]] void* GetImplData() { return m_implData; }
		[[nodiscard]] void const* GetImplData() const { return m_implData; }

		[[nodiscard]] FontFaceId GetFontFaceId(TextSizeInfo const& textSize) const {
			return GetFontFaceId(textSize.scale, textSize.dpiX, textSize.dpiY);
		}
		[[nodiscard]] FontFaceId GetFontFaceId(f32 scale, f32 dpiX, f32 dpiY) const;

		[[nodiscard]] u32 GetLineheight(TextSizeInfo const& textSize) {
			return GetLineheight(textSize.scale, textSize.dpiX, textSize.dpiY);
		}
		[[nodiscard]] u32 GetLineheight(f32 scale, f32 dpiX, f32 dpiY);

		void RenderText(
			Std::Span<char const> const& str,
			Math::Vec4 const& color,
			Rect const& widgetRect,
			TextSizeInfo const& textSize,
			DrawInfo& drawInfo)
		{
			return RenderText(
				str,
				color,
				widgetRect,
				textSize.scale,
				textSize.dpiX,
				textSize.dpiY,
				drawInfo);
		}

		void RenderText(
			Std::Span<char const> const& str,
			Math::Vec4 const& color,
			Rect const& widgetRect,
			f32 scale,
			f32 dpiX,
			f32 dpiY,
			DrawInfo& drawInfo);

		[[nodiscard]] Extent GetOuterExtent(Std::Span<char const> str, TextSizeInfo const& textSize) {
			return GetOuterExtent(str, textSize.scale, textSize.dpiX, textSize.dpiY);
		}
		[[nodiscard]] Extent GetOuterExtent(
			Std::Span<char const> str,
			f32 scale,
			f32 dpiX,
			f32 dpiY);


		Extent GetOuterExtent(
			Std::Span<char const> str,
			TextSizeInfo const& textSize,
			Rect* outRects)
		{
			return GetOuterExtent(
				str,
				textSize.scale,
				textSize.dpiX,
				textSize.dpiY,
				outRects);
		}

		// The outputted rects are relative to (0, 0).
		// The rects do not need to be initialized.
		Extent GetOuterExtent(
			Std::Span<char const> str,
			f32 scale,
			f32 dpiX,
			f32 dpiY,
			Rect* outRects);
	};

	namespace impl
	{
		void InitializeTextManager(
			TextManager& manager,
			App::Context* appCtx,
			Gfx::Context* gfxCtx);
	}
}