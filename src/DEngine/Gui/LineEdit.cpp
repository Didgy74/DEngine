#include <DEngine/Gui/LineEdit.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

void LineEdit::Render(
	Context& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	DrawInfo& drawInfo) const
{
	Gfx::GuiDrawCmd cmd;
	cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
	cmd.filledMesh.color = { 0.25f, 0.25f, 0.25f, 1.f };
	cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
	cmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
	cmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
	cmd.rectExtent.x = (f32)widgetRect.extent.width / framebufferExtent.width;
	cmd.rectExtent.y = (f32)widgetRect.extent.height / framebufferExtent.height;

	drawInfo.drawCmds.push_back(cmd);

	Text::Render(
		ctx,
		framebufferExtent,
		widgetRect,
		drawInfo);
}

void LineEdit::CharEvent(Context& ctx, u32 charEvent)
{
	if (selected)
	{
		if (type == Type::Float)
		{
			bool hasPeriod = false;
			for (auto item : text)
			{
				if (item == '.')
				{
					hasPeriod = true;
					break;
				}
			}
			if ((48 <= charEvent && charEvent <= 58) || (!hasPeriod && charEvent == '.'))
			{
				this->text.push_back((u8)charEvent);
				if (textChangedPfn)
					textChangedPfn(*this);
			}
			else if (charEvent == '-' && text.empty())
			{
				this->text.push_back((u8)charEvent);
				if (textChangedPfn)
					textChangedPfn(*this);
			}

		}
		else if (type == Type::Integer)
		{
			if (48 <= charEvent && charEvent <= 58)
			{
				this->text.push_back((u8)charEvent);
				if (textChangedPfn)
					textChangedPfn(*this);
			}
			else if (charEvent == '-' && text.empty())
			{
				this->text.push_back((u8)charEvent);
				if (textChangedPfn)
					textChangedPfn(*this);
			}
		}
	}
}

void LineEdit::CharRemoveEvent(Context& ctx)
{
	if (selected && !text.empty())
	{
		text.pop_back();
		if (textChangedPfn)
			textChangedPfn(*this);
	}
}

void LineEdit::CursorClick(
	Rect widgetRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	if (event.button == CursorButton::Left)
	{
		bool cursorIsInside = widgetRect.PointIsInside(cursorPos);

		if (cursorIsInside && event.clicked && !selected)
		{
			selected = true;
			//text.clear();
		}
		else if (!cursorIsInside && event.clicked && selected)
		{
			selected = false;
			if (text.empty())
			{
				text = '0';
				if (textChangedPfn)
					textChangedPfn(*this);
			}
		}
	}
}

#include <DEngine/Application.hpp>

void LineEdit::TouchEvent(
	Rect widgetRect,
	Gui::TouchEvent touch)
{
	bool cursorIsInside = widgetRect.PointIsInside(touch.position);

	if (touch.id == 0 && touch.type == TouchEventType::Down && cursorIsInside && !selected)
	{
		selected = true;
		//text.clear();
		App::OpenSoftInput();
	}
}
