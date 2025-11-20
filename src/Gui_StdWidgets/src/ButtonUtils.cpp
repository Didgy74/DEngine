#include <DEngine/Gui/StdWidgets/ButtonUtils.hpp>

import DEngine.Gui.TextEngine;

using namespace DEngine::Gui;

ButtonUtils::GetSizeHintReturnT ButtonUtils::GetSizeHint(
	GetSizeHintParams const& params,
	Rect* outTextGlyphRects)
{
	auto& textEngine = params.textEngine;
	auto& window = params.window;
	auto fontScale = params.fontScale;
	auto textMarginPx = params.textMarginPx;
	auto minimumHeightCmOpt = params.minimumHeightCm;
	auto textSpan = params.textSpan;

	auto normalTextScale = window.contentScale * fontScale;

	auto normalFontSizeId = textEngine.GetFontFaceSizeId(normalTextScale, window.dpi, window.dpi);
	auto normalFontHeightPx = textEngine.GetFontFaceSizeMetrics(normalFontSizeId).lineHeight;

	auto normalButtonHeightPx = normalFontHeightPx + 2*textMarginPx;
	auto outButtonHeightPx = normalButtonHeightPx;

	// If we have been given a minimum height, use that instead.
	if (minimumHeightCmOpt.Has()) {
		auto minHeightPx = CmToPixels(minimumHeightCmOpt.Get(), window.dpi);
		if (minHeightPx > normalButtonHeightPx) {
			outButtonHeightPx = minHeightPx;
		}
	}

	auto textOuterExtent = textEngine.GetOuterExtent(
		textSpan,
		normalFontSizeId,
		TextHeightType::Alphas,
		{ outTextGlyphRects, textSpan.Size() });

	SizeHint outSizeHint = {};
	outSizeHint.minimum.height = outButtonHeightPx;
	outSizeHint.minimum.width = textOuterExtent.width + 2*textMarginPx;
	// The button should ideally not be taller than it is wide.
	outSizeHint.minimum.width = Std::Max(outSizeHint.minimum.width, outSizeHint.minimum.height);

	GetSizeHintReturnT returnVal = {};
	returnVal.sizeHint = outSizeHint;
	returnVal.fontFaceSizeId = normalFontSizeId;
	returnVal.textOuterExtent = textOuterExtent;

	return returnVal;
}