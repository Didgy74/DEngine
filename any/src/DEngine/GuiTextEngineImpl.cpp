#include <DEngine/GuiTextEngineImpl.hpp>

#include <DEngine/Std/Containers/FnRef.hpp>

#include <stdexcept>

// Definitely temporary
// Make an actual good interface instead
#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Platform/Platform.hpp>


// Maybe temporary?
// Could consider type-erasing the text manager
// and move it into it's own header+source file
// Or make an actually good public interface to use it.
#include <unordered_map>
#include <functional>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftsizes.h>

import DEngine.Std.Common;
import DEngine.Std.Utility;

using namespace DEngine;

namespace DEngine::impl {
	struct GlyphData {
		FT_Glyph_Metrics internal_ftGlyphMetrics = {};
		[[nodiscard]] auto const& FtGlyphMetrics() const { return internal_ftGlyphMetrics; }

		[[nodiscard]] bool HasBitmap() const {
			return internal_ftGlyphMetrics.width != 0;
		}

		[[nodiscard]] auto Width() const {
			return (u32)Std::Round((f64)FtGlyphMetrics().width / (f64)(1 << 6));
		}
		[[nodiscard]] auto Height() const {
			return (u32)Std::Round((f64)FtGlyphMetrics().height / (f64)(1 << 6));
		}

		// in pixels
		[[nodiscard]] auto AdvanceX() const {
			return (i32)Std::Round((f64)FtGlyphMetrics().horiAdvance / (f64)(1 << 6));
		}
		// in pixels
		[[nodiscard]] auto PosOffsetX() const {
			return (i32)Std::Round((f64)FtGlyphMetrics().horiBearingX / (f64)(1 << 6));
		}
		[[nodiscard]] auto PosOffsetY() const {
			return -(i32)Std::Round((f64)FtGlyphMetrics().horiBearingY / (f64)(1 << 6));
		}
	};
}

struct GuiTextEngineImpl::ImplData {
	struct FontFaceSizeData {
		// Over time we need to load new glyphs from an existing font-face even if we've already established it.
		// This stores a capturing lambda that makes the exact call we need to make in order to setup the
		// FT_Face in a state that will load our new glyphs at the right size.
		//
		// It is critical that the given lambda captures everything it needs to do this.
		//
		// TODO: Would it make sense to have FT_Face object per font-face? We definitely need one per thread
		// if we're doing multi-threading in the future.
		std::function<void(FT_Face)> internal_requestFtSizeFn = {};
		// Should be called "Prepare"
		// Transitions the given FT_Face into a state where it will be working on the correct
		// size that we want.
		void RequestFtSize(FT_Face ftFace) const { internal_requestFtSizeFn(ftFace); }

		// DONT MODIFY ONCE SET, IS CRTICAL FOR CALCULATING KEY
		// Note: Wtf is the key? I think it's like the original FT_Size_Metrics when we loaded
		// this font-face-size, and we use it to identify it within our Map.
		FT_Size_Metrics internal_ftSizeMetrics = {};
		[[nodiscard]] auto const& FtSizeMetrics() const { return internal_ftSizeMetrics; }



		// This allows us to have different, tighter size-metric depending on whether we only want to display
		// alphas, numbers etc.
		struct SizeMetric {
			u32 lineHeight = 0;
			i32 ascender = 0;
			i32 descender = 0;
		};
		Std::Array<SizeMetric, (int)Gui::TextHeightType::COUNT> internal_sizeMetrics;
		[[nodiscard]] auto const& GetSizeMetrics(Gui::TextHeightType in) const noexcept {
			return internal_sizeMetrics[(int)in];
		}

		// TODO: We should change the name of this.
		struct NonspecificSizeMetrics {
			// lineHeight as described by FreeType. I think it represents the distance between to lines of text.
			// Not necessarily tied to any bounding box of a single line of text.
			u32 lineHeight = 0;
			i32 ascender = 0;
			i32 descender = 0;
			// Height of lowercase letter 'x'
			i32 xHeight = 0;
		};
		NonspecificSizeMetrics nonspecificSizeMetrics = {};

		// Stores glyph-data that is not in the ascii-table
		std::unordered_map<u32, impl::GlyphData> glyphDatas;

