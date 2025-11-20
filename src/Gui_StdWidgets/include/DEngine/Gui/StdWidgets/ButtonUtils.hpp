#pragma once

#include <DEngine/Gui/Widget.hpp>

import DEngine.Gui.FontFaceSizeId;

// Helper functions to bring similar button sizing behavior across multiple Widgets
namespace DEngine::Gui::ButtonUtils {

	struct GetSizeHintParams {
		TextEngine& textEngine;
		EventWindowInfo const& window;
		Std::Span<char const> const& textSpan;
		f32 const& fontScale;
		u32 const& textMarginPx;
		Std::Opt<f32> const& minimumHeightCm;
	};
	struct GetSizeHintReturnT {
		SizeHint sizeHint = {};
		FontFaceSizeId fontFaceSizeId = {};
		Extent textOuterExtent = {};
	};
	[[nodiscard]] GetSizeHintReturnT GetSizeHint(
		GetSizeHintParams const& params,
		Rect* outTextGlyphRects);
}