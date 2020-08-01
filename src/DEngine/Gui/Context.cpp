#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/impl/ImplData.hpp>

#include <DEngine/Containers/Box.hpp>
#include <DEngine/Utility.hpp>

#include <vector>

using namespace DEngine;
using namespace DEngine::Gui;

// TEMPORARY INCLUDES
#include <DEngine/Application.hpp>
#include <DEngine/Gui/StackLayout.hpp>
#include <DEngine/Gui/Text.hpp>

Context Context::Create(Gfx::Context* gfxCtx)
{
	Context newCtx;
	newCtx.pImplData = new impl::ImplData;
	impl::ImplData& implData = *static_cast<impl::ImplData*>(newCtx.pImplData);
	
	// Initialize text manager stuff.
	{
		implData.textManager.gfxCtx = gfxCtx;
		FT_Error ftError = FT_Init_FreeType(&implData.textManager.ftLib);
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("DEngine - Editor: Unable to initialize FreeType");

		App::FileInputStream fontFile("data/gui/arial.ttf");
		if (!fontFile.IsOpen())
			throw std::runtime_error("DEngine - Editor: Unable to open font file.");
		fontFile.Seek(0, App::FileInputStream::SeekOrigin::End);
		u64 fileSize = fontFile.Tell().Value();
		implData.textManager.fontFileData.resize(fileSize);
		fontFile.Seek(0, App::FileInputStream::SeekOrigin::Start);
		fontFile.Read((char*)implData.textManager.fontFileData.data(), fileSize);
		fontFile.Close();

		ftError = FT_New_Memory_Face(
			implData.textManager.ftLib,
			(FT_Byte const*)implData.textManager.fontFileData.data(),
			(FT_Long)fileSize,
			0,
			&implData.textManager.face);

		if (ftError != FT_Err_Ok)
			throw std::runtime_error("DEngine - Editor: Unable to load font");

		implData.textManager.dpi = 72;
		ftError = FT_Set_Char_Size(
			implData.textManager.face,    /* handle to face object           */
			0,       /* char_width in 1/64th of points  */
			24 * 64,   /* char_height in 1/64th of points */
			implData.textManager.dpi,     /* horizontal device resolution    */
			implData.textManager.dpi);   /* vertical device resolution      */
		if (ftError != FT_Err_Ok)
			throw std::runtime_error("DEngine - Editor: Unable to set pixel sizes");
	}
	// Text manager stuff end

	{
		// Add the test first window shit
		impl::WindowNode newNode{};
		newNode.id = WindowID(0);
		App::Extent appWindowExtent = App::GetWindowSize(App::WindowID(0));
		newNode.data.rect.extent = { appWindowExtent.width, appWindowExtent.height };
		newNode.data.rect.position = App::GetWindowPosition(App::WindowID(0));
		newNode.data.clearColor = { 0.1f, 0.1f, 0.1f, 1.f };

		StackLayout* outmostLayout = new StackLayout(StackLayout::Direction::Vertical);
		newNode.data.topLayout = outmostLayout;
		implData.windows.push_back(Std::Move(newNode));

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

void Context::PushEvent(CharEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Input;
	newEvent.input.type = impl::Event::Input::Type::Char;
	newEvent.input.charEvent = event;
	implData.eventQueue.push_back(newEvent);
}

void Context::PushEvent(CharRemoveEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Input;
	newEvent.input.type = impl::Event::Input::Type::CharRemove;
	newEvent.input.charRemove = event;
	implData.eventQueue.push_back(newEvent);
}

void Context::PushEvent(CursorClickEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Input;
	newEvent.input.type = impl::Event::Input::Type::CursorClick;
	newEvent.input.cursorClick = event;
	implData.eventQueue.push_back(newEvent);
}

void Context::PushEvent(CursorMoveEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Input;
	newEvent.input.type = impl::Event::Input::Type::CursorMove;
	newEvent.input.cursorMove = event;
	implData.eventQueue.push_back(newEvent);
}

void Context::PushEvent(TouchEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Input;
	newEvent.input.type = impl::Event::Input::Type::Touch;
	newEvent.input.touch = event;
	implData.eventQueue.push_back(newEvent);
}

void Context::PushEvent(WindowCursorEnterEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Window;
	newEvent.window.type = impl::Event::Window::Type::CursorEnter;
	newEvent.window.cursorEnter = event;
	implData.eventQueue.push_back(newEvent);
}

void Context::PushEvent(WindowMinimizeEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Window;
	newEvent.window.type = impl::Event::Window::Type::Minimize;
	newEvent.window.minimize = event;
	implData.eventQueue.push_back(newEvent);
}

void Context::PushEvent(WindowMoveEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Window;
	newEvent.window.type = impl::Event::Window::Type::Move;
	newEvent.window.move = event;
	implData.eventQueue.push_back(newEvent);
}

void Context::PushEvent(WindowResizeEvent event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	impl::Event newEvent{};
	newEvent.type = impl::Event::Type::Window;
	newEvent.window.type = impl::Event::Window::Type::Resize;
	newEvent.window.resize = event;
	implData.eventQueue.push_back(newEvent);
}

namespace DEngine::Gui::impl
{
	void DispatchEvent(ImplData& implData, WindowCursorEnterEvent resize)
	{
		auto windowIt = std::find_if(
			implData.windows.begin(),
			implData.windows.end(),
			[resize](WindowNode const& node) -> bool { return node.id == resize.windowId; });

		auto& windowData = windowIt->data;
	}

	void DispatchEvent(ImplData& implData, WindowMinimizeEvent minimize)
	{
		auto windowIt = std::find_if(
			implData.windows.begin(),
			implData.windows.end(),
			[minimize](WindowNode const& node) -> bool { return node.id == minimize.windowId; });

		auto& windowData = windowIt->data;
		windowData.isMinimized = minimize.wasMinimized;
	}

	void DispatchEvent(ImplData& implData, WindowMoveEvent move)
	{
		auto windowIt = std::find_if(
			implData.windows.begin(),
			implData.windows.end(),
			[move](WindowNode const& node) -> bool { return node.id == move.windowId; });

		auto& windowData = windowIt->data;
		windowData.rect.position = move.position;
	}

	void DispatchEvent(ImplData& implData, WindowResizeEvent resize)
	{
		auto windowIt = std::find_if(
			implData.windows.begin(),
			implData.windows.end(),
			[resize](WindowNode const& node) -> bool { return node.id == resize.windowId; });

		auto& windowData = windowIt->data;
		windowData.rect.extent = resize.extent;
	}

	void DispatchWindowEvents(ImplData& implData, Event::Window windowEvent)
	{
		switch (windowEvent.type)
		{
		case Event::Window::Type::CursorEnter:
			DispatchEvent(implData, windowEvent.cursorEnter);
			break;
		case Event::Window::Type::Minimize:
			DispatchEvent(implData, windowEvent.minimize);
			break;
		case Event::Window::Type::Move:
			DispatchEvent(implData, windowEvent.move);
			break;
		case Event::Window::Type::Resize:
			DispatchEvent(implData, windowEvent.resize);
			break;
		}
	}

	void DispatchEvent(Context& ctx, ImplData& implData, CharEvent event)
	{
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				windowNode.data.topLayout->CharEvent(
					ctx,
					event.utfValue);
			}
		}
	}

	void DispatchEvent(Context& ctx, ImplData& implData, CharRemoveEvent event)
	{
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				windowNode.data.topLayout->CharRemoveEvent(ctx);
			}
		}
	}

	void DispatchEvent(ImplData& implData, CursorClickEvent event)
	{
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				windowNode.data.topLayout->CursorClick(
					{ { 0, 0 }, windowNode.data.rect.extent },
					implData.cursorPosition - windowNode.data.rect.position,
					event);
			}
		}
	}

	void DispatchEvent(ImplData& implData, CursorMoveEvent event)
	{
		implData.cursorPosition = event.position;
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				CursorMoveEvent modifiedEvent{};
				modifiedEvent.position = implData.cursorPosition - windowNode.data.rect.position;
				modifiedEvent.positionDelta = event.positionDelta;

				windowNode.data.topLayout->CursorMove(
					{ { 0, 0 }, windowNode.data.rect.extent },
					modifiedEvent);
			}
		}
	}

	void DispatchEvent(ImplData& implData, TouchEvent event)
	{
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				windowNode.data.topLayout->TouchEvent(
					{ { 0, 0 }, windowNode.data.rect.extent },
					event);
			}
		}
	}

	void ProcessInputEvent(Context& ctx, ImplData& implData, Event::Input inputEvent)
	{
		switch (inputEvent.type)
		{
		case Event::Input::Type::Char:
			DispatchEvent(ctx, implData, inputEvent.charEvent);
			break;
		case Event::Input::Type::CharRemove:
			DispatchEvent(ctx, implData, inputEvent.charRemove);
			break;
		case Event::Input::Type::CursorClick:
			DispatchEvent(implData, inputEvent.cursorClick);
			break;
		case Event::Input::Type::CursorMove:
			DispatchEvent(implData, inputEvent.cursorMove);
			break;
		case Event::Input::Type::Touch:
			DispatchEvent(implData, inputEvent.touch);
			break;
		}
	}

	void ProcessEvents(Context& ctx, ImplData& implData)
	{
		for (auto const& event : implData.eventQueue)
		{
			switch (event.type)
			{
			case Event::Type::Window:
				DispatchWindowEvents(implData, event.window);
				break;
			case Event::Type::Input:
				ProcessInputEvent(ctx, implData, event.input);
				break;
			}
		}
		implData.eventQueue.clear();
	}
}

