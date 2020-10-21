#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Utility.hpp>

#include <vector>
#include <string_view>

using namespace DEngine;
using namespace DEngine::Gui;

// TEMPORARY INCLUDES
#include <DEngine/Application.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Text.hpp>

#include <stdexcept>

Context Context::Create(
	WindowHandler& windowHandler,
	Gfx::Context* gfxCtx)
{
	Context newCtx;
	newCtx.pImplData = new impl::ImplData;
	impl::ImplData& implData = *static_cast<impl::ImplData*>(newCtx.pImplData);
	
	implData.windowHandler = &windowHandler;

	impl::TextManager::Initialize(
		implData.textManager,
		gfxCtx);

	{
		// Add the test first window shit
		impl::WindowNode newNode{};
		newNode.id = WindowID(0);
		App::Extent appWindowExtent = App::GetWindowSize(App::WindowID(0));
		newNode.data.rect.extent = { appWindowExtent.width, appWindowExtent.height };
		newNode.data.rect.position = App::GetWindowPosition(App::WindowID(0));
		App::Extent appWindowVisibleExtent = App::GetWindowVisibleSize(App::WindowID(0));
		newNode.data.visibleRect.extent = { appWindowVisibleExtent.width, appWindowVisibleExtent.height };
		newNode.data.visibleRect.position = App::GetWindowVisiblePosition(App::WindowID(0));
		newNode.data.clearColor = { 0.1f, 0.1f, 0.1f, 1.f };

		StackLayout* outmostLayout = new StackLayout(StackLayout::Direction::Vertical);
		newNode.data.topLayout = Std::Box<Layout>{ outmostLayout };
		implData.windows.emplace_back(Std::Move(newNode));

		newCtx.outerLayout = outmostLayout;
	}

	return static_cast<Context&&>(newCtx);
}

Context::Context(Context&& other) noexcept :
	outerLayout(other.outerLayout),
	vertices(other.vertices),
	indices(other.indices),
	drawCmds(other.drawCmds),
	windowUpdates(other.windowUpdates),
	pImplData(other.pImplData)
{
	other.pImplData = nullptr;
	other.outerLayout = nullptr;
}

void Context::TakeInputConnection(
	Widget& widget,
	SoftInputFilter softInputFilter,
	std::string_view currentText)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	if (implData.inputConnectionWidget)
	{
		implData.inputConnectionWidget->InputConnectionLost(*this);
	}
	implData.windowHandler->OpenSoftInput(
		currentText,
		softInputFilter);
	implData.inputConnectionWidget = &widget;
}

void Context::ClearInputConnection(
	Widget& widget)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	DENGINE_IMPL_GUI_ASSERT(&widget == implData.inputConnectionWidget);

	implData.windowHandler->HideSoftInput();

	implData.inputConnectionWidget = nullptr;
}

WindowHandler& Context::GetWindowHandler() const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	return *implData.windowHandler;
}

void Context::Test(
	WindowID windowId, 
	Std::Box<Layout> layout, 
	Rect rect)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	
	auto windowNodeIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[windowId](decltype(implData.windows.front()) const& val) -> bool { return windowId == val.id; });
	DENGINE_IMPL_GUI_ASSERT(windowNodeIt != implData.windows.end());
	auto& windowNode = *windowNodeIt;

	impl::Test_Menu yo{};
	yo.rect = rect;
	yo.topLayout = static_cast<Std::Box<Layout>&&>(layout);
	windowNode.test_Menu = Std::Move(yo);
}

void Context::PushEvent(CharEnterEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.topLayout)
		{
			windowNode.data.topLayout->CharEnterEvent(*this);
		}
	}
}

void Context::PushEvent(CharEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.topLayout)
		{
			windowNode.data.topLayout->CharEvent(
				*this,
				event.utfValue);
		}
	}
}

void Context::PushEvent(CharRemoveEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.topLayout)
		{
			windowNode.data.topLayout->CharRemoveEvent(*this);
		}
	}
}