		// This loads all the low UTF-value glyphs
		// I don't know in what context it is used.
		void EnsureLowGlyphsLoaded(FT_Face ftFace);
		bool lowGlyphsLoaded = false;
		static constexpr uSize lowGlyphTableSize = 256;
		// Stores glyph-data that is in the ASCII-range.
		// Is not guaranteed to be filled even if the FontFaceSizeData? Could be useful if we're querying the global
		// font data but still have never queried any specific fonts.
		Std::Array<impl::GlyphData, lowGlyphTableSize> lowGlyphDatas;

		// Stores the UTF-32 value of glyphs that we know we need to rasterize and upload to the GPU
		// for the next frame.
		std::vector<u32> glyphBitmapUploadJobs;

		FontFaceSizeData() = default;
		FontFaceSizeData(FontFaceSizeData const&) = delete;
		FontFaceSizeData(FontFaceSizeData&&) = default;
		FontFaceSizeData& operator=(FontFaceSizeData const&) = delete;
		FontFaceSizeData& operator=(FontFaceSizeData&&) = default;
	};

	// FreeType requires that our entire font file is stored in memory for the lifetime of the FT_Library
	std::vector<u8> fontFileData;
	FT_Library ftLib = {};
	// The FT_Face object is a stateful one that we change in order to load new font info and glyph info.
	// This needs to have the correct size set, and then the correct glyph set before we can load most stuff.
	//
	// TODO: This can get problematic once we start reading font data in multi-threaded context.
	FT_Face ftFace = {};

	// We want to have the functionality where we guesstimate what point-size of text is gonna fill a certain pixel
	// height. For that we use a reference font-face-size, and we trust that it scales uniformly.
	// This should never actually load any glyphs (?)
	FontFaceSizeData referenceSize = {};

	struct FontFaceNode {
		[[nodiscard]] auto Key() const { return (Gui::FontFaceSizeId)sizeData.FtSizeMetrics().height; }
		FontFaceSizeData sizeData;
	};
	std::vector<FontFaceNode> faceNodes;

	[[nodiscard]] FontFaceNode* TryFindFontSizeNode(Gui::FontFaceSizeId sizeId) {
		auto nodeIt = Std::FindIf(
			faceNodes.begin(),
			faceNodes.end(),
			[&](auto const& item) { return item.Key() == sizeId; });
		if (nodeIt != faceNodes.end())
			return &*nodeIt;
		return nullptr;
	}

	// The renderer requires us to explicitly create font-faces before we can start uploading
	// glyphs associated with that font-face. This vector tracks which font-faces we need to create in the
	// renderer before the next frame starts.
	std::vector<Gui::FontFaceSizeId> fontFaceSizeUploadJobs;
	void PushFontSizeIdUploadJob(Gui::FontFaceSizeId id) {
		auto contains = Std::Contains(
			fontFaceSizeUploadJobs.begin(),
			fontFaceSizeUploadJobs.end(),
			id);
		if (!contains) {
			fontFaceSizeUploadJobs.push_back(id);
		}
	}
};


namespace DEngine::impl
{
	constexpr int baseFontSize = 12;
	constexpr int referenceFontSizePixels = 100;

	struct DumbFontFaceMetrics
	{

		int x_ppem;
		int y_ppem;

		float x_scale;
		float y_scale;

		float ascender;
		float descender;
		float height;
		float max_advance;
	};

	auto ToDumb(FT_Size_Metrics in) {
		DumbFontFaceMetrics out = {};
		out.x_ppem = in.x_ppem;
		out.y_ppem = in.y_ppem;
		// This member is in 16.16 fractional point. We gotta divide by 2^16.
		out.x_scale = (float)in.x_scale / (1 << 16);
		out.x_scale = (float)in.x_scale / (1 << 16);

		// This member is in 26.6 fractional point. We gotta divide by 2^6.
		out.ascender = (float)in.ascender / (1 << 6);
		out.descender = (float)in.descender / (1 << 6);
		out.height = (float)in.height / (1 << 6);
		out.max_advance = (float)in.max_advance / (1 << 6);
		return out;
	}

	struct DumbGlyphMetrics {
		float width;
		float height;

		float horiBearingX;
		float horiBearingY;
		float horiAdvance;

		float vertBearingX;
		float vertBearingY;
		float vertAdvance;
	};

