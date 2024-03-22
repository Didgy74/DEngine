#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Array.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>

#include <stdexcept>

// Definitely temporary
// Make an actual good interface instead
#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Application.hpp>

#include <DEngine/Math/Common.hpp>
#include <DEngine/FixedWidthTypes.hpp>
#include <DEngine/Std/Utility.hpp>
#include <DEngine/Std/Containers/Opt.hpp>

// Maybe temporary?
// Could consider type-erasing the text manager
// and move it into it's own header+source file
// Or make an actually good public interface to use it.
#include <unordered_map>
#include <functional>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftsizes.h>

namespace DEngine::Gui::impl
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

	struct GlyphData {
		FT_Glyph_Metrics internal_ftGlyphMetrics = {};
		[[nodiscard]] auto const& FtGlyphMetrics() const { return internal_ftGlyphMetrics; }

		[[nodiscard]] bool HasBitmap() const {
			return internal_ftGlyphMetrics.width != 0;
		}

		[[nodiscard]] auto Width() const {
			return (u32)Math::Round((f64)FtGlyphMetrics().width / (f64)(1 << 6));
		}
		[[nodiscard]] auto Height() const {
			return (u32)Math::Round((f64)FtGlyphMetrics().height / (f64)(1 << 6));
		}

		// in pixels
		[[nodiscard]] auto AdvanceX() const {
			return (i32)Math::Round((f64)FtGlyphMetrics().horiAdvance / (f64)(1 << 6));
		}
		// in pixels
		[[nodiscard]] auto PosOffsetX() const {
			return (i32)Math::Round((f64)FtGlyphMetrics().horiBearingX / (f64)(1 << 6));
		}
		[[nodiscard]] auto PosOffsetY() const {
			return -(i32)Math::Round((f64)FtGlyphMetrics().horiBearingY / (f64)(1 << 6));
		}
	};

	[[nodiscard]] auto GenFontSizeKeyFrom(FT_Size_Metrics const& metrics) {
		return (FontFaceSizeId)metrics.height;
	}

	struct TextManagerImpl
	{
		std::vector<u8> fontFileData;
		FT_Library ftLib = {};

		struct FontFaceSizeData {
			void EnsureInit(FT_Face ftFace);

			std::function<void(FT_Face)> internal_requestSizeFn = {};
			void RequestFtSize(FT_Face ftFace) const {
				internal_requestSizeFn(ftFace);
			}

			// DONT MODIFY ONCE SET, IS CRTICAL FOR CALCULATING KEY
			FT_Size_Metrics internal_ftSizeMetrics = {};
			[[nodiscard]] auto const& FtSizeMetrics() const { return internal_ftSizeMetrics; }
			bool initialized = false;

			struct SizeMetric {
				u32 lineHeight = 0;
				i32 ascender = 0;
			};
			Std::Array<SizeMetric, (int)TextHeightType::COUNT> internal_sizeMetrics;
			[[nodiscard]] auto const& GetSizeMetrics(TextHeightType in) const noexcept {
				return internal_sizeMetrics[(int)in];
			}

			// Stores glyph-data that is not in the ascii-table
			std::unordered_map<u32, GlyphData> glyphDatas;

			static constexpr uSize lowGlyphTableSize = 128;
			// Stores glyph-data that is in the ascii-table.
			Std::Array<GlyphData, lowGlyphTableSize> lowGlyphDatas;

			std::vector<u32> glyphBitmapUploadJobs;

			FontFaceSizeData() = default;
			FontFaceSizeData(FontFaceSizeData const&) = delete;
			FontFaceSizeData(FontFaceSizeData&&) = default;
			FontFaceSizeData& operator=(FontFaceSizeData const&) = delete;
			FontFaceSizeData& operator=(FontFaceSizeData&&) = default;
		};

		struct FontFaceNode {
			[[nodiscard]] auto Key() const { return (FontFaceSizeId)sizeData.FtSizeMetrics().height; }
			FontFaceSizeData sizeData;
		};
		FT_Face ftFace = {};

		FontFaceSizeData referenceSize = {};
		std::vector<FontFaceNode> faceNodes;
		[[nodiscard]] FontFaceNode* SearchFontSizeNode(FontFaceSizeId sizeId) {
			auto nodeIt = Std::FindIf(
				faceNodes.begin(),
				faceNodes.end(),
				[&](auto const& item) { return item.Key() == sizeId; });
			if (nodeIt != faceNodes.end())
				return &*nodeIt;
			else
				return nullptr;
		}

		std::vector<FontFaceSizeId> fontFaceSizeUploadJobs;
		void PushFontSizeIdUploadJob(FontFaceSizeId id) {
			auto contains = Std::Contains(
				fontFaceSizeUploadJobs.begin(),
				fontFaceSizeUploadJobs.end(),
				id);
			if (!contains) {
				fontFaceSizeUploadJobs.push_back(id);
			}
		}
	};

	[[nodiscard]] auto& GetImplData(TextManager& in) { return *static_cast<TextManagerImpl*>(in.GetImplDataPtr()); }
	[[nodiscard]] auto const& GetImplData(TextManager const& in) { return *static_cast<TextManagerImpl const*>(in.GetImplDataPtr()); }

	// Assumes the size has already been requested and set.
	TextManagerImpl::FontFaceSizeData LoadFontFaceSizeData(
		FT_Face ftFace,
		std::function<void(FT_Face)>&& requestSizeFn)
	{
		TextManagerImpl::FontFaceSizeData output = {};

		output.internal_ftSizeMetrics = ftFace->size->metrics;
		output.internal_requestSizeFn = Std::Move(requestSizeFn);

		{
			auto& metrics = output.internal_sizeMetrics[(int)TextHeightType::Normal];
			metrics.lineHeight = ftFace->size->metrics.height / (1 << 6);
			metrics.ascender = ftFace->size->metrics.ascender / (1 << 6);
		}

		auto loadSizeMetric = []( FT_Face ftFace, Std::Span<char const> str) {
			TextManagerImpl::FontFaceSizeData::SizeMetric output = {};
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

		auto getTestString = [](TextHeightType in) {
			using T = TextHeightType;
			switch (in) {
				case T::Normal: return Std::CStrToSpan("");
				case T::Alphas: return Std::CStrToSpan("abcdefghijklmnpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
				case T::Numerals: return Std::CStrToSpan("0123456789");
				default: return Std::Span<char const>{};
			}
		};

		// Load the alpha data
		for (int i = 0; i < (int)TextHeightType::COUNT; i++) {
			auto type = (TextHeightType)i;
			if (type == TextHeightType::Normal)
				continue;

			auto testString = getTestString(type);
			output.internal_sizeMetrics[i] = loadSizeMetric(ftFace, testString);
		}

		return output;
	}

	[[nodiscard]] auto GetFontFaceSizeId(
		TextManagerImpl& implData,
		f32 scale,
		f32 dpiX,
		f32 dpiY)
	{
		auto ftFace = implData.ftFace;

		float sizeInPt = baseFontSize * scale;

		auto requestFn = [=](FT_Face ftFace) {
			// FreeType takes fixed-point floating point integers,
			// with 6 bits for decimal.
			// To convert our size from regular integer to this,
			// we multiply by 2^6 (64)
			auto ftError = FT_Set_Char_Size(
				ftFace,
				0,
				(int)Math::Round(sizeInPt * (1 << 6)),
				(u32)Math::Round(dpiX),
				(u32)Math::Round(dpiY));
			if (ftError != FT_Err_Ok)
				throw std::runtime_error("DEngine - TextManager: Unable to set pixel sizes");
		};
		requestFn(ftFace);
		auto key = GenFontSizeKeyFrom(ftFace->size->metrics);

		auto nodePtr = implData.SearchFontSizeNode(key);

		if (nodePtr == nullptr) {
			// Create the node
			TextManagerImpl::FontFaceNode newNode = {};
			newNode.sizeData = LoadFontFaceSizeData(
				ftFace,
				requestFn);
			implData.faceNodes.emplace_back(Std::Move(newNode));
			implData.PushFontSizeIdUploadJob((FontFaceSizeId)key);
		}
		// Otherwise just return the id.
		return (FontFaceSizeId)key;
	}

	Std::Opt<GlyphData> LoadNewGlyph(
		TextManagerImpl::FontFaceSizeData& sizeData,
		FT_Face face,
		u32 utfValue)
	{
		DENGINE_IMPL_GUI_ASSERT(utfValue != 0);

		// Load glyph data
		FT_UInt glyphIndex = FT_Get_Char_Index(face, utfValue);
		if (glyphIndex == 0) // 0 is an error index
			return Std::nullOpt;

		FT_Error ftError = FT_Load_Glyph(
			face,
			glyphIndex,
			FT_LOAD_DEFAULT);
		if (ftError == FT_Err_Invalid_Size_Handle)
			throw std::runtime_error("FreeType: Invalid size handle when loading glyph.");
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("Unable to load glyph");

		/*
		ftError = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("Unable to render glyph");
		 */

		if (face->glyph->metrics.width != 0) {
			sizeData.glyphBitmapUploadJobs.push_back(utfValue);
		}

		GlyphData newData{};
		newData.internal_ftGlyphMetrics = face->glyph->metrics;
		return newData;
	}

	TextManagerImpl::FontFaceSizeData& GetFontSizeData(
		TextManagerImpl& implData,
		FontFaceSizeId sizeId)
	{
		auto nodeIt = Std::FindIf(
		implData.faceNodes.begin(),
		implData.faceNodes.end(),
		[&](auto const& item) {
			return item.Key() == sizeId;
		});
		DENGINE_IMPL_GUI_ASSERT(nodeIt != implData.faceNodes.end());
		auto& node = *nodeIt;
		return node.sizeData;
	}

	TextManagerImpl::FontFaceSizeData& GetFontFace(
		TextManagerImpl& implData,
		f32 inScale,
		f32 inDpiX,
		f32 inDpiY)
	{
		DENGINE_IMPL_GUI_ASSERT(inScale > 0.f);
		DENGINE_IMPL_GUI_ASSERT(inDpiX > 0.f);
		DENGINE_IMPL_GUI_ASSERT(inDpiY > 0.f);

		auto ftFace = implData.ftFace;

		auto requestFn = [=](FT_Face ftFace) {
			auto ftError = FT_Set_Char_Size(
				ftFace,
				0,
				(int)Math::Round((f64)baseFontSize * (f64)inScale * (f64)(1 << 6)),
				(u32)Math::Round(inDpiX),
				(u32)Math::Round(inDpiY));
			if (ftError != FT_Err_Ok) {
				throw std::runtime_error("Unable to request size");
			}
		};
		requestFn(ftFace);
		auto targetKey = GenFontSizeKeyFrom(ftFace->size->metrics);

		// Then search for it
		auto nodePtr = implData.SearchFontSizeNode(targetKey);
		if (nodePtr != nullptr) {
			return nodePtr->sizeData;
		} else {
			// Load the new size
			TextManagerImpl::FontFaceNode newNode = {};

			newNode.sizeData = LoadFontFaceSizeData(
				ftFace,
				requestFn);
			implData.faceNodes.emplace_back(Std::Move(newNode));
			implData.PushFontSizeIdUploadJob(targetKey);

			return implData.faceNodes.back().sizeData;
		}
	}

	auto const& GetGlyphData(
		TextManagerImpl::FontFaceSizeData& sizeData,
		FT_Face ftFace,
		u32 utfValue)
	{
		sizeData.EnsureInit(ftFace);

		if (utfValue < sizeData.lowGlyphDatas.Size()) {
			// ASCII values are already loaded, so we don't need to check if it is.
			return sizeData.lowGlyphDatas[utfValue];
		}
		else
		{
			auto glyphDataIt = sizeData.glyphDatas.find(utfValue);
			if (glyphDataIt == sizeData.glyphDatas.end()) {
				auto glyphOpt = LoadNewGlyph(sizeData, ftFace, utfValue);
				if (!glyphOpt.Has())
					throw std::runtime_error("Unable to load glyph");
				glyphDataIt = sizeData.glyphDatas
					.insert(std::pair{ utfValue, Std::Move(glyphOpt.Get()) })
					.first;
			}

			return glyphDataIt->second;
		}
	}
}

using namespace DEngine;
using namespace DEngine::Gui;

void Gui::impl::InitializeTextManager(
	TextManager& manager,
	App::Context* appCtx,
	Gfx::Context* gfxCtx)
{
	auto* implDataPtr = new TextManagerImpl();
	manager.m_implData = implDataPtr;
	auto& implData = *implDataPtr;

	FT_Error ftError = FT_Init_FreeType(&implData.ftLib);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Editor: Unable to initialize FreeType");

	App::FileInputStream fontFile("data/gui/Roboto-Light.ttf");
	if (!fontFile.IsOpen())
		throw std::runtime_error("DEngine - Editor: Unable to open font file.");
	fontFile.Seek(0, App::FileInputStream::SeekOrigin::End);
	u64 fileSize = fontFile.Tell().Value();
	implData.fontFileData.resize((uSize)fileSize);
	fontFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
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
			FT_Set_Pixel_Sizes(ftFace, 0, referenceFontSizePixels);
		};
		requestSizeFn(ftFace);
		// Then we load the data for this???
		implData.referenceSize = LoadFontFaceSizeData(ftFace, requestSizeFn);
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

	auto glyphMetricsB = ToDumb(ftFace->glyph->metrics);
	*/


	int i = 0;

}