void Context::PushEvent(CursorClickEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.topLayout)
		{
			bool bleh = false;
			if (windowNode.test_Menu.HasValue() && windowNode.test_Menu.Value().topLayout)
			{
				auto& yo = windowNode.test_Menu.Value();
				SizeHint sizeHint = yo.topLayout->SizeHint(*this);
				Rect widgetRect = { yo.rect.position, sizeHint.preferred };
				Math::Vec2Int cursorPos = implData.cursorPosition - windowNode.data.rect.position;
				if (widgetRect.PointIsInside(cursorPos))
				{
					bleh = true;
					yo.topLayout->CursorClick(
						*this,
						windowNode.id,
						{ yo.rect.position, sizeHint.preferred },
						{ yo.rect.position, sizeHint.preferred },
						cursorPos,
						event);
				}
				else
				{
					windowNode.test_Menu = Std::nullOpt;
				}
			}

			if (!bleh)
			{
				windowNode.data.topLayout->CursorClick(
					*this,
					windowNode.id,
					{ windowNode.data.visibleRect.position, windowNode.data.visibleRect.extent },
					{ windowNode.data.visibleRect.position, windowNode.data.visibleRect.extent },
					implData.cursorPosition - windowNode.data.rect.position,
					event);
			}
		}
	}
}

void Context::PushEvent(CursorMoveEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	implData.cursorPosition = event.position;
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.topLayout)
		{
			CursorMoveEvent modifiedEvent{};
			modifiedEvent.position = implData.cursorPosition - windowNode.data.rect.position;
			modifiedEvent.positionDelta = event.positionDelta;

			bool bleh = false;
			if (windowNode.test_Menu.HasValue() && windowNode.test_Menu.Value().topLayout)
			{
				auto& yo = windowNode.test_Menu.Value();
				SizeHint sizeHint = yo.topLayout->SizeHint(*this);
				Rect widgetRect = { yo.rect.position, sizeHint.preferred };
				if (widgetRect.PointIsInside(modifiedEvent.position))
				{
					yo.topLayout->CursorMove(
						*this,
						windowNode.id,
						{ yo.rect.position, sizeHint.preferred },
						{ yo.rect.position, sizeHint.preferred },
						modifiedEvent);
					bleh = true;
				}
			}

			if (!bleh)
			{
				windowNode.data.topLayout->CursorMove(
					*this,
					windowNode.id,
					{ windowNode.data.visibleRect.position, windowNode.data.visibleRect.extent },
					{ windowNode.data.visibleRect.position, windowNode.data.visibleRect.extent },
					modifiedEvent);
			}
		}
	}
}

void Context::PushEvent(TouchEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.topLayout)
		{
			bool bleh = false;
			if (windowNode.test_Menu.HasValue() && windowNode.test_Menu.Value().topLayout)
			{
				auto& yo = windowNode.test_Menu.Value();
				SizeHint sizeHint = yo.topLayout->SizeHint(*this);
				Rect widgetRect = { yo.rect.position, sizeHint.preferred };
				Math::Vec2Int cursorPos = implData.cursorPosition - windowNode.data.rect.position;
				if (widgetRect.PointIsInside(cursorPos) && event.type == TouchEventType::Down)
				{
					bleh = true;
					yo.topLayout->TouchEvent(
						*this,
						windowNode.id,
						{ yo.rect.position, sizeHint.preferred },
						{ yo.rect.position, sizeHint.preferred },
						event);
				}
				else
				{
					windowNode.test_Menu = Std::nullOpt;
				}
			}

			windowNode.data.topLayout->TouchEvent(
				*this,
				windowNode.id,
				{ windowNode.data.visibleRect.position, windowNode.data.visibleRect.extent },
				{ windowNode.data.visibleRect.position, windowNode.data.visibleRect.extent },
				event);
		}
	}
}

void Context::PushEvent(WindowCloseEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	WindowID windowId = windowIt->id;
	implData.windows.erase(windowIt);
	implData.windowHandler->CloseWindow(windowId);
}

void Context::PushEvent(WindowCursorEnterEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
}

void Context::PushEvent(WindowMinimizeEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
	windowData.isMinimized = event.wasMinimized;
}

void Context::PushEvent(WindowMoveEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
	windowData.rect.position = event.position;
}

void Context::PushEvent(WindowResizeEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
	windowData.rect.extent = event.extent;
	windowData.visibleRect = event.visibleRect;
}

