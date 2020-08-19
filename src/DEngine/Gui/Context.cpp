#include <DEngine/Gui/Context.hpp>
#include <DEngine/Gui/impl/ImplData.hpp>

#include <DEngine/Containers/Box.hpp>
#include <DEngine/Utility.hpp>

#include <vector>
#include <algorithm>

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
	static void DispatchEvent(ImplData& implData, WindowCursorEnterEvent resize)
	{
		auto windowIt = std::find_if(
			implData.windows.begin(),
			implData.windows.end(),
			[resize](WindowNode const& node) -> bool { return node.id == resize.windowId; });

		auto& windowData = windowIt->data;
	}

	static void DispatchEvent(ImplData& implData, WindowMinimizeEvent minimize)
	{
		auto windowIt = std::find_if(
			implData.windows.begin(),
			implData.windows.end(),
			[minimize](WindowNode const& node) -> bool { return node.id == minimize.windowId; });

		auto& windowData = windowIt->data;
		windowData.isMinimized = minimize.wasMinimized;
	}

	static void DispatchEvent(ImplData& implData, WindowMoveEvent move)
	{
		auto windowIt = std::find_if(
			implData.windows.begin(),
			implData.windows.end(),
			[move](WindowNode const& node) -> bool { return node.id == move.windowId; });

		auto& windowData = windowIt->data;
		windowData.rect.position = move.position;
	}

	static void DispatchEvent(ImplData& implData, WindowResizeEvent resize)
	{
		auto windowIt = std::find_if(
			implData.windows.begin(),
			implData.windows.end(),
			[resize](WindowNode const& node) -> bool { return node.id == resize.windowId; });

		auto& windowData = windowIt->data;
		windowData.rect.extent = resize.extent;
	}

	static void DispatchWindowEvents(ImplData& implData, Event::Window windowEvent)
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

	static void DispatchEvent(Context& ctx, ImplData& implData, CharEvent event)
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

	static void DispatchEvent(Context& ctx, ImplData& implData, CharRemoveEvent event)
	{
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				windowNode.data.topLayout->CharRemoveEvent(ctx);
			}
		}
	}

	static void DispatchEvent(Context& ctx, ImplData& implData, CursorClickEvent event)
	{
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				windowNode.data.topLayout->CursorClick(
					ctx,
					{ { 0, 0 }, windowNode.data.rect.extent },
					{ { 0, 0 }, windowNode.data.rect.extent },
					implData.cursorPosition - windowNode.data.rect.position,
					event);
			}
		}
	}

	static void DispatchEvent(Context& ctx, ImplData& implData, CursorMoveEvent event)
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
					ctx,
					{ { 0, 0 }, windowNode.data.rect.extent },
					{ { 0, 0 }, windowNode.data.rect.extent },
					modifiedEvent);
			}
		}
	}

	static void DispatchEvent(Context& ctx, ImplData& implData, TouchEvent event)
	{
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				windowNode.data.topLayout->TouchEvent(
					ctx,
					{ { 0, 0 }, windowNode.data.rect.extent },
					{ { 0, 0 }, windowNode.data.rect.extent },
					event);
			}
		}
	}

	static void ProcessInputEvent(Context& ctx, ImplData& implData, Event::Input inputEvent)
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
			DispatchEvent(ctx, implData, inputEvent.cursorClick);
			break;
		case Event::Input::Type::CursorMove:
			DispatchEvent(ctx, implData, inputEvent.cursorMove);
			break;
		case Event::Input::Type::Touch:
			DispatchEvent(ctx, implData, inputEvent.touch);
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

	void Tick(Context& ctx, ImplData& implData)
	{
		for (auto& windowNode : implData.windows)
		{
			if (windowNode.data.topLayout)
			{
				windowNode.data.topLayout->Tick(
					ctx,
					{ {}, windowNode.data.rect.extent },
					{ {}, windowNode.data.rect.extent });
			}
		}
	}
}

void Context::ProcessEvents()
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);

	impl::ProcessEvents(*this, implData);

	impl::Tick(*this, implData);

	// Render stuff
	Render();
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
				vertices,
				indices,
				drawCmds);
			windowNode.data.topLayout->Render(
				*this,
				windowNode.data.rect.extent,
				Gui::Rect{ { 0, 0 }, windowNode.data.rect.extent },
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

namespace DEngine::Gui::impl
{
	TextManager::GlyphData LoadNewGlyph(
		TextManager& manager,
		u32 utfValue)
	{
		if (utfValue == 0)
			return {};

		// Load glyph data
		FT_UInt glyphIndex = FT_Get_Char_Index(manager.face, utfValue);
		if (glyphIndex == 0) // 0 is an error index
			//throw std::runtime_error("Unable to load glyph index");
			return {};

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
				glyphDataIt = manager.glyphDatas.insert({ utfValue, LoadNewGlyph(manager, utfValue) }).first;
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
		32 * 64,   /* char_height in 1/64th of points */
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
	Extent framebufferExtent,
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
			cmd.rectPosition.x = (f32)glyphPos.x / framebufferExtent.width;
			cmd.rectPosition.y = (f32)glyphPos.y / framebufferExtent.height;
			cmd.rectExtent.x = (f32)glyphData.bitmapWidth / framebufferExtent.width;
			cmd.rectExtent.y = (f32)glyphData.bitmapHeight / framebufferExtent.height;

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
