export module DEngine.Gui.TextEngine;

import DEngine.FixedWidthTypes;
import DEngine.Gui.FontFaceSizeId;
import DEngine.Gui.Utility;
import DEngine.Std.Common;
import DEngine.Std.Span;

export namespace DEngine::Gui {
	struct TextSizeInfo {
		f32 scale;
		f32 dpiX;
		f32 dpiY;
	};

	enum class TextHeightType {
		// TODO: I don't know if it makes sense to have "normal" as an enum here.
		// This enum is usually used to get size-metrics that are tighter to specific types of strings
		// (i.e only alphas or only numerics). But for "normal" text there is more info that we care about,
		// such the height of 'x'.
		Normal,
		Alphas,
		Numerals,
		COUNT
	};

	/**
		Here there be no null-terminated strings allowed

		It's expected that the functions will load glyphs on demand.

		For now, these functions don't have to be thread-safe from the perspective of the Gui library.

		TODO: There is no API to determine the size of a selection highlight if you don't already have the
		glyph-rects for that string.

		The TextEngine class provides functionality for Widgets to measure text and interacts with the
		DrawEngine to render text.

		The TextEngine class is only ever guaranteed to be alive for the duration of a single event
		dispatch. No systems should ever store long-lived references to TextEngine instances.
	 */
	class TextEngine {
	public:
		virtual ~TextEngine() = default;

		// TODO: We no longer support DPI in both directions. Can simplify this.
		[[nodiscard]] virtual FontFaceSizeId GetFontFaceSizeId(f32 scale, f32 dpiX, f32 dpiY) = 0;
		[[nodiscard]] FontFaceSizeId GetFontFaceSizeId(TextSizeInfo const& textSize) {
			return GetFontFaceSizeId(textSize.scale, textSize.dpiX, textSize.dpiY);
		}
		[[nodiscard]] virtual FontFaceSizeId FontFaceSizeIdForLinePixelHeight(
			u32 height,
			TextHeightType textHeightType) = 0;

		// This is probably not very accurate, but whatever.
		// This struct is globally for the entire font-face. This means the values might go outside the boundary
		// of standard alphas and numerics.
		//
		// All these metrics are described in pixels.
		struct FontFaceSizeMetrics {
			// The lineHeight expresses the distance between two lines of text.
			// Probably don't use it for determining how big the text is gonna be. It's usually gonna be smaller than
			// you expect.
			u32 lineHeight = 0;
			// Ascender describes how high above the baseline the glyphs might go. Usually outside the boundaries
			// of regular alphas and numerics.
			//
			// For determining the height of the selection highlight, use ascender - descender.
			i32 ascender = 0;
			// Opposite of ascender. Describes how far stuff goes below the baseline.
			i32 descender = 0;

			// The physical height, in pixels, of a lowercase 'x'. Useful for vertically centering text.
			i32 xHeight = 0;
		};
		[[nodiscard]] virtual FontFaceSizeMetrics GetFontFaceSizeMetrics(FontFaceSizeId id) = 0;



		// GetOuterExtent should just give the visual size of the given text.
		// The line will always have a fixed height relative to the FontFaceSizeId and text-type (?)
		// So assuming alphas, it will check all lowercase and uppercase letters and find the max ascender + descender.
		//
		// A manual TextHeightType should rarely be used. Just use the standard stuff tbh.
		virtual Extent GetOuterExtent(
			Std::Span<char const> str,
			FontFaceSizeId sizeId,
			TextHeightType textHeightType,
			Std::Span<Rect> outRects) = 0;
		[[nodiscard]] Extent GetOuterExtent(
			Std::Span<char const> str,
			FontFaceSizeId sizeId)
		{
			return GetOuterExtent(
				str,
				sizeId,
				TextHeightType::Normal,
				{});
		}
		// Doesn't need to be nodiscard because the Span out parameter will hold something useful
		Extent GetOuterExtent(
			Std::Span<char const> str,
			FontFaceSizeId sizeId,
			Std::Span<Rect> outRects)
		{
			return GetOuterExtent(
				str,
				sizeId,
				TextHeightType::Normal,
				outRects);
		}

		// Gives the X offset of where to position the text caret.
		//
		// Can take in an index in the range [0, stringSize] but inclusive of stringSize.
		//
		// The returned offset is the position to the left of the chosen glyph. If given an index one past the end,
		// it will give the position to the right of the final glyph.
		[[nodiscard]] virtual u32 GetCaretPosX(
			Std::Span<char const> str,
			FontFaceSizeId sizeId,
			u32 index) = 0;
	};

	namespace TextUtils {
		inline i32 CalculateTextCenteringOffsetY(u32 boxHeight, i32 ascender, u32 xHeight) {
			// Move the text so the top of of the line matches the centerY of the outer box.
			f64 offset = (f64)boxHeight / 2;
			// Then move it back up so that the baseline of the line is what matches centerY of the box.
			offset -= ascender;
			// Then move it back down by half of the x-height
			offset += (f64)xHeight / 2;
			return (i32)Std::Round(offset);
		}

		inline i32 CalculateTextCenteringOffsetY(u32 boxHeight, TextEngine::FontFaceSizeMetrics const& sizeMetrics) {
			return CalculateTextCenteringOffsetY(boxHeight, sizeMetrics.ascender, sizeMetrics.xHeight);
		}
	}
}