void Context::Render() const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);

	vertices.clear();
	indices.clear();
	drawCmds.clear();
	windowUpdates.clear();
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.isMinimized)
			continue;

		windowNode.data.drawCmdOffset = (u32)this->drawCmds.size();

		if (windowNode.data.topLayout)
		{
			DrawInfo drawInfo(
				windowNode.data.rect.extent,
				vertices,
				indices,
				drawCmds);

			windowNode.data.topLayout->Render(
				*this,
				windowNode.data.rect.extent,
				{ windowNode.data.visibleRect.position, windowNode.data.visibleRect.extent },
				{ windowNode.data.visibleRect.position, windowNode.data.visibleRect.extent },
				drawInfo);

			if (windowNode.test_Menu.HasValue() && windowNode.test_Menu.Value().topLayout)
			{
				auto& yo = windowNode.test_Menu.Value();
				SizeHint sizeHint = yo.topLayout->SizeHint(*this);
				yo.topLayout->Render(
					*this,
					windowNode.data.rect.extent,
					{ yo.rect.position, sizeHint.preferred },
					{ yo.rect.position, sizeHint.preferred },
					drawInfo);
			}
		}

		windowNode.data.drawCmdCount = (u32)this->drawCmds.size() - windowNode.data.drawCmdOffset;

		Gfx::NativeWindowUpdate newUpdate{};
		newUpdate.id = Gfx::NativeWindowID(windowNode.id);
		newUpdate.clearColor = windowNode.data.clearColor;
		newUpdate.drawCmdOffset = windowNode.data.drawCmdOffset;
		newUpdate.drawCmdCount = windowNode.data.drawCmdCount;
		auto windowEvents = App::GetWindowEvents(App::WindowID(windowNode.id));
		if (windowEvents.restore)
			newUpdate.event = Gfx::NativeWindowEvent::Restore;
		else if (windowEvents.resize)
			newUpdate.event = Gfx::NativeWindowEvent::Resize;

		this->windowUpdates.push_back(newUpdate);
	}
}

namespace DEngine::Gui::impl
{
	TextManager::GlyphData LoadNewGlyph(
		TextManager& manager,
		u32 utfValue)
	{
		if (utfValue == 0)
			return TextManager::GlyphData{};

		// Load glyph data
		FT_UInt glyphIndex = FT_Get_Char_Index(manager.face, utfValue);
		if (glyphIndex == 0) // 0 is an error index
			//throw std::runtime_error("Unable to load glyph index");
			return TextManager::GlyphData{};

		FT_Error ftError = FT_Load_Glyph(
			manager.face,
			glyphIndex,
			FT_LOAD_DEFAULT);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("Unable to load glyph");

		ftError = FT_Render_Glyph(manager.face->glyph, FT_RENDER_MODE_NORMAL);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("Unable to render glyph");

		if (manager.face->glyph->bitmap.buffer != nullptr)
		{
			manager.gfxCtx->NewFontTexture(
				utfValue,
				manager.face->glyph->bitmap.width,
				manager.face->glyph->bitmap.rows,
				manager.face->glyph->bitmap.pitch,
				{ (std::byte const*)manager.face->glyph->bitmap.buffer,
					(uSize)manager.face->glyph->bitmap.pitch * manager.face->glyph->bitmap.rows });
		}

		TextManager::GlyphData newData{};
		newData.hasBitmap = manager.face->glyph->bitmap.buffer != nullptr;
		newData.bitmapWidth = manager.face->glyph->bitmap.width;
		newData.bitmapHeight = manager.face->glyph->bitmap.rows;
		newData.advanceX = manager.face->glyph->advance.x / 64;
		newData.posOffset = Math::Vec2Int{ (i32)manager.face->glyph->metrics.horiBearingX / 64, (i32)-manager.face->glyph->metrics.horiBearingY / 64 };
		return newData;
	}

