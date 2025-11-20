#pragma once

#include <DEngine/Gfx/Gfx.hpp>

import DEngine.Gui.FontFaceSizeId;
import DEngine.Gui.TextEngine;
import DEngine.Gui.Utility;

namespace DEngine {
	class GuiTextEngineImpl : public Gui::TextEngine {
	public:
		// TODO: This can fail if we don't deploy necessary files and shit.
		// Should be turned into a factory function that returns an optional.
		GuiTextEngineImpl();

		struct ImplData;
		ImplData* m_implData = nullptr;
		[[nodiscard]] ImplData& GetImplDataPtr() { return *m_implData; }
		[[nodiscard]] ImplData const& GetImplDataPtr() const { return *m_implData; }

		Gui::FontFaceSizeId GetFontFaceSizeId(f32 scale, f32 dpiX, f32 dpiY) override;
		Gui::FontFaceSizeId FontFaceSizeIdForLinePixelHeight(
			u32 height,
			Gui::TextHeightType textHeightType) override;
		FontFaceSizeMetrics GetFontFaceSizeMetrics(Gui::FontFaceSizeId id) override;

		Gui::Extent GetOuterExtent(
			Std::Span<char const> str,
			Gui::FontFaceSizeId sizeId,
			Gui::TextHeightType textHeightType,
			Std::Span<Gui::Rect> outRects) override;

		// Gives the X offset of where to position the text caret.
		//
		// Can take in an index in the range [0, stringSize] but inclusive of stringSize.
		//
		// The returned offset is the position to the left of the chosen glyph. If given an index one past the end,
		// it will give the position to the right of the final glyph.
		u32 GetCaretPosX(
			Std::Span<char const> str,
			Gui::FontFaceSizeId sizeId,
			u32 index) override;
		/*
		Gui::Extent GetOuterExtent(
			Std::Span<char const> str,
			f32 scale,
			f32 dpiX,
			f32 dpiY) override;

		Gui::Extent GetOuterExtent(
			Std::Span<char const> str,
			f32 scale,
			f32 dpiX,
			f32 dpiY,
			Std::Span<Gui::Rect> outRects) override;

		Gui::Extent GetOuterExtent(
			Std::Span<char const> str,
			Gui::FontFaceSizeId sizeId) override;

		Gui::Extent GetOuterExtent(
			Std::Span<char const> str,
			Gui::FontFaceSizeId sizeId,
			Gui::TextHeightType textHeightType,
			Std::Opt<Std::FnRef<void(int, Gui::Rect const&)>> const& outRectFn) override;
		*/

		/*
		// The outputted rects are relative to (0, 0).
		// The rects do not need to be initialized.
		Gui::Extent GetOuterExtent(
			Std::Span<char const> str,
			f32 scale,
			f32 dpiX,
			f32 dpiY,
			Std::Span<Gui::Rect> outRects)
		{
			return GetOuterExtent(
				str,
				GetFontFaceSizeId(scale, dpiX, dpiY),
				Gui::TextHeightType::Normal,
				outRects);
		}
		*/

		/*
		[[nodiscard]] virtual Gui::Extent GetOuterExtent(
			Std::Span<char const> str,
			Gui::FontFaceSizeId sizeId,
			Gui::TextHeightType textHeightType)
		{
			return GetOuterExtent(str, sizeId, textHeightType, Std::nullOpt);
		}

		// The outputted rects are relative to (0, 0).
		// The rects do not need to be initialized.
		Gui::Extent GetOuterExtent(
			Std::Span<char const> str,
			Gui::FontFaceSizeId sizeId,
			Gui::TextHeightType textHeightType,
			Std::Span<Gui::Rect> outRects)
		{
			if (!outRects.Empty()) {
				DENGINE_IMPL_GUI_ASSERT(outRects.Size() == str.Size());
				auto lambda = [&](int i, Gui::Rect const& rect) { outRects[i] = rect; };
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
		*/



		// Flushes text load jobs to the renderer.
		void FlushQueuedJobs(Gfx::Context& gfxCtx);
	};
}