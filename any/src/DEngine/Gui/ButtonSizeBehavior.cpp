#include <DEngine/Gui/ButtonSizeBehavior.hpp>

using namespace DEngine::Gui;

FontFaceSizeId ButtonSizeBehavior::FindFontFaceSizeId(
	f32 normalSize,
	f32 dpiX,
	f32 dpiY,
	f32 minimumHeightCm,
	TextManager& textMgr)
{
	// Figure out if the default font will be smaller than our wanted size.
	auto lineHeightPixels = textMgr.GetLineheight(normalSize, dpiX, dpiY);

	auto dotsPerCm = dpiY / 2.54;
	// Convert millimeters to pixels
	auto minimumPhysicalSizePixels = (int)Math::Ceil(minimumHeightCm * dotsPerCm);

	auto useMinimumHeight = lineHeightPixels < minimumPhysicalSizePixels;

	auto fontSizeId = FontFaceSizeId::Invalid;
	if (useMinimumHeight) {
		// Find a fitting font size
		fontSizeId = textMgr.FontFaceSizeIdForLinePixelHeight(minimumPhysicalSizePixels);
	} else {
		fontSizeId = textMgr.GetFontFaceSizeId(normalSize, dpiX, dpiY);
	}
	return fontSizeId;
}


/*
ButtonSizeBehavior::ViewData ButtonSizeBehavior::GatherSizeHint(
	GatherSizeHint_Params const& params,
	Std::Opt<Std::FnRef<void(int, Rect const&)>> const& rectOutputFn)
{
	auto normalSizeScale = params.normalSizeScale;
	auto dpiX = params.dpiX;
	auto dpiY = params.dpiY;
	auto minimumHeightCm = params.minimumHeightCm;
	auto& textMgr = params.textMgr;

	ViewData returnValue = {};

	// Figure out if the default font will be smaller than our wanted size.
	auto lineHeightPixels = textMgr.GetLineheight(normalSizeScale, dpiX, dpiY);
	auto minimumPhysicalSizePixels = CmToPixels(minimumHeightCm, dpiY);
	auto useMinimumHeight = lineHeightPixels < minimumPhysicalSizePixels;

	if (useMinimumHeight) {
		returnValue.fontSizeId = textMgr.FontFaceSizeIdForLinePixelHeight(minimumPhysicalSizePixels);
	} else {
		returnValue.fontSizeId = textMgr.GetFontFaceSizeId(normalSizeScale, dpiX, dpiY);
	}

	returnValue.textOuterExtent = textMgr.GetOuterExtent(
		params.text,
		returnValue.fontSizeId,
		rectOutputFn);

	if (useMinimumHeight) {
		returnValue.textOuterExtent.height = minimumPhysicalSizePixels;
	}

	returnValue.sizeHint.minimum = returnValue.textOuterExtent;
	returnValue.sizeHint.minimum.width = Math::Max(
		returnValue.sizeHint.minimum.width,
		(u32)CmToPixels(minimumHeightCm, dpiX));
	returnValue.sizeHint.minimum.height = Math::Max(
		returnValue.sizeHint.minimum.height,
		(u32)CmToPixels(minimumHeightCm, dpiY));

	return returnValue;
}
 */