void Gui::impl::TextManagerImpl::FontFaceSizeData::EnsureInit(
	FT_Face inFtFace)
{
	if (initialized)
		return;

	this->RequestFtSize(inFtFace);

	// Loop over ASCII glyphs and load thmem
	// Load all ASCII characters.


	for (int i = 32; i < lowGlyphDatas.Size(); i += 1) {
		auto glyphOpt = LoadNewGlyph(*this, inFtFace, (u32)i);
		if (glyphOpt.Has()) {
			this->lowGlyphDatas[i] = Std::Move(glyphOpt.Get());
		}

	}


	initialized = true;
}

namespace DEngine::Gui::impl
{
	static Extent TextMan_GetOuterExtent2(
		TextManager& textManager,
		Std::RangeFnRef<u32> str,
		FontFaceSizeId fontFaceSizedId,
		TextHeightType textHeightType,
		Std::Opt<Std::FnRef<void(int i, Rect const& outRect)>> const& outputRectFn)
	{
		auto& implData = GetImplData(textManager);
		auto& sizeData = impl::GetFontSizeData(implData, fontFaceSizedId);

		auto const& metrics = sizeData.GetSizeMetrics(textHeightType);

		Extent returnValue = {};
		returnValue.height = metrics.lineHeight;

		// Iterate over the string, find the bounding box width
		Math::Vec2Int penPos = {};
		penPos.y = metrics.ascender;
		int const strLength = (int)str.Size();
		for (int i = 0; i < strLength; i += 1) {
			auto const glyphChar = str.Invoke(i);
			DENGINE_IMPL_GUI_ASSERT(glyphChar != 0);

			auto const& glyphData = impl::GetGlyphData(
				sizeData,
				implData.ftFace,
				glyphChar);

			if (i == 0)
				penPos.x -= glyphData.PosOffsetX();

			if (outputRectFn.Has()) {
				Rect rect = {};
				if (glyphData.HasBitmap()) {
					rect.position = penPos;
					rect.position.x += glyphData.PosOffsetX();
					rect.position.y += glyphData.PosOffsetY();
					rect.extent.width = glyphData.Width();
					rect.extent.height = glyphData.Height();
				}
				outputRectFn.Get()(i, rect);
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

	[[nodiscard]] static Extent TextMan_GetOuterExtent(
		TextManager& textManager,
		Std::RangeFnRef<u32> str,
		FontFaceSizeId sizeId,
		TextHeightType textHeightType,
		Rect* outRects)
	{
		if (outRects != nullptr) {
			auto lambda = [&outRects](auto i, Rect const& x){ outRects[i] = x; };
			return TextMan_GetOuterExtent2(
				textManager,
				str,
				sizeId,
				textHeightType,
				{ lambda });
		} else {
			return TextMan_GetOuterExtent2(
				textManager,
				str,
				sizeId,
				textHeightType,
				Std::nullOpt);
		}
	}
}

Extent Gui::TextManager::GetOuterExtent(
	Std::RangeFnRef<u32> str,
	FontFaceSizeId sizeId,
	TextHeightType textHeightType,
	Std::Opt<Std::FnRef<void(int, Rect const&)>> const& outRectFn)
{
	return impl::TextMan_GetOuterExtent2(
		*this,
		str,
		sizeId,
		textHeightType,
		outRectFn);
}

u32 TextManager::GetLineheight(FontFaceSizeId sizeId, TextHeightType textHeightType) {
	using namespace impl;
	auto& implData = GetImplData(*this);
	auto& sizeData = impl::GetFontSizeData(implData, sizeId);
	return sizeData.GetSizeMetrics(textHeightType).lineHeight;
}

FontFaceSizeId TextManager::GetFontFaceSizeId(f32 scale, f32 dpiX, f32 dpiY)
{
	using namespace impl;
	auto& implData = GetImplData(*this);
	return (FontFaceSizeId)impl::GetFontFaceSizeId(implData, scale, dpiX, dpiY);
}

FontFaceSizeId TextManager::FontFaceSizeIdForLinePixelHeight(
	u32 height,
	TextHeightType textHeightType)
{
	using namespace impl;

	auto& implData = GetImplData(*this);
	auto const& referenceSize = implData.referenceSize;

	auto ftFace = implData.ftFace;

	auto referenceSizeMetrics = referenceSize.GetSizeMetrics(textHeightType);
	auto referenceSizeLineheight = referenceSize.GetSizeMetrics(TextHeightType::Normal).lineHeight;
	auto requestFn = [=](FT_Face ftFace){

		// Calculate some sort of ratio?
		auto ratio = (f64)referenceSizeLineheight / (f64)referenceSizeMetrics.lineHeight;

		auto newHeight = (f64)height * ratio;

		if (textHeightType == TextHeightType::Alphas) {
			int i = 0;
		}

		FT_Size_RequestRec req = {};
		req.type = FT_SIZE_REQUEST_TYPE_BBOX;
		req.width = 0;
		req.height = (FT_Long)Math::Floor(newHeight) * (1 << 6);
		auto ftError = FT_Request_Size(ftFace, &req);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("freetype unable to load this size.");
	};
	requestFn(ftFace);

	auto targetKey = GenFontSizeKeyFrom(ftFace->size->metrics);

	auto nodePtr = implData.SearchFontSizeNode(targetKey);

	if (nodePtr == nullptr) {
		// Load the new size
		TextManagerImpl::FontFaceNode newNode = {};
		newNode.sizeData = LoadFontFaceSizeData(ftFace, requestFn);
		implData.faceNodes.emplace_back(Std::Move(newNode));
		implData.PushFontSizeIdUploadJob(targetKey);
	}

	return (FontFaceSizeId)targetKey;
}


void Gui::TextManager::RenderText(
	Std::Span<char const> const& input,
	Math::Vec4 const& color,
	Rect const& widgetRect,
	FontFaceSizeId sizeId,
	DrawInfo& drawInfo)
{
	using namespace impl;

	auto& implData = GetImplData(*this);
	auto& fontFace = impl::GetFontSizeData(implData, sizeId);

	auto& metrics = fontFace.GetSizeMetrics(TextHeightType::Normal);

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



void TextManager::FlushQueuedJobs(Gfx::Context& gfxCtx)
{
	using namespace impl;
	auto& implData = GetImplData(*this);
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

				if (utfValue == 65) {
					int i = 0;
				}

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
				job.data = { (std::byte const*)ftBitmap.buffer, ftBitmap.pitch * ftBitmap.rows };
				gfxCtx.NewFontTextures(job);
			}
		}
		sizedFont.sizeData.glyphBitmapUploadJobs.clear();
	}


}