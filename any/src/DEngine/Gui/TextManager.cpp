#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Array.hpp>

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

#include <ft2build.h>
#include FT_FREETYPE_H

namespace DEngine::Gui::impl
{
	struct TextManagerImpl
	{
		Gfx::Context* gfxCtx = nullptr;
		App::Context* appCtx = nullptr;

		std::vector<u8> fontFileData;
		FT_Library ftLib = {};

		struct GlyphData
		{
			bool hasBitmap = {};
			u32 bitmapWidth = {};
			u32 bitmapHeight = {};

			i32 advanceX = {};
			Math::Vec2Int posOffset = {};
		};
		struct FontFace {
			FT_Face ftFace = {};

			u32 lineheight = 0;
			u32 lineMinY = 0;

			// Stores glyph-data that is not in the ascii-table
			std::unordered_map<u32, GlyphData> glyphDatas;
			static constexpr uSize lowGlyphTableSize = 256;
			Std::Array<GlyphData, lowGlyphTableSize> lowGlyphDatas;

			FontFace() = default;
			FontFace(FontFace const&) = delete;
			FontFace(FontFace&&) = default;
			FontFace& operator=(FontFace const&) = delete;
			FontFace& operator=(FontFace&&) = default;
		};

		struct FontFaceNode {
			u64 id;
			FontFace face;
		};

		std::vector<FontFaceNode> faceNodes;
	};

	u64 GetFontFaceId(
		TextManagerImpl const& implData,
		f32 scale,
		f32 dpiX,
		f32 dpiY)
	{
		return (u64)Math::Round(scale * 100);
	}

	Std::Opt<TextManagerImpl::GlyphData> LoadNewGlyph(
		TextManagerImpl& implData,
		FontFaceId fontFaceId,
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
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("Unable to load glyph");

		ftError = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("Unable to render glyph");

		if (face->glyph->bitmap.buffer != nullptr)
		{
			implData.gfxCtx->NewFontTexture(
				(Gfx::FontFaceId)fontFaceId,
				utfValue,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				face->glyph->bitmap.pitch,
				{
					(std::byte const*)face->glyph->bitmap.buffer,
					(uSize)face->glyph->bitmap.pitch * face->glyph->bitmap.rows });
		}

		TextManagerImpl::GlyphData newData{};
		newData.hasBitmap = face->glyph->bitmap.buffer != nullptr;
		newData.bitmapWidth = face->glyph->bitmap.width;
		newData.bitmapHeight = face->glyph->bitmap.rows;
		newData.advanceX = face->glyph->advance.x / 64;
		newData.posOffset = {
			(i32)face->glyph->metrics.horiBearingX / 64,
			(i32)-face->glyph->metrics.horiBearingY / 64 };
		return newData;
	}

	TextManagerImpl::FontFace LoadFontFace(
		TextManagerImpl& implData,
		FontFaceId fontFaceId,
		f32 scale,
		f32 dpiX,
		f32 dpiY)
	{
		TextManagerImpl::FontFace newFontFace = {};

		FT_Error ftError = FT_New_Memory_Face(
			implData.ftLib,
			(FT_Byte const*)implData.fontFileData.data(),
			(FT_Long)implData.fontFileData.size(),
			0,
			&newFontFace.ftFace);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("DEngine - Text Manager: Unable to load font");

		auto ftFace = newFontFace.ftFace;

		float sizeInSp = 24;
		sizeInSp *= scale;
		sizeInSp *= dpiX / 180.f;
		sizeInSp = Math::Round(sizeInSp);

		ftError = FT_Set_Pixel_Sizes(
			ftFace,
			(int)sizeInSp,
			(int)sizeInSp);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("DEngine - TextManager: Unable to set pixel sizes");

		/*
		// FreeType takes fixed-point floating point integers,
		// with 6 bits for decimal.
		// To convert our size from regular integer to this,
		// we multiply by 2^6 (64)
		ftError = FT_Set_Char_Size(
			ftFace,
			xSizeInPt * 64,
			ySizeInPt * 64,
			(u32)Math::Round(dpiX),
			(u32)Math::Round(dpiY));
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("DEngine - TextManager: Unable to set pixel sizes");
		 */

		newFontFace.lineheight = ftFace->size->metrics.height / 64;
		newFontFace.lineMinY = Math::Abs((i32)ftFace->bbox.yMin / 64);

		// Load all ASCII characters.
		for (int i = 1; i < newFontFace.lowGlyphDatas.Size(); i += 1) {
			auto glyphOpt = LoadNewGlyph(implData, fontFaceId, ftFace, (u32)i);
			if (glyphOpt.Has())
				newFontFace.lowGlyphDatas[i] = Std::Move(glyphOpt.Get());
		}
		return newFontFace;
	}

	TextManagerImpl::FontFace& GetFontFace(
		TextManagerImpl& implData,
		f32 inScale,
		f32 inDpiX,
		f32 inDpiY)
	{
		DENGINE_IMPL_GUI_ASSERT(inScale > 0.f);
		DENGINE_IMPL_GUI_ASSERT(inDpiX > 0.f);
		DENGINE_IMPL_GUI_ASSERT(inDpiY > 0.f);

		auto fontFaceId = GetFontFaceId(implData, inScale, inDpiX, inDpiY);

		// Then search for it
		auto nodeIt = Std::FindIf(
			implData.faceNodes.begin(),
			implData.faceNodes.end(),
			[fontFaceId](auto const& item) { return fontFaceId == item.id; });
		if (nodeIt != implData.faceNodes.end()) {
			return nodeIt->face;
		} else {
			// Load the new face
			TextManagerImpl::FontFaceNode newNode = {};
			newNode.id = fontFaceId;
			newNode.face = LoadFontFace(implData, (FontFaceId)fontFaceId, inScale, inDpiX, inDpiY);
			implData.faceNodes.emplace_back(Std::Move(newNode));

			implData.gfxCtx->NewFontFace((Gfx::FontFaceId)fontFaceId);

			return implData.faceNodes.back().face;
		}
	}

