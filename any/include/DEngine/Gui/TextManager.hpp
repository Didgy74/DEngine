#pragma once

#include <DEngine/Gui/Utility.hpp>
#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/Containers/RangeFnRef.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Math/Vector.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>

// Temporary
#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>

namespace DEngine::Gui
{
	class DrawInfo;
	enum class FontFaceSizeId : u64 { Invalid = u64(-1) };

	struct TextSizeInfo {
		f32 scale;
		f32 dpiX;
		f32 dpiY;
	};

	enum class TextHeightType {
		Normal,
		Alphas,
		Numerals,
		COUNT
	};

	// Here there be no null-terminated strings allowed
	class TextManager {
	public:
		void* m_implData = nullptr;
		[[nodiscard]] void* GetImplDataPtr() { return m_implData; }
		[[nodiscard]] void const* GetImplDataPtr() const { return m_implData; }

		[[nodiscard]] FontFaceSizeId GetFontFaceSizeId(TextSizeInfo const& textSize) {
			return GetFontFaceSizeId(textSize.scale, textSize.dpiX, textSize.dpiY);
		}
		[[nodiscard]] FontFaceSizeId GetFontFaceSizeId(f32 scale, f32 dpiX, f32 dpiY);
		[[nodiscard]] FontFaceSizeId FontFaceSizeIdForLinePixelHeight(u32 height) {
			return FontFaceSizeIdForLinePixelHeight(height, TextHeightType::Normal);
		}
		[[nodiscard]] FontFaceSizeId FontFaceSizeIdForLinePixelHeight(
			u32 height,
			TextHeightType textHeightType);

		[[nodiscard]] u32 GetLineheight(TextSizeInfo const& textSize) {
			return GetLineheight(textSize.scale, textSize.dpiX, textSize.dpiY);
		}
		[[nodiscard]] u32 GetLineheight(f32 scale, f32 dpiX, f32 dpiY) {
			return GetLineheight(GetFontFaceSizeId(scale, dpiX, dpiY));
		}
		[[nodiscard]] u32 GetLineheight(FontFaceSizeId sizeId, TextHeightType textHeightType = TextHeightType::Normal);

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

		void RenderText(
			Std::Span<char const> const& str,
			Math::Vec4 const& color,
			Rect const& widgetRect,
			FontFaceSizeId fontSizeId,
			DrawInfo& drawInfo);

		[[nodiscard]] Extent GetOuterExtent(Std::Span<char const> str, TextSizeInfo const& textSize) {
			return GetOuterExtent(str, textSize.scale, textSize.dpiX, textSize.dpiY);
		}
		[[nodiscard]] Extent GetOuterExtent(
			Std::Span<char const> str,
			f32 scale,
			f32 dpiX,
			f32 dpiY)
		{
			return GetOuterExtent(
				str,
				GetFontFaceSizeId(scale, dpiX, dpiY));
		}
		[[nodiscard]] Extent GetOuterExtent(
			Std::Span<char const> str,
			FontFaceSizeId sizeId)
		{
			return GetOuterExtent(
				str,
				sizeId,
				TextHeightType::Normal,
				Std::nullOpt);
		}


		Extent GetOuterExtent(
			Std::Span<char const> str,
			FontFaceSizeId sizeId,
			TextHeightType textHeightType,
			Std::Opt<Std::FnRef<void(int, Rect const&)>> const& outRectFn)
		{
			return GetOuterExtent(
				{ (int)str.Size(), [&](auto i) { return (u32)str[i]; } },
				sizeId,
				textHeightType,
				outRectFn);
		}

		Extent GetOuterExtent(
			Std::Span<u32 const> str,
			FontFaceSizeId sizeId,
			TextHeightType textHeightType,
			Std::Opt<Std::FnRef<void(int, Rect const&)>> const& outRectFn)
		{
			return GetOuterExtent(
				{ (int)str.Size(), [&](auto i) { return str[i]; } },
				sizeId,
				textHeightType,
				outRectFn);
		}

		Extent GetOuterExtent(
			Std::RangeFnRef<u32> str,
			FontFaceSizeId sizeId,
			TextHeightType textHeightType,
			Std::Opt<Std::FnRef<void(int, Rect const&)>> const& outRectFn);

		Extent GetOuterExtent(
			Std::Span<char const> str,
			TextSizeInfo const& textSize,
			Std::Span<Rect> outRects)
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
			Std::Span<Rect> outRects)
		{
			return GetOuterExtent(
				str,
				GetFontFaceSizeId(scale, dpiX, dpiY),
				TextHeightType::Normal,
				outRects);
		}

		[[nodiscard]] Extent GetOuterExtent(
			Std::Span<char const> str,
			FontFaceSizeId sizeId,
			TextHeightType textHeightType)
		{
			return GetOuterExtent(str, sizeId, textHeightType, Std::nullOpt);
		}

		// The outputted rects are relative to (0, 0).
		// The rects do not need to be initialized.
		Extent GetOuterExtent(
			Std::Span<char const> str,
			FontFaceSizeId sizeId,
			TextHeightType textHeightType,
			Std::Span<Rect> outRects)
		{
			if (!outRects.Empty()) {
				DENGINE_IMPL_GUI_ASSERT(outRects.Size() == str.Size());
				auto lambda = [&](int i, Rect const& rect) { outRects[i] = rect; };
				return GetOuterExtent(
					str,
					sizeId,
					textHeightType,
					{ lambda });
			} else {
				return GetOuterExtent(
					str,
					sizeId,
					textHeightType,
					Std::nullOpt);
			}
		}



		void FlushQueuedJobs(Gfx::Context& gfxCtx);
	};

	namespace impl
	{
		void InitializeTextManager(
			TextManager& manager,
			App::Context* appCtx,
			Gfx::Context* gfxCtx);
	}
}