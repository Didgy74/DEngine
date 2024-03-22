#pragma once

#include <DEngine/Gui/TextManager.hpp>

#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/Opt.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>

namespace DEngine::Gui::ButtonSizeBehavior {
	[[nodiscard]] FontFaceSizeId FindFontFaceSizeId(
		f32 normalSize,
		f32 dpiX,
		f32 dpiY,
		f32 minimumHeightCm,
		TextManager& textMgr);

	struct ViewData {
		FontFaceSizeId fontSizeId;
		Extent textOuterExtent;
		SizeHint sizeHint;
	};

	struct GatherSizeHint_Params {
		f32 normalSizeScale = {};
		f32 dpiX = {};
		f32 dpiY = {};
		f32 minimumHeightCm = {};
		Std::Span<char const> text;
		TextManager& textMgr;
	};
	[[nodiscard]] ViewData GatherSizeHint(
		GatherSizeHint_Params const& params,
		Std::Opt<Std::FnRef<void(int, Rect const&)>> const& rectOutputFn);

	[[nodiscard]] inline ViewData GatherSizeHint(
		GatherSizeHint_Params const& params,
		Rect* outRects)
	{
		auto lambda = [=](int i, Rect const& rect) {
			outRects[i] = rect;
		};
		if (outRects != nullptr) {
			return GatherSizeHint(params, { lambda });
		} else {
			return GatherSizeHint(params, Std::nullOpt);
		}
	}
}