	TextManagerImpl::GlyphData const& GetGlyphData(
		TextManagerImpl& implData,
		FontFaceId fontFaceId,
		TextManagerImpl::FontFace& fontFace,
		u32 utfValue)
	{
		if (utfValue < fontFace.lowGlyphDatas.Size()) {
			// ASCII values are already loaded, so we don't need to check if it is.
			return fontFace.lowGlyphDatas[utfValue];
		}
		else
		{
			auto glyphDataIt = fontFace.glyphDatas.find(utfValue);
			if (glyphDataIt == fontFace.glyphDatas.end()) {
				auto glyphOpt = LoadNewGlyph(implData, fontFaceId, fontFace.ftFace, utfValue);
				if (!glyphOpt.Has())
					throw std::runtime_error("Unable to load glyph");

				glyphDataIt = fontFace.glyphDatas
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

	implData.gfxCtx = gfxCtx;
	implData.appCtx = appCtx;

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
}

namespace DEngine::Gui::impl
{
	template<class CastCallable>
	static Extent TextMan_GetOuterExtent2(
		TextManager& textManager,
		Std::Span<char const> str,
		f32 scale,
		f32 dpiX,
		f32 dpiY,
		CastCallable const* castCallable)
	{
		auto& implData = *static_cast<impl::TextManagerImpl*>(textManager.GetImplData());
		auto fontFaceId = impl::GetFontFaceId(implData, scale, dpiX, dpiY);
		auto& fontFace = impl::GetFontFace(implData, scale, dpiX, dpiY);

		Extent returnValue = {};
		returnValue.height = textManager.GetLineheight(scale, dpiX, dpiY);

		// Iterate over the string, find the bounding box width
		Math::Vec2Int penPos = {};
		penPos.y += (i32)fontFace.lineheight;
		penPos.y -= (i32)fontFace.lineMinY;
		int const strLength = (int)str.Size();
		for (int i = 0; i < strLength; i += 1)
		{
			auto const glyphChar = (u32)(unsigned char)str[i];

			DENGINE_IMPL_GUI_ASSERT(glyphChar != 0);

			auto const& glyphData = impl::GetGlyphData(
				implData,
				(FontFaceId)fontFaceId,
				fontFace,
				glyphChar);

			returnValue.width += glyphData.advanceX;

			if (castCallable)
			{
				Rect rect = {};
				if (glyphData.hasBitmap)
				{
					rect.position = penPos;
					rect.position += glyphData.posOffset;
					rect.extent.width = glyphData.bitmapWidth;
					rect.extent.height = glyphData.bitmapHeight;
				}
				(*castCallable)(i, rect);
				penPos.x += glyphData.advanceX;
			}
		}

		return returnValue;
	}

	[[nodiscard]] static Extent TextMan_GetOuterExtent(
		TextManager& textManager,
		Std::Span<char const> str,
		f32 scale,
		f32 dpiX,
		f32 dpiY,
		Rect* outRects)
	{
		auto lambda = [outRects](auto i, Rect const& x){ outRects[i] = x; };
		auto const* lambdaPtr = outRects != nullptr ? &lambda : nullptr;
		return TextMan_GetOuterExtent2(
			textManager,
			str,
			scale,
			dpiX,
			dpiY,
			lambdaPtr);
	}
}

Extent Gui::TextManager::GetOuterExtent(
	Std::Span<char const> str,
	f32 scale,
	f32 dpiX,
	f32 dpiY)
{
	return impl::TextMan_GetOuterExtent(*this, str, scale, dpiX, dpiY, nullptr);
}

Extent Gui::TextManager::GetOuterExtent(
	Std::Span<char const> str,
	f32 scale,
	f32 dpiX,
	f32 dpiY,
	Rect* outRects)
{
	return impl::TextMan_GetOuterExtent(*this, str, scale, dpiX, dpiY, outRects);
}

u32 TextManager::GetLineheight(f32 scale, f32 dpiX, f32 dpiY)
{
	using namespace impl;

	auto& implData = *static_cast<TextManagerImpl*>(GetImplData());

	auto& fontFace = GetFontFace(implData, scale, dpiX, dpiY);

	return fontFace.lineheight;
}

FontFaceId TextManager::GetFontFaceId(f32 scale, f32 dpiX, f32 dpiY) const
{
	using namespace impl;
	auto& implData = *static_cast<TextManagerImpl const*>(GetImplData());
	return (FontFaceId)impl::GetFontFaceId(implData, scale, dpiX, dpiY);
}

void Gui::TextManager::RenderText(
	Std::Span<char const> const& input,
	Math::Vec4 const& color,
	Rect const& widgetRect,
	f32 scale,
	f32 dpiX,
	f32 dpiY,
	DrawInfo& drawInfo)
{
	auto& implData = *static_cast<impl::TextManagerImpl*>(GetImplData());
	auto fontFaceId = impl::GetFontFaceId(implData, scale, dpiX, dpiY);
	auto& fontFace = impl::GetFontFace(implData, scale, dpiX, dpiY);

	Math::Vec2Int penPos = widgetRect.position;
	penPos.y += (i32)fontFace.lineheight;
	penPos.y -= (i32)fontFace.lineMinY;

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
	cmd.text.fontFaceId = (Gfx::FontFaceId)fontFaceId;
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
	impl::TextMan_GetOuterExtent2(
		*this,
		input,
		scale,
		dpiX,
		dpiY,
		&setRectLambda);

	drawInfo.drawCmds->push_back(cmd);
}