void Context::ProcessEvents()
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);

	impl::ProcessEvents(*this, implData);
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.topLayout)
		{
			windowNode.data.topLayout->Tick(
				*this,
				{ {}, windowNode.data.rect.extent });
		}
	}

	// Render stuff
	this->vertices.clear();
	this->indices.clear();
	this->drawCmds.clear();
	this->windowUpdates.clear();
	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.isMinimized)
			continue;

		windowNode.data.drawCmdOffset = (u32)this->drawCmds.size();

		if (windowNode.data.topLayout)
		{
			DrawInfo drawInfo(
				vertices,
				indices,
				drawCmds);
			windowNode.data.topLayout->Render(
				*this,
				windowNode.data.rect.extent,
				Gui::Rect{ { 0, 0 }, windowNode.data.rect.extent },
				drawInfo);
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

void impl::TextManager::RenderText(
	TextManager& manager,
	std::string_view string,
	Math::Vec4 color,
	Extent framebufferExtent,
	Rect widgetRect,
	DrawInfo const& drawInfo)
{
	Math::Vec2Int penPos = widgetRect.position;

	u32 linespace = manager.face->size->metrics.height / 72;

	penPos.y += linespace;

	for (uSize i = 0; i < string.size(); i++)
	{
		u32 glyphChar = string[i];

		auto glyphDataIt = manager.glyphDatas.find(glyphChar);
		if (glyphDataIt == manager.glyphDatas.end())
		{
			// Load glyph data
			FT_UInt glyphIndex = FT_Get_Char_Index(manager.face, glyphChar);
			if (glyphIndex == 0) // 0 is an error index
				throw std::runtime_error("Unable to load glyph index");

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
					glyphChar,
					manager.face->glyph->bitmap.width,
					manager.face->glyph->bitmap.rows,
					manager.face->glyph->bitmap.pitch,
					{ (std::byte const*)manager.face->glyph->bitmap.buffer,
						(uSize)manager.face->glyph->bitmap.pitch * manager.face->glyph->bitmap.rows });
			}

			GlyphData newData{};
			newData.hasBitmap = manager.face->glyph->bitmap.buffer != nullptr;
			newData.bitmapWidth = manager.face->glyph->bitmap.width;
			newData.bitmapHeight = manager.face->glyph->bitmap.rows;
			newData.advance = manager.face->glyph->metrics.horiAdvance;
			newData.horizBearingX = manager.face->glyph->metrics.horiBearingX;
			newData.horizBearingY = manager.face->glyph->metrics.horiBearingY;
			glyphDataIt = manager.glyphDatas.insert({ glyphChar, newData }).first;
		}

		GlyphData const& glyphData = glyphDataIt->second;

		if (glyphData.hasBitmap)
		{
			// This holds the top-left position of the glyph.
			Math::Vec2Int glyphPos = penPos;
			glyphPos.x += glyphData.horizBearingX / 64;
			glyphPos.y -= glyphData.horizBearingY / 64;

			Gfx::GuiDrawCmd cmd;
			cmd.type = Gfx::GuiDrawCmd::Type::TextGlyph;
			cmd.textGlyph.utfValue = glyphChar;
			cmd.textGlyph.color = color;
			cmd.rectPosition.x = (f32)glyphPos.x / framebufferExtent.width;
			cmd.rectPosition.y = (f32)glyphPos.y / framebufferExtent.height;
			cmd.rectExtent.x = (f32)glyphData.bitmapWidth / framebufferExtent.width;
			cmd.rectExtent.y = (f32)glyphData.bitmapHeight / framebufferExtent.height;

			drawInfo.drawCmds.push_back(cmd);
		}

		penPos.x += glyphData.advance / 64; // Advance is measured 1/64ths of a pixel.
	}
}