	auto ToDumb(FT_Glyph_Metrics in) {
		DumbGlyphMetrics out = {};
		out.width = (float)in.width / (1 << 6);
		out.height = (float)in.height / (1 << 6);
		out.horiBearingX = (float)in.horiBearingX / (1 << 6);
		out.horiBearingY = (float)in.horiBearingY / (1 << 6);
		out.horiAdvance = (float)in.horiAdvance / (1 << 6);
		out.vertBearingX = (float)in.vertBearingX / (1 << 6);
		out.vertBearingY = (float)in.vertBearingY / (1 << 6);
		out.vertAdvance = (float)in.vertAdvance / (1 << 6);
		return out;
	}

	[[nodiscard]] auto GenFontSizeKeyFrom(FT_Size_Metrics const& metrics) {
		return (Gui::FontFaceSizeId)metrics.height;
	}

	Std::Opt<GlyphData> LoadNewGlyph(
		FT_Face face,
		u32 utfValue)
	{
		DENGINE_IMPL_ASSERT(utfValue != 0);

		// Load glyph data
		FT_UInt glyphIndex = FT_Get_Char_Index(face, utfValue);
		if (glyphIndex == 0) // 0 is an error index
			return Std::nullOpt;

		FT_Error ftError = FT_Load_Glyph(
			face,
			glyphIndex,
			FT_LOAD_DEFAULT);
		// TODO: Should probably be doing proper error handling.
		if (ftError != FT_Err_Ok)
			return Std::nullOpt;

		GlyphData newData{};
		newData.internal_ftGlyphMetrics = face->glyph->metrics;
		return newData;
	}

