#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Array.hpp>

#include <stdexcept>

// Definitely temporary
// Make an actual good interface instead
#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Application.hpp>

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

		FT_Library ftLib = {};
		FT_Face face = {};
		std::vector<u8> fontFileData;
		u32 lineheight = 0;
		u32 lineMinY = 0;
		struct GlyphData
		{
			bool hasBitmap = {};
			u32 bitmapWidth = {};
			u32 bitmapHeight = {};

			i32 advanceX = {};
			Math::Vec2Int posOffset = {};
		};
		// Stores glyph-data that is not in the ascii-table
		std::unordered_map<u32, GlyphData> glyphDatas;
		static constexpr uSize lowGlyphTableSize = 256;
		Std::Array<GlyphData, lowGlyphTableSize> lowGlyphDatas;
	};

	TextManagerImpl::GlyphData LoadNewGlyph(
		TextManager& manager,
		u32 utfValue)
	{
		auto& implData = *static_cast<TextManagerImpl*>(manager.GetImplData());

		if (utfValue == 0)
			return TextManagerImpl::GlyphData{};

		// Load glyph data
		FT_UInt glyphIndex = FT_Get_Char_Index(implData.face, utfValue);
		if (glyphIndex == 0) // 0 is an error index
			return TextManagerImpl::GlyphData{};

		FT_Error ftError = FT_Load_Glyph(
			implData.face,
			glyphIndex,
			FT_LOAD_DEFAULT);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("Unable to load glyph");

		ftError = FT_Render_Glyph(implData.face->glyph, FT_RENDER_MODE_NORMAL);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("Unable to render glyph");

		if (implData.face->glyph->bitmap.buffer != nullptr)
		{
			implData.gfxCtx->NewFontTexture(
				utfValue,
				implData.face->glyph->bitmap.width,
				implData.face->glyph->bitmap.rows,
				implData.face->glyph->bitmap.pitch,
				{
					(std::byte const*)implData.face->glyph->bitmap.buffer,
					(uSize)implData.face->glyph->bitmap.pitch * implData.face->glyph->bitmap.rows });
		}

		TextManagerImpl::GlyphData newData{};
		newData.hasBitmap = implData.face->glyph->bitmap.buffer != nullptr;
		newData.bitmapWidth = implData.face->glyph->bitmap.width;
		newData.bitmapHeight = implData.face->glyph->bitmap.rows;
		newData.advanceX = implData.face->glyph->advance.x / 64;
		newData.posOffset = {
			(i32)implData.face->glyph->metrics.horiBearingX / 64,
			(i32)-implData.face->glyph->metrics.horiBearingY / 64 };
		return newData;
	}

	TextManagerImpl::GlyphData const& GetGlyphData(
		TextManager& manager,
		u32 utfValue)
	{
		auto& implData = *static_cast<TextManagerImpl*>(manager.GetImplData());

		if (utfValue < implData.lowGlyphDatas.Size())
		{
			// ASCII values are already loaded, so don't need to check if it is.
			return implData.lowGlyphDatas[utfValue];
		}
		else
		{
			auto glyphDataIt = implData.glyphDatas.find(utfValue);
			if (glyphDataIt == implData.glyphDatas.end())
			{
				glyphDataIt = implData.glyphDatas.insert(std::pair{ utfValue, LoadNewGlyph(manager, utfValue) }).first;
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

	App::FileInputStream fontFile("data/gui/Roboto-Regular.ttf");
	if (!fontFile.IsOpen())
		throw std::runtime_error("DEngine - Editor: Unable to open font file.");
	fontFile.Seek(0, App::FileInputStream::SeekOrigin::End);
	u64 fileSize = fontFile.Tell().Value();
	implData.fontFileData.resize(fileSize);
	fontFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
	fontFile.Read((char*)implData.fontFileData.data(), fileSize);
	fontFile.Close();

	ftError = FT_New_Memory_Face(
		implData.ftLib,
		(FT_Byte const*)implData.fontFileData.data(),
		(FT_Long)fileSize,
		0,
		&implData.face);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Editor: Unable to load font");

	ftError = FT_Set_Char_Size(
		implData.face,    /* handle to face object           */
		0,       /* char_width in 1/64th of points  */
		72 * 64,   /* char_height in 1/64th of points */
		0,     /* horizontal device resolution    */
		0);   /* vertical device resolution      */
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Editor: Unable to set pixel sizes");

	implData.lineheight = implData.face->size->metrics.height / 64;
	implData.lineMinY = Math::Abs((i32)implData.face->bbox.yMin / 64);


	// Load all ASCII characters.
	for (uSize i = 0; i < implData.lowGlyphDatas.Size(); i += 1)
		implData.lowGlyphDatas[i] = impl::LoadNewGlyph(manager, (u32)i);
}

void Gui::TextManager::RenderText(
	Std::Span<char const> string,
	Math::Vec4 color,
	Rect widgetRect,
	DrawInfo& drawInfo)
{
	auto& implData = *static_cast<impl::TextManagerImpl*>(GetImplData());

	Math::Vec2Int penPos = widgetRect.position;
	penPos.y += implData.lineheight;
	penPos.y -= implData.lineMinY;

	for (uSize i = 0; i < string.Size(); i += 1)
	{
		u32 glyphChar = string[i];
		auto const& glyphData = impl::GetGlyphData(*this, glyphChar);
		if (glyphData.hasBitmap)
		{
			// This holds the top-left position of the glyph.
			Math::Vec2Int glyphPos = penPos;
			glyphPos += glyphData.posOffset;

			Gfx::GuiDrawCmd cmd = {};
			cmd.type = Gfx::GuiDrawCmd::Type::TextGlyph;
			cmd.textGlyph.utfValue = glyphChar;
			cmd.textGlyph.color = color;
			cmd.rectPosition.x = (f32)glyphPos.x / drawInfo.GetFramebufferExtent().width;
			cmd.rectPosition.y = (f32)glyphPos.y / drawInfo.GetFramebufferExtent().height;
			cmd.rectExtent.x = (f32)glyphData.bitmapWidth / drawInfo.GetFramebufferExtent().width;
			cmd.rectExtent.y = (f32)glyphData.bitmapHeight / drawInfo.GetFramebufferExtent().height;

			drawInfo.drawCmds->push_back(cmd);
		}

		penPos.x += glyphData.advanceX;
	}
}

namespace DEngine::Gui::impl
{
	[[nodiscard]] static Extent TextMan_GetOuterExtent(
		TextManager& textManager,
		Std::Span<char const> str,
		Math::Vec2Int pos,
		Rect* outRects)
	{
		Extent returnValue = {};
		returnValue.height = textManager.GetLineheight();

		auto& implData = *static_cast<impl::TextManagerImpl*>(textManager.GetImplData());

		// Iterate over the string, find the bounding box width
		Math::Vec2Int penPos = pos;
		penPos.y += (i32)implData.lineheight;
		penPos.y -= (i32)implData.lineMinY;
		int const strLength = (int)str.Size();
		for (int i = 0; i < strLength; i += 1)
		{
			auto const glyphChar = (u32)(unsigned char)str[i];

			DENGINE_IMPL_GUI_ASSERT(glyphChar != 0);

			auto& glyphData = impl::GetGlyphData(textManager, glyphChar);

			returnValue.width += glyphData.advanceX;

			if (outRects)
			{
				auto& rect = outRects[i];
				rect = {};
				if (glyphData.hasBitmap)
				{
					rect.position = penPos;
					rect.position += glyphData.posOffset;
					rect.extent.width = glyphData.bitmapWidth;
					rect.extent.height = glyphData.bitmapHeight;
				}
				penPos.x += glyphData.advanceX;
			}
		}

		return returnValue;
	}
}

Extent Gui::TextManager::GetOuterExtent(
	Std::Span<char const> str)
{
	return impl::TextMan_GetOuterExtent(*this, str, {}, nullptr);
}

Extent Gui::TextManager::GetOuterExtent(
	Std::Span<char const> str,
	Math::Vec2Int pos,
	Rect* outRects)
{
	return impl::TextMan_GetOuterExtent(*this, str, pos, outRects);
}

u32 TextManager::GetLineheight()
{
	auto& implData = *static_cast<impl::TextManagerImpl*>(GetImplData());

	return implData.lineheight;
}
