#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

#include <DEngine/Std/Containers/Box.hpp>
#include <DEngine/Std/Utility.hpp>

#include <DEngine/Gui/Layer.hpp>
#include <DEngine/Gui/Widget.hpp>
#include <DEngine/Gui/SizeHintCollection.hpp>

#include <DEngine/Application.hpp>

#include <vector>
#include <stdexcept>

using namespace DEngine;
using namespace DEngine::Gui;

namespace DEngine::Gui::impl
{
	void ImplData_PreDispatchStuff(ImplData& implData)
	{
		implData.transientAlloc.Reset();

		implData.sizeHintColl.Clear();
	}

	// All rects should be relative to window position, not visible rect
	void BuildSizeHintCollection(
		Context const& ctx,
		ImplData const& implData,
		SizeHintCollection& sizeHintColl,
		Std::FrameAllocator& transientAlloc)
	{
		sizeHintColl.Clear();

		for (auto& windowNode : implData.windows)
		{
			if (!windowNode.data.topLayout)
				continue;

			SizeHintCollection::Pusher sizeHintPusher{ sizeHintColl };
			Widget::GetSizeHint2_Params params = {
				.ctx = ctx,
				.transientAlloc = transientAlloc,
				.pusher = sizeHintPusher };

			auto& widget = *windowNode.data.topLayout;
			widget.GetSizeHint2(params);

			transientAlloc.Reset();
		}

		sizeHintColl.rects.resize(sizeHintColl.widgets.size());

		for (auto& windowNode : implData.windows)
		{
			if (!windowNode.data.topLayout)
				continue;

			SizeHintCollection::Pusher2 sizeHintPusher2 { sizeHintColl };

			//Widget::BuildChildRects_Params params{ *this, sizeHintPusher2, transientAlloc };
			Widget::BuildChildRects_Params params {
				.ctx = ctx,
				.builder = sizeHintPusher2,
				.transientAlloc = transientAlloc };

			auto const& windowRect = windowNode.data.rect;
			auto const& visibleOffset = windowNode.data.visibleOffset;
			auto const& visibleExtent = windowNode.data.visibleExtent;
			Rect const visibleRect = Rect::Intersection(
				{{}, windowRect.extent},
				{{(i32) visibleOffset.x, (i32) visibleOffset.y}, visibleExtent});

			auto& widget = *windowNode.data.topLayout;
			params.builder.PushRect(
				widget,
				{visibleRect, visibleRect});
			widget.BuildChildRects(
				params,
				visibleRect,
				visibleRect);

			transientAlloc.Reset();
		}
	}
}

Context Context::Create(
	WindowHandler& windowHandler,
	Gfx::Context* gfxCtx)
{
	Context newCtx;
	newCtx.pImplData = new impl::ImplData;
	auto& implData = *static_cast<impl::ImplData*>(newCtx.pImplData);
	
	implData.windowHandler = &windowHandler;

	impl::TextManager::Initialize(
		implData.textManager,
		gfxCtx);

	return static_cast<Context&&>(newCtx);
}

Context::Context(Context&& other) noexcept :
	pImplData(other.pImplData)
{
	other.pImplData = nullptr;
}

void Context::TakeInputConnection(
	Widget& widget,
	SoftInputFilter softInputFilter,
	Std::Span<char const> currentText)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	if (implData.inputConnectionWidget)
	{
		implData.inputConnectionWidget->InputConnectionLost();
	}
	implData.windowHandler->OpenSoftInput(
		{ currentText.Data(), currentText.Size() },
		softInputFilter);
	implData.inputConnectionWidget = &widget;
}

void Context::ClearInputConnection(
	Widget& widget)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	DENGINE_IMPL_GUI_ASSERT(&widget == implData.inputConnectionWidget);

	implData.windowHandler->HideSoftInput();

	implData.inputConnectionWidget = nullptr;
}

WindowHandler& Context::GetWindowHandler() const
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	return *implData.windowHandler;
}

void Context::SetFrontmostLayer(
	WindowID windowId,
	Std::Box<Layer>&& layer)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);

	auto const windowNodeIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[windowId](auto const& val) -> bool { return windowId == val.id; });
	DENGINE_IMPL_GUI_ASSERT(windowNodeIt != implData.windows.end());
	auto& windowNode = *windowNodeIt;

	windowNode.frontmostLayer = static_cast<Std::Box<Layer>&&>(layer);
}

void Context::PushEvent(CharEnterEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		if (!windowNode.data.topLayout)
			continue;

		windowNode.data.topLayout->CharEnterEvent(*this);
	}
}

