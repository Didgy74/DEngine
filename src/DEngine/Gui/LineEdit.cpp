#include <DEngine/Gui/LineEdit.hpp>
#include <DEngine/Application.hpp>

#include <DEngine/Utility.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

Gui::LineEdit::~LineEdit()
{
}

void LineEdit::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	Gfx::GuiDrawCmd cmd{};
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
		visibleRect,
		drawInfo);
}

void LineEdit::CharEnterEvent(Context& ctx)
{
	if (selected)
	{
		selected = false;
		App::HideSoftInput();
		if (StringView().empty())
		{
			String_Set("0");
			App::UpdateCharInputContext(StringView()); // TEMPORARY!
			if (textChangedPfn)
				textChangedPfn(*this);
		}
	}
}

void LineEdit::CharEvent(Context& ctx, u32 charEvent)
{
	if (selected)
	{
		bool validChar = false;
		switch (this->type)
		{
		case Type::Float:
			if ('0' <= charEvent && charEvent <= '9')
			{
				validChar = true;
				break;
			}
			if (charEvent == '-' && StringView().length() == 0)
			{
				validChar = true;
				break;
			}
			if (charEvent == '.') // Check if we already have dot
			{
				bool alreadyHasDot = StringView().find('.') != std::string_view::npos;
				if (!alreadyHasDot)
				{
					validChar = true;
					break;
				}
			}
			break;
		case Type::Integer:
			if ('0' <= charEvent && charEvent <= '9')
			{
				validChar = true;
				break;
			}
			if (charEvent == '-' && StringView().length() == 0)
			{
				validChar = true;
				break;
			}
			break;
		case Type::UnsignedInteger:
			if ('0' <= charEvent && charEvent <= '9')
			{
				validChar = true;
				break;
			}
			break;
		}
		if (validChar)
		{
			String_PushBack((u8)charEvent);
			App::UpdateCharInputContext(StringView()); // TEMPORARY!
			auto const& string = StringView();
			if (string != "" && string != "-" && string != "." && string != "-.")
			{
				if (textChangedPfn)
					textChangedPfn(*this);
				App::UpdateCharInputContext(StringView());
			}
		}
	}
}

void LineEdit::CharRemoveEvent(Context& ctx)
{
	if (selected && !StringView().empty())
	{
		String_PopBack();
		App::UpdateCharInputContext(StringView()); // TEMPORARY!
		auto const& string = StringView();
		if (string != "" && string != "-" && string != "." && string != "-.")
		{
			if (textChangedPfn)
				textChangedPfn(*this);
		}
	}
}

void LineEdit::CursorClick(
	Context& ctx,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	if (event.button == CursorButton::Left)
	{
		bool cursorIsInside = widgetRect.PointIsInside(cursorPos);

		if (cursorIsInside && event.clicked && !selected)
		{
			selected = true;
		}
		else if (!cursorIsInside && event.clicked && selected)
		{
			selected = false;
			if (StringView().empty())
			{
				String_Set("0");
				if (textChangedPfn)
					textChangedPfn(*this);
			}
		}
	}
}

void LineEdit::TouchEvent(
	Context& ctx,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event)
{
	bool cursorIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);

	if (event.id == 0 && event.type == TouchEventType::Down && cursorIsInside && !selected)
	{
		selected = true;
		// TEMP
		WindowHandler* test = nullptr;
		App::SoftInputFilter filter{};
		if (this->type == Type::Float)
			filter = App::SoftInputFilter::Float;
		else if (this->type == Type::Integer)
			filter = App::SoftInputFilter::Integer;
		else if (this->type == Type::UnsignedInteger)
			filter = App::SoftInputFilter::UnsignedInteger;
		App::OpenSoftInput(this->StringView(), filter);
	}

	if (event.id == 0 && event.type == TouchEventType::Down && !cursorIsInside && selected)
	{
		App::HideSoftInput();
		if (StringView().empty())
		{
			String_Set("0");
			if (textChangedPfn)
				textChangedPfn(*this);
		}
		selected = false;
	}
}