	TextManager::GlyphData const& GetGlyphData(
		TextManager& manager,
		u32 utfValue)
	{
		if (utfValue < manager.lowGlyphDatas.Size())
		{
			// ASCII values are already loaded, so don't need to check if it is.
			return manager.lowGlyphDatas[utfValue];
		}
		else
		{
			auto glyphDataIt = manager.glyphDatas.find(utfValue);
			if (glyphDataIt == manager.glyphDatas.end())
			{
				glyphDataIt = manager.glyphDatas.insert(std::pair{ utfValue, LoadNewGlyph(manager, utfValue) }).first;
			}

			return glyphDataIt->second;
		}
	}
}

void impl::TextManager::Initialize(
	TextManager& manager,
	Gfx::Context* gfxCtx)
{
	manager.gfxCtx = gfxCtx;
	FT_Error ftError = FT_Init_FreeType(&manager.ftLib);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Editor: Unable to initialize FreeType");

	App::FileInputStream fontFile("data/gui/arial.ttf");
	if (!fontFile.IsOpen())
		throw std::runtime_error("DEngine - Editor: Unable to open font file.");
	fontFile.Seek(0, App::FileInputStream::SeekOrigin::End);
	u64 fileSize = fontFile.Tell().Value();
	manager.fontFileData.resize(fileSize);
	fontFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
	fontFile.Read((char*)manager.fontFileData.data(), fileSize);
	fontFile.Close();

	ftError = FT_New_Memory_Face(
		manager.ftLib,
		(FT_Byte const*)manager.fontFileData.data(),
		(FT_Long)fileSize,
		0,
		&manager.face);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Editor: Unable to load font");

	ftError = FT_Set_Char_Size(
		manager.face,    /* handle to face object           */
		0,       /* char_width in 1/64th of points  */
		46 * 64,   /* char_height in 1/64th of points */
		0,     /* horizontal device resolution    */
		0);   /* vertical device resolution      */
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Editor: Unable to set pixel sizes");

	manager.lineheight = manager.face->size->metrics.height / 64;
	manager.lineMinY = Math::Abs((i32)manager.face->bbox.yMin / 64);


	// Load all ASCII characters.
	for (uSize i = 0; i < manager.lowGlyphDatas.Size(); i += 1)
		manager.lowGlyphDatas[i] = impl::LoadNewGlyph(manager, (u32)i);
}

void impl::TextManager::RenderText(
	TextManager& manager,
	std::string_view string,
	Math::Vec4 color,
	Rect widgetRect,
	DrawInfo& drawInfo)
{
	Math::Vec2Int penPos = widgetRect.position;
	penPos.y += manager.lineheight;
	penPos.y -= manager.lineMinY;

	for (uSize i = 0; i < string.size(); i += 1)
	{
		u32 glyphChar = string[i];
		GlyphData const& glyphData = GetGlyphData(manager, glyphChar);
		if (glyphData.hasBitmap)
		{
			// This holds the top-left position of the glyph.
			Math::Vec2Int glyphPos = penPos;
			glyphPos += glyphData.posOffset;

			Gfx::GuiDrawCmd cmd;
			cmd.type = Gfx::GuiDrawCmd::Type::TextGlyph;
			cmd.textGlyph.utfValue = glyphChar;
			cmd.textGlyph.color = color;
			cmd.rectPosition.x = (f32)glyphPos.x / drawInfo.GetFramebufferExtent().width;
			cmd.rectPosition.y = (f32)glyphPos.y / drawInfo.GetFramebufferExtent().height;
			cmd.rectExtent.x = (f32)glyphData.bitmapWidth / drawInfo.GetFramebufferExtent().width;
			cmd.rectExtent.y = (f32)glyphData.bitmapHeight / drawInfo.GetFramebufferExtent().height;

			drawInfo.drawCmds.push_back(cmd);
		}

		penPos.x += glyphData.advanceX;
	}
}

SizeHint impl::TextManager::GetSizeHint(
	TextManager& manager,
	std::string_view str)
{
	SizeHint returnVar{};
	returnVar.preferred.height = manager.lineheight;
	
	// Iterate over the string, find the bounding box width
	for (uSize i = 0; i < str.size(); i += 1)
	{
		u32 glyphChar = str[i];
		GlyphData const& glyphData = GetGlyphData(manager, glyphChar);
		returnVar.preferred.width += glyphData.advanceX;
	}

	return returnVar;
}