void Context::PushEvent(CharEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		if (!windowNode.data.topLayout)
			continue;

		windowNode.data.topLayout->CharEvent(
			*this,
			event.utfValue);
	}
}

void Context::PushEvent(CharRemoveEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		if (!windowNode.data.topLayout)
			continue;

		windowNode.data.topLayout->CharRemoveEvent(*this);
	}
}

void Context::PushEvent(CursorPressEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	auto& windows = implData.windows;

	auto windowIt = Std::FindIf(
		windows.begin(),
		windows.end(),
		[&event](auto const& item) { return item.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != windows.end());
	auto& windowNode = *windowIt;

	implData.cursorWindowId = event.windowId;

	impl::ImplData_PreDispatchStuff(implData);
	impl::BuildSizeHintCollection(
		*this,
		implData,
		implData.sizeHintColl,
		implData.transientAlloc);

	if (windowNode.data.topLayout)
	{
		RectCollection rectCollection { implData.sizeHintColl };
		Widget::CursorPressParams params {
			.ctx = *this,
			.rectCollection = rectCollection };
		params.windowId = windowNode.id;
		params.cursorPos = implData.cursorPosition - windowNode.data.rect.position;
		params.event = event;

		auto& widget = *windowNode.data.topLayout;
		widget.CursorPress2(params);
	}
}

void Context::PushEvent(CursorMoveEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	auto& windows = implData.windows;

	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[&event](auto const& item) { return item.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowNode = *windowIt;

	impl::ImplData_PreDispatchStuff(implData);
	impl::BuildSizeHintCollection(
		*this,
		implData,
		implData.sizeHintColl,
		implData.transientAlloc);

	// Update the globally stored cursor position
	implData.cursorPosition = event.position + windowNode.data.rect.position;
	implData.cursorWindowId = event.windowId;

	if (windowNode.data.topLayout)
	{
		auto& widget = *windowNode.data.topLayout;

		RectCollection rectCollection { implData.sizeHintColl };

		auto modifiedEvent = event;
		modifiedEvent.position = implData.cursorPosition - windowNode.data.rect.position;

		Widget::CursorMoveParams params {
			.ctx = *this,
			.rectCollection = rectCollection };
		params.windowId = windowNode.id;
		params.event = modifiedEvent;

		widget.CursorMove(params, false);
	}
}

void Context::PushEvent(WindowCursorExitEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	auto& windowNodes = implData.windows;

	auto const windowNodeIt = Std::FindIf(
		windowNodes.begin(),
		windowNodes.end(),
		[event](auto const& node) { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowNodeIt != windowNodes.end());

	auto& windowNode = *windowNodeIt;

	if (implData.cursorWindowId.HasValue())
	{
		auto const prevWindowId = implData.cursorWindowId.Value();

		if (event.windowId == prevWindowId)
		{
			implData.cursorWindowId = Std::nullOpt;

			if (windowNode.data.topLayout)
			{
				auto& widget = *windowNode.data.topLayout;
				widget.CursorExit(*this);
			}
		}
	}
}

void Context::PushEvent(WindowFocusEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	auto& windowNodes = implData.windows;

	auto const windowNodeIt = Std::FindIf(
		windowNodes.begin(),
		windowNodes.end(),
		[event](auto const& node) { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowNodeIt != windowNodes.end());

	if (event.gainedFocus)
	{
		// Move our window to the front
		auto tempWindowNode = Std::Move(*windowNodeIt);
		windowNodes.erase(windowNodeIt);
		windowNodes.emplace(windowNodes.begin(), Std::Move(tempWindowNode));

		// Call window focus gained event on widget?
	}
}

void Context::PushEvent(WindowMoveEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	auto& windowNodes = implData.windows;

	auto const windowNodeIt = Std::FindIf(
		windowNodes.begin(),
		windowNodes.end(),
		[event](auto const& node) { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowNodeIt != windowNodes.end());

	auto& windowNode = *windowNodeIt;
	windowNode.data.rect.position = event.position;
}

void Context::Render2(Render2_Params const& params) const
{
	auto const& implData = *static_cast<impl::ImplData const*>(pImplData);
	auto& sizeHintColl = params.sizeHintCollection;
	auto& transientAlloc = params.transientAlloc;

	impl::BuildSizeHintCollection(
		*this,
		implData,
		sizeHintColl,
		transientAlloc);

	for (auto const& windowNode : implData.windows)
	{
		if (windowNode.data.isMinimized || !windowNode.data.topLayout)
			continue;
		auto& widget = *windowNode.data.topLayout;

		auto const& windowRect = windowNode.data.rect;
		auto const& visibleOffset = windowNode.data.visibleOffset;
		auto const& visibleExtent = windowNode.data.visibleExtent;
		Rect const visibleRect = Rect::Intersection(
			{ {}, windowRect.extent },
			{ { (i32)visibleOffset.x, (i32)visibleOffset.y }, visibleExtent });

		DrawInfo drawInfo = {
			windowNode.data.rect.extent,
			params.vertices,
			params.indices,
			params.drawCmds };

		RectCollection const rectCollection { sizeHintColl };

		Widget::Render_Params const renderParams {
			.ctx = *this,
			.rectCollection = rectCollection,
			.framebufferExtent = windowNode.data.rect.extent,
			.drawInfo = drawInfo };

		u32 const drawCmdOffset = params.drawCmds.size();

		widget.Render2(
			renderParams,
			visibleRect,
			visibleRect);

		u32 const drawCmdCount = params.drawCmds.size() - drawCmdOffset;

		Gfx::NativeWindowUpdate newUpdate = {};
		newUpdate.id = (Gfx::NativeWindowID)windowNode.id;
		newUpdate.clearColor = windowNode.data.clearColor;
		newUpdate.drawCmdOffset = drawCmdOffset;
		newUpdate.drawCmdCount = drawCmdCount;

		params.windowUpdates.push_back(newUpdate);
	}
}

void Context::AdoptWindow(
	WindowID id,
	Math::Vec4 clearColor,
	Rect rect,
	Math::Vec2UInt visibleOffset,
	Extent visibleExtent,
	Std::Box<Widget>&& widget)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);

	impl::WindowNode newNode = {};
	newNode.id = id;
	newNode.data.rect = rect;
	newNode.data.visibleOffset = visibleOffset;
	newNode.data.visibleExtent = visibleExtent;
	newNode.data.clearColor = clearColor;
	newNode.data.topLayout = Std::Move(widget);

	implData.windows.emplace(implData.windows.begin(), Std::Move(newNode));
}

/*
void Context::PushEvent(TouchMoveEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	for (auto& windowNode : implData.windows)
	{
		auto modifiedEvent = event;
		modifiedEvent.position -= Math::Vec2{
			(f32)windowNode.data.rect.position.x,
			(f32)windowNode.data.rect.position.y };

		auto const& rect = windowNode.data.rect;
		Math::Vec2Int visiblePos = {
			rect.position.x + (i32)windowNode.data.visibleOffset.x,
			rect.position.y + (i32)windowNode.data.visibleOffset.y };
		Rect const visibleRect = { visiblePos, windowNode.data.visibleExtent };

		bool touchOccluded = false;

		if (windowNode.frontmostLayer)
		{
			auto& layer = *windowNode.frontmostLayer;
			bool const temp = layer.TouchMove(
				*this,
				visibleRect,
				visibleRect,
				modifiedEvent,
				touchOccluded);

			touchOccluded = temp;
		}

		if (!windowNode.data.topLayout)
			continue;
		
		windowNode.data.topLayout->TouchMoveEvent(
			*this,
			windowNode.id,
			visibleRect,
			visibleRect,
			event,
			touchOccluded);
	}
}
*/

/*
void Context::PushEvent(TouchPressEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);

	for (auto& windowNode : implData.windows)
	{
		auto modifiedEvent = event;
		modifiedEvent.position -= Math::Vec2{
			(f32)windowNode.data.rect.position.x,
			(f32)windowNode.data.rect.position.y };

		auto const& visibleOffset = windowNode.data.visibleOffset;
		Math::Vec2Int const visiblePos =
			windowNode.data.rect.position +
			Math::Vec2Int{ (i32)visibleOffset.x, (i32)visibleOffset.y };

		Rect const windowRect = { {}, windowNode.data.rect.extent };
		auto visibleRect = windowNode.data.visibleRect;
		visibleRect.position -= windowNode.data.rect.position;

		bool eventConsumed = false;
		bool destroyMenu = false;

		if (windowNode.frontmostLayer)
		{
			auto& layer = *windowNode.frontmostLayer;
			auto const temp = layer.TouchPress(
				*this,
				windowRect,
				visibleRect,
				modifiedEvent);

			eventConsumed = temp.eventConsumed;
			destroyMenu = temp.destroy;
		}
		if (destroyMenu)
			windowNode.frontmostLayer = nullptr;

		if (!windowNode.data.topLayout || (eventConsumed && modifiedEvent.pressed))
			continue;

		windowNode.data.topLayout->TouchPressEvent(
			*this,
			windowNode.id,
			visibleRect,
			visibleRect,
			modifiedEvent);
	}
}

void Context::PushEvent(WindowCloseEvent const& event)
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	WindowID windowId = windowIt->id;
	implData.windows.erase(windowIt);
	implData.windowHandler->CloseWindow(windowId);
}

void Context::PushEvent(WindowCursorEnterEvent const& event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
}

void Context::PushEvent(WindowMinimizeEvent const& event)
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

void Context::PushEvent(WindowMoveEvent const& event)
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(pImplData);
	auto windowIt = Std::FindIf(
		implData.windows.begin(),
		implData.windows.end(),
		[event](auto const& node) -> bool { return node.id == event.windowId; });
	DENGINE_IMPL_GUI_ASSERT(windowIt != implData.windows.end());
	auto& windowData = windowIt->data;
	windowData.rect.position = event.position;
	windowData.visibleRect.position = windowData.rect.position;
}

void Context::PushEvent(WindowResizeEvent const& event)
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

void Context::Render(
	std::vector<Gfx::GuiVertex>& vertices,
	std::vector<u32>& indices,
	std::vector<Gfx::GuiDrawCmd>& drawCmds,
	std::vector<Gfx::NativeWindowUpdate>& windowUpdates) const
{
	auto& implData = *static_cast<impl::ImplData*>(pImplData);

	for (auto& windowNode : implData.windows)
	{
		if (windowNode.data.isMinimized)
			continue;

		windowNode.data.drawCmdOffset = (u32)drawCmds.size();

		Rect const windowRect = { {}, windowNode.data.rect.extent };
		auto visibleRect = windowNode.data.visibleRect;
		visibleRect.position -= windowNode.data.rect.position;

		DrawInfo drawInfo = {
			windowNode.data.rect.extent,
			vertices,
			indices,
			drawCmds };

		if (windowNode.data.topLayout)
		{
			windowNode.data.topLayout->Render(
				*this,
				windowNode.data.rect.extent,
				visibleRect,
				visibleRect,
				drawInfo);
		}

		if (windowNode.frontmostLayer)
		{
			auto const& layer = *windowNode.frontmostLayer;
			layer.Render(
				*this,
				windowRect,
				visibleRect,
				drawInfo);
		}

		windowNode.data.drawCmdCount = (u32)drawCmds.size() - windowNode.data.drawCmdOffset;

		Gfx::NativeWindowUpdate newUpdate = {};
		newUpdate.id = Gfx::NativeWindowID(windowNode.id);
		newUpdate.clearColor = windowNode.data.clearColor;
		newUpdate.drawCmdOffset = windowNode.data.drawCmdOffset;
		newUpdate.drawCmdCount = windowNode.data.drawCmdCount;
		auto windowEvents = App::GetWindowEvents(App::WindowID(windowNode.id));
		if (windowEvents.restore)
			newUpdate.event = Gfx::NativeWindowEvent::Restore;
		else if (windowEvents.resize)
			newUpdate.event = Gfx::NativeWindowEvent::Resize;

		windowUpdates.push_back(newUpdate);
	}
}
*/

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

void Gui::impl::TextManager::Initialize(
	TextManager& manager,
	Gfx::Context* gfxCtx)
{
	manager.gfxCtx = gfxCtx;
	FT_Error ftError = FT_Init_FreeType(&manager.ftLib);
	if (ftError != FT_Err_Ok)
		throw std::runtime_error("DEngine - Editor: Unable to initialize FreeType");

	App::FileInputStream fontFile("data/gui/Roboto-Regular.ttf");
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
		52 * 64,   /* char_height in 1/64th of points */
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

void Gui::impl::TextManager::RenderText(
	TextManager& manager,
	Std::Span<char const> string,
	Math::Vec4 color,
	Rect widgetRect,
	DrawInfo& drawInfo)
{
	Math::Vec2Int penPos = widgetRect.position;
	penPos.y += manager.lineheight;
	penPos.y -= manager.lineMinY;

	for (uSize i = 0; i < string.Size(); i += 1)
	{
		u32 glyphChar = string[i];
		GlyphData const& glyphData = GetGlyphData(manager, glyphChar);
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

SizeHint Gui::impl::TextManager::GetSizeHint(
	TextManager& manager,
	Std::Span<char const> str)
{
	SizeHint returnVar = {};
	returnVar.preferred.height = manager.lineheight;
	
	// Iterate over the string, find the bounding box width
	for (uSize i = 0; i < str.Size(); i += 1)
	{
		u32 glyphChar = str[i];
		GlyphData const& glyphData = GetGlyphData(manager, glyphChar);
		returnVar.preferred.width += glyphData.advanceX;
	}

	return returnVar;
}