	GuiTextEngineImpl::ImplData::FontFaceSizeData LoadFontFaceSizeData(
		FT_Face ftFace,
		std::function<void(FT_Face)>&& requestSizeFn)
	{
		requestSizeFn(ftFace);

		GuiTextEngineImpl::ImplData::FontFaceSizeData output = {};

		output.internal_ftSizeMetrics = ftFace->size->metrics;

		output.internal_requestFtSizeFn = Std::Move(requestSizeFn);

		// Load global size metrics for this font-face-size
		{
			output.nonspecificSizeMetrics.lineHeight = output.internal_ftSizeMetrics.height / (1 << 6);
			output.nonspecificSizeMetrics.ascender = output.internal_ftSizeMetrics.ascender / (1 << 6);
			output.nonspecificSizeMetrics.descender = output.internal_ftSizeMetrics.descender / (1 << 6);
			auto xGlyphDataOpt = LoadNewGlyph(ftFace, 'x');
			if (!xGlyphDataOpt.Has())
				throw std::runtime_error("bad bad");
			auto const& xGlyphData = xGlyphDataOpt.Value();
			output.nonspecificSizeMetrics.xHeight = xGlyphData.Height();
		}


		{
			auto& metrics = output.internal_sizeMetrics[(int)Gui::TextHeightType::Normal];
			metrics.lineHeight = output.internal_ftSizeMetrics.height / (1 << 6);
			metrics.ascender = output.internal_ftSizeMetrics.ascender / (1 << 6);
			metrics.descender = output.internal_ftSizeMetrics.descender / (1 << 6);
		}

		auto loadSizeMetric = [](FT_Face ftFace, Std::Span<char const> str) {
			GuiTextEngineImpl::ImplData::FontFaceSizeData::SizeMetric output = {};
			i32 maxOffsetY = 0;
			i32 maxBottom = 0;
			for (auto i : str) {
				auto index = FT_Get_Char_Index(ftFace, i);
				if (i == 0)
					continue;

				FT_Load_Glyph(ftFace, index, FT_LOAD_DEFAULT);
				i32 currHeight = ftFace->glyph->metrics.height;
				i32 currOffset = ftFace->glyph->metrics.horiBearingY;
				if (currOffset > maxOffsetY)
					maxOffsetY = currOffset;

				i32 currBottom = currHeight - currOffset;
				if (currBottom > maxBottom)
					maxBottom = currBottom;
			}
			output.lineHeight = (maxOffsetY + maxBottom) / (1 << 6);
			output.ascender = maxOffsetY / (1 << 6);
			return output;
		};

		auto getTestString = [](Gui::TextHeightType in) {
			using T = Gui::TextHeightType;
			switch (in) {
				case T::Normal: return Std::CStrToSpan("");
				case T::Alphas: return Std::CStrToSpan("abcdefghijklmnpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
				case T::Numerals: return Std::CStrToSpan("0123456789");
				default: return Std::Span<char const>{};
			}
		};

		// Load the alpha data
		for (int i = 0; i < (int)Gui::TextHeightType::COUNT; i++) {
			auto type = (Gui::TextHeightType)i;
			if (type == Gui::TextHeightType::Normal)
				continue;

			auto testString = getTestString(type);
			output.internal_sizeMetrics[i] = loadSizeMetric(ftFace, testString);
		}

		return output;
	}

	[[nodiscard]] auto GetFontFaceSizeId(
		GuiTextEngineImpl::ImplData& implData,
		f32 scale,
		f32 dpiX,
		f32 dpiY)
	{
		auto ftFace = implData.ftFace;

		float sizeInPt = baseFontSize * scale;

		auto requestFn = [=](FT_Face ftFaceIn) {
			// FreeType takes fixed-point floating point integers, with 6 bits for decimal.
			// To convert our size from regular integer to this, we multiply by 2^6 (64)
			auto ftError = FT_Set_Char_Size(
				ftFaceIn,
				0,
				(int)Std::Round(sizeInPt * (1 << 6)),
				(u32)Std::Round(dpiX),
				(u32)Std::Round(dpiY));
			if (ftError != FT_Err_Ok)
				throw std::runtime_error("DEngine - TextManager: Unable to set pixel sizes");
		};
		requestFn(ftFace);
		auto key = GenFontSizeKeyFrom(ftFace->size->metrics);

		auto nodePtr = implData.TryFindFontSizeNode(key);

		if (nodePtr == nullptr) {
			// Create the node
			GuiTextEngineImpl::ImplData::FontFaceNode newNode = {};
			newNode.sizeData = LoadFontFaceSizeData(
				ftFace,
				requestFn);
			implData.faceNodes.emplace_back(Std::Move(newNode));
			implData.PushFontSizeIdUploadJob((Gui::FontFaceSizeId)key);
		}
		// Otherwise just return the id.
		return (Gui::FontFaceSizeId)key;
	}

	// Grabs the node that stores all info of a font at a specific size
	GuiTextEngineImpl::ImplData::FontFaceSizeData& GetFontSizeData(
		GuiTextEngineImpl::ImplData& implData,
		Gui::FontFaceSizeId sizeId)
	{
		auto nodeIt = Std::FindIf(
			implData.faceNodes.begin(),
			implData.faceNodes.end(),
			[&](auto const& item) {
				return item.Key() == sizeId;
			});
		DENGINE_IMPL_ASSERT(nodeIt != implData.faceNodes.end());
		auto& node = *nodeIt;
		return node.sizeData;
	}

	GuiTextEngineImpl::ImplData::FontFaceSizeData& GetFontFace(
		GuiTextEngineImpl::ImplData& implData,
		f32 inScale,
		f32 inDpiX,
		f32 inDpiY)
	{
		DENGINE_IMPL_ASSERT(inScale > 0.f);
		DENGINE_IMPL_ASSERT(inDpiX > 0.f);
		DENGINE_IMPL_ASSERT(inDpiY > 0.f);

		auto ftFace = implData.ftFace;

		// We need to consistently assign our FT_Face a specific size when retrieving it.
		// By storing the exact function we call and the captures in a lambda, we make sure
		// we always get the exact same size info whenever we are modifying our FT_Face to load
		// the given size.
		auto requestFn = [=](FT_Face ftFace) {
			auto ftError = FT_Set_Char_Size(
				ftFace,
				0,
				(int)Std::Round((f64)baseFontSize * (f64)inScale * (f64)(1 << 6)),
				(u32)Std::Round(inDpiX),
				(u32)Std::Round(inDpiY));
			if (ftError != FT_Err_Ok) {
				throw std::runtime_error("Unable to request size");
			}
		};
		requestFn(ftFace);
		auto targetKey = GenFontSizeKeyFrom(ftFace->size->metrics);

		// Then search for it
		auto nodePtr = implData.TryFindFontSizeNode(targetKey);
		if (nodePtr != nullptr) {
			return nodePtr->sizeData;
		}

		// Load the new size
		GuiTextEngineImpl::ImplData::FontFaceNode newNode = {};
		newNode.sizeData = LoadFontFaceSizeData(
			ftFace,
			requestFn);
		implData.faceNodes.emplace_back(Std::Move(newNode));
		implData.PushFontSizeIdUploadJob(targetKey);

		return implData.faceNodes.back().sizeData;

	}

	auto const& GetGlyphData(
		GuiTextEngineImpl::ImplData::FontFaceSizeData& sizeData,
		FT_Face ftFace,
		u32 utfValue)
	{
		sizeData.EnsureLowGlyphsLoaded(ftFace);

		if (utfValue < sizeData.lowGlyphDatas.Size()) {
			// ASCII values are already loaded, so we don't need to check if it is.
			return sizeData.lowGlyphDatas[utfValue];
		}

		// Search our map for the glyph. If we didn't find the glyph, then we load it.
		auto glyphDataIt = sizeData.glyphDatas.find(utfValue);
		if (glyphDataIt == sizeData.glyphDatas.end()) {
			auto glyphOpt = LoadNewGlyph(ftFace, utfValue);
			if (!glyphOpt.Has())
				throw std::runtime_error("Unable to load glyph");

			glyphDataIt = sizeData.glyphDatas
				.insert(std::pair{ utfValue, Std::Move(glyphOpt.Get()) })
				.first;

			// If the new glyph has a bitmap associated that we can render, we queue it up for rasterization and
			// sending it to the renderer.
			if (glyphDataIt->second.HasBitmap()) {
				sizeData.glyphBitmapUploadJobs.push_back(utfValue);
			}
		}

		return glyphDataIt->second;
	}
}

using namespace DEngine;


GuiTextEngineImpl::GuiTextEngineImpl()
{
	auto* implDataPtr = new ImplData();
	this->m_implData = implDataPtr;
	auto& implData = *implDataPtr;

	FT_Error ftError = FT_Init_FreeType(&implData.ftLib);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Editor: Unable to initialize FreeType");

	Platform::FileInputStream fontFile("assets/Gui/Roboto-Light.ttf");
	if (!fontFile.IsOpen())
		throw std::runtime_error("DEngine - Editor: Unable to open font file.");
	fontFile.Seek(0, Platform::FileInputStream::SeekOrigin::End);
	u64 fileSize = fontFile.Tell().Value();
	implData.fontFileData.resize((uSize)fileSize);
	fontFile.Seek(0, Platform::FileInputStream::SeekOrigin::Start);
	fontFile.Read((char*)implData.fontFileData.data(), fileSize);
	fontFile.Close();

	FT_Face ftFace;
	ftError = FT_New_Memory_Face(
		implData.ftLib,
		(FT_Byte const*)implData.fontFileData.data(),
		(FT_Long)implData.fontFileData.size(),
		0,
		&ftFace);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Text Manager: Unable to load font");
	implData.ftFace = ftFace;

	{
		// Load a reference pixel-height font, so we have a reference for when requesting
		// fonts to fit pixel sizes that are not just the outermost bounding box.
		auto requestSizeFn = [](FT_Face ftFace) {
			FT_Set_Pixel_Sizes(ftFace, 0, impl::referenceFontSizePixels);
		};
		requestSizeFn(ftFace);
		// Then we load the data for this???
		implData.referenceSize = impl::LoadFontFaceSizeData(ftFace, requestSizeFn);
	}

	/*
	FT_Size_RequestRec sizeRequest = {};
	sizeRequest.type = FT_SIZE_REQUEST_TYPE_BBOX;
	// Need to convert to 26.6 fractional floating point.
	sizeRequest.width = 1000 << 6;
	sizeRequest.height = 1000 << 6;
	sizeRequest.horiResolution = 0;
	sizeRequest.vertResolution = 0;
	ftError = FT_Request_Size(ftFace, &sizeRequest);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - TextManager: Unable to set pixel sizes");

	ftError = FT_Set_Char_Size(ftFace, 100 * 64, 100 * 64, 0, 0);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - TextManager: Unable to set pixel sizes");
	ftError = FT_Set_Pixel_Sizes(ftFace, 0, 100);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - TextManager: Unable to set pixel sizes");

	auto sizeA = ToDumb(ftFace->size->metrics);

	ftError = FT_Load_Glyph(
		ftFace,
		'e',
		FT_LOAD_DEFAULT);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("Unable to load glyph");
	ftError = FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("Unable to render glyph");

	auto glyphMetricsA = ToDumb(ftFace->glyph->metrics);


	FT_Size_RequestRec sizeRequestB = {};
	sizeRequestB.type = FT_SIZE_REQUEST_TYPE_NOMINAL;
	// Need to convert to 26.6 fractional floating point.
	sizeRequestB.width = 72 << 6;
	sizeRequestB.height = 72 << 6;
	sizeRequestB.horiResolution = 144;
	sizeRequestB.vertResolution = 144;
	ftError = FT_Request_Size(ftFace, &sizeRequestB);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - TextManager: Unable to set pixel sizes");


	auto sizeB = ToDumb(ftFace->size->metrics);

	ftError = FT_Load_Glyph(
		ftFace,
		'T',
		FT_LOAD_DEFAULT);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("Unable to load glyph");

	auto glyphMetricsB = ToD
	umb(ftFace->glyph->metrics);
	*/

}

void GuiTextEngineImpl::ImplData::FontFaceSizeData::EnsureLowGlyphsLoaded(
	FT_Face inFtFace)
{
	if (this->lowGlyphsLoaded)
		return;

	this->RequestFtSize(inFtFace);

	// Loop over ASCII glyphs and load them. Load all ASCII characters.
	for (int i = 32; i < this->lowGlyphDatas.Size(); i += 1) {
		u32 utfValue = (u32)i;
		auto glyphOpt = impl::LoadNewGlyph(inFtFace, utfValue);
		if (glyphOpt.Has()) {
			this->lowGlyphDatas[i] = Std::Move(glyphOpt.Get());
			// If we loaded the glyph successfully, and we know it has a bitmap that we can display. Queue it for
			// rasterization and sending it to the renderer.
			if (this->lowGlyphDatas[i].HasBitmap()) {
				this->glyphBitmapUploadJobs.push_back(utfValue);
			}
		}
	}

	lowGlyphsLoaded = true;
}

namespace DEngine::impl
{
	static Gui::Extent TextMan_GetOuterExtent(
		GuiTextEngineImpl& textManager,
		Std::Span<char const> str,
		Gui::FontFaceSizeId fontFaceSizedId,
		Gui::TextHeightType textHeightType,
		Std::Span<Gui::Rect> outRects)
	{
		auto& implData = textManager.GetImplDataPtr();
		auto& sizeData = impl::GetFontSizeData(implData, fontFaceSizedId);

		auto const& metrics = sizeData.GetSizeMetrics(textHeightType);

		Gui::Extent returnValue = {};
		returnValue.height = metrics.lineHeight;

		// Iterate over the string, find the bounding box width
		Math::Vec2Int penPos = {};
		penPos.y = metrics.ascender;
		int const strLength = (int)str.Size();
		for (int i = 0; i < strLength; i += 1) {
			auto const glyphChar = str[i];
			DENGINE_IMPL_ASSERT(glyphChar != 0);

			auto const& glyphData = impl::GetGlyphData(
				sizeData,
				implData.ftFace,
				glyphChar);

			if (i == 0)
				penPos.x -= glyphData.PosOffsetX();

			if (outRects.Data() != nullptr) {
				Gui::Rect rect = {};
				if (glyphData.HasBitmap()) {
					rect.position = penPos;
					rect.position.x += glyphData.PosOffsetX();
					rect.position.y += glyphData.PosOffsetY();
					rect.extent.width = glyphData.Width();
					rect.extent.height = glyphData.Height();
				}
				outRects[i] = rect;
			}

			if (i == strLength - 1) {
				returnValue.width += glyphData.Width();
				returnValue.width += glyphData.PosOffsetX();
			} else {
				returnValue.width += glyphData.AdvanceX();
			}

			penPos.x += glyphData.AdvanceX();
		}

		return returnValue;
	}
}

/*
u32 GuiTextEngineImpl::GetLineheight(Gui::FontFaceSizeId sizeId, Gui::TextHeightType textHeightType) {
	using namespace impl;
	auto& implData = this->GetImplDataPtr();
	auto& sizeData = impl::GetFontSizeData(implData, sizeId);
	return sizeData.GetSizeMetrics(textHeightType).lineHeight;
}
*/

Gui::FontFaceSizeId GuiTextEngineImpl::GetFontFaceSizeId(f32 scale, f32 dpiX, f32 dpiY)
{
	using namespace impl;
	auto& implData = this->GetImplDataPtr();
	return impl::GetFontFaceSizeId(implData, scale, dpiX, dpiY);
}

Gui::FontFaceSizeId GuiTextEngineImpl::FontFaceSizeIdForLinePixelHeight(
	u32 height,
	Gui::TextHeightType textHeightType)
{
	using namespace impl;

	auto& implData = this->GetImplDataPtr();
	auto const& referenceSize = implData.referenceSize;

	auto ftFace = implData.ftFace;

	auto referenceSizeMetrics = referenceSize.GetSizeMetrics(textHeightType);
	auto referenceSizeLineheight = referenceSize.GetSizeMetrics(Gui::TextHeightType::Normal).lineHeight;
	auto requestFn = [=](FT_Face ftFace){

		// Calculate some sort of ratio?
		auto ratio = (f64)referenceSizeLineheight / (f64)referenceSizeMetrics.lineHeight;

		auto newHeight = (f64)height * ratio;

		FT_Size_RequestRec req = {};
		req.type = FT_SIZE_REQUEST_TYPE_BBOX;
		req.width = 0;
		req.height = (FT_Long)Std::Floor(newHeight) * (1 << 6);
		auto ftError = FT_Request_Size(ftFace, &req);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("freetype unable to load this size.");
	};
	requestFn(ftFace);

	auto targetKey = GenFontSizeKeyFrom(ftFace->size->metrics);

	auto nodePtr = implData.TryFindFontSizeNode(targetKey);

	if (nodePtr == nullptr) {
		// Load the new size
		ImplData::FontFaceNode newNode = {};
		newNode.sizeData = LoadFontFaceSizeData(ftFace, requestFn);
		implData.faceNodes.emplace_back(Std::Move(newNode));
		implData.PushFontSizeIdUploadJob(targetKey);
	}

	return targetKey;
}

Gui::TextEngine::FontFaceSizeMetrics GuiTextEngineImpl::GetFontFaceSizeMetrics(Gui::FontFaceSizeId id)
{
	auto& implData = this->GetImplDataPtr();
	auto& sizeData = impl::GetFontSizeData(implData, id);

	FontFaceSizeMetrics returnValue = {};
	returnValue.lineHeight = sizeData.nonspecificSizeMetrics.lineHeight;
	returnValue.ascender = sizeData.nonspecificSizeMetrics.ascender;
	returnValue.descender = sizeData.nonspecificSizeMetrics.descender;
	returnValue.xHeight = sizeData.nonspecificSizeMetrics.xHeight;
	return returnValue;
}

Gui::Extent GuiTextEngineImpl::GetOuterExtent(
	Std::Span<char const> str,
	Gui::FontFaceSizeId fontFaceSizeId,
	Gui::TextHeightType textHeightType,
	Std::Span<Gui::Rect> outRects)
{
	return impl::TextMan_GetOuterExtent(
		*this,
		str,
		fontFaceSizeId,
		textHeightType,
		outRects);
}

u32 GuiTextEngineImpl::GetCaretPosX(
	Std::Span<char const> str,
	Gui::FontFaceSizeId sizeId,
	u32 targetIndex)
{
	auto& implData = this->GetImplDataPtr();
	auto& sizeData = impl::GetFontSizeData(implData, sizeId);

	DENGINE_IMPL_ASSERT(targetIndex <= str.Size());

	if (targetIndex == 0 && str.Empty())
		return 0;
	else if (targetIndex == 0 && !str.Empty()) {
	}

	i32 accumulator = 0;
	for (int temp = 0; temp < targetIndex; temp += 1) {
		int i = temp;

		auto const glyphChar = str[i];
		DENGINE_IMPL_ASSERT(glyphChar != 0);
		auto const& glyphData = impl::GetGlyphData(
			sizeData,
			implData.ftFace,
			glyphChar);

		if (i == 0)
			accumulator -= glyphData.PosOffsetX();

		// If we are at the final glyph we want, and there is a bitmap, we want to align it perfectly... Kind of?
		accumulator += glyphData.AdvanceX();
	}

	return accumulator;
}

void GuiTextEngineImpl::FlushQueuedJobs(Gfx::Context& gfxCtx)
{
	using namespace impl;
	auto& implData = this->GetImplDataPtr();
	auto ftFace = implData.ftFace;

	for (auto const& item : implData.fontFaceSizeUploadJobs) {
		gfxCtx.NewFontFace((Gfx::FontFaceId)item);
	}
	implData.fontFaceSizeUploadJobs.clear();

	int ftError = {};
	// Loop over all fonts and check if they have any upload jobs
	for (auto& sizedFont : implData.faceNodes) {
		if (!sizedFont.sizeData.glyphBitmapUploadJobs.empty()) {

			sizedFont.sizeData.RequestFtSize(ftFace);

			for (auto const& utfValue : sizedFont.sizeData.glyphBitmapUploadJobs) {
				// Render the glyph
				auto ftGlyphIndex = FT_Get_Char_Index(ftFace, utfValue);
				if (ftGlyphIndex == 0)
					throw std::runtime_error("FreeType: tried to grab and render an invalid char index.");

				ftError = FT_Load_Glyph(
					ftFace,
					ftGlyphIndex,
					FT_LOAD_DEFAULT);
				if (ftError == FT_Err_Invalid_Size_Handle)
					throw std::runtime_error("FreeType: Invalid size handle when loading glyph.");
				if (ftError != FT_Err_Ok)
					throw std::runtime_error("Unable to load glyph");

				ftError = FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL);
				if (ftError != FT_Err_Ok)
					throw std::runtime_error("Unable to render glyph");

				auto const& ftBitmap = ftFace->glyph->bitmap;
				Gfx::FontBitmapUploadJob job = {};
				job.fontFaceId = (Gfx::FontFaceId)sizedFont.Key();
				job.utfValue = utfValue;
				job.width = ftBitmap.width;
				job.height = ftBitmap.rows;
				job.pitch = ftBitmap.pitch;
				job.data = { (char const*)ftBitmap.buffer, ftBitmap.pitch * ftBitmap.rows };
				gfxCtx.NewFontTextures(Std::Span{ job });
			}
		}
		sizedFont.sizeData.glyphBitmapUploadJobs.clear();
	}
}

/*
void GuiTextEngineImpl::RenderText(
	Std::Span<char const> const& input,
	Math::Vec4 const& color,
	Gui::Rect const& widgetRect,
	Gui::FontFaceSizeId sizeId,
	Gui::DrawInfo& drawInfo)
{
	auto& implData = this->GetImplDataPtr();
	auto& fontFace = impl::GetFontSizeData(implData, sizeId);

	auto& metrics = fontFace.GetSizeMetrics(Gui::TextHeightType::Normal);

	Math::Vec2Int penPos = widgetRect.position;
	penPos.y += metrics.ascender;

	auto& utfValues = *drawInfo.utfValues;
	auto oldUtfValuesLen = utfValues.size();
	auto& rects = *drawInfo.textGlyphRects;

	auto inputSize = input.Size();

	// Copy the UTF values
	utfValues.resize(oldUtfValuesLen + inputSize);
	for (int i = 0; i < input.Size(); i++)
		utfValues[oldUtfValuesLen + i] = (unsigned char)input[i];

	Gfx::GuiDrawCmd cmd = {};
	cmd.type = Gfx::GuiDrawCmd::Type::Text;
	cmd.text.fontFaceId = (Gfx::FontFaceId)sizeId;
	cmd.text.startIndex = oldUtfValuesLen;
	cmd.text.count = input.Size();
	cmd.text.color = color;

	// Push the rects
	rects.resize(oldUtfValuesLen + inputSize);
	auto newRects = Std::Span{ rects.data() + oldUtfValuesLen, inputSize };

	auto fbExtent = drawInfo.GetFramebufferExtent();

	auto setRectLambda = [=](auto i, Rect const& rect) {
		newRects[i] = Gfx::GlyphRect {
			.pos = {
				.x = (f32)(rect.position.x + widgetRect.position.x) / (f32)fbExtent.width,
				.y = (f32)(rect.position.y + widgetRect.position.y) / (f32)fbExtent.height,
			},
			.extent = {
				.x = (f32)rect.extent.width / (f32)fbExtent.width,
				.y = (f32)rect.extent.height / (f32)fbExtent.height,
			}};
	};
	this->GetOuterExtent(
		input,
		sizeId,
		TextHeightType::Normal,
		{ setRectLambda });

	drawInfo.drawCmds->push_back(cmd);
}
*/