#include <DEngine/Gui/LineEdit.hpp>

#include <DEngine/Gui/Context.hpp>
#include "ImplData.hpp"

#include <DEngine/Std/Utility.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

Gui::LineEdit::~LineEdit()
{
	if (this->inputConnectionCtx)
	{
		this->inputConnectionCtx->ClearInputConnection(*this);
		this->inputConnectionCtx = nullptr;
	}
}

bool LineEdit::CurrentlyBeingEdited() const
{
	return this->inputConnectionCtx;
}

SizeHint DEngine::Gui::LineEdit::GetSizeHint(Context const& ctx) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	return impl::TextManager::GetSizeHint(
		implData.textManager,
		text);
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
	cmd.filledMesh.color = { 0.0f, 0.0f, 0.0f, 0.3f };
	cmd.filledMesh.mesh = drawInfo.GetQuadMesh();
	cmd.rectPosition.x = (f32)widgetRect.position.x / framebufferExtent.width;
	cmd.rectPosition.y = (f32)widgetRect.position.y / framebufferExtent.height;
	cmd.rectExtent.x = (f32)widgetRect.extent.width / framebufferExtent.width;
	cmd.rectExtent.y = (f32)widgetRect.extent.height / framebufferExtent.height;

	drawInfo.drawCmds.push_back(cmd);

	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());
	impl::TextManager::RenderText(
		implData.textManager,
		this->text,
		{ 1.f, 1.f, 1.f, 1.f },
		widgetRect,
		drawInfo);
}

void LineEdit::CharEnterEvent(Context& ctx)
{
	if (inputConnectionCtx)
	{
		inputConnectionCtx = nullptr;
		ctx.ClearInputConnection(*this);
		if (text.empty())
		{
			text = "0";
			if (textChangedPfn)
				textChangedPfn(*this);
		}
	}
}

void LineEdit::CharEvent(Context& ctx, u32 charEvent)
{
	if (inputConnectionCtx)
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
			if (charEvent == '-' && text.length() == 0)
			{
				validChar = true;
				break;
			}
			if (charEvent == '.') // Check if we already have dot
			{
				bool alreadyHasDot = text.find('.') != std::string_view::npos;
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
			if (charEvent == '-' && text.length() == 0)
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
			text.push_back((u8)charEvent);
			auto const& string = text;
			if (string != "" && string != "-" && string != "." && string != "-.")
			{
				if (textChangedPfn)
					textChangedPfn(*this);
			}
		}
	}
}

void LineEdit::CharRemoveEvent(Context& ctx)
{
	if (inputConnectionCtx && !text.empty())
	{
		text.pop_back();
		auto const& string = text;
		if (string != "" && string != "-" && string != "." && string != "-.")
		{
			if (textChangedPfn)
				textChangedPfn(*this);
		}
	}
}

void LineEdit::InputConnectionLost(Context& ctx)
{
	if (this->inputConnectionCtx)
	{
		this->inputConnectionCtx->ClearInputConnection(*this);
		this->inputConnectionCtx = nullptr;
	}
}

void LineEdit::CursorClick(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Math::Vec2Int cursorPos,
	CursorClickEvent event)
{
	if (event.button == CursorButton::Left)
	{
		bool cursorIsInside = widgetRect.PointIsInside(cursorPos);

		if (cursorIsInside && event.clicked && inputConnectionCtx == nullptr)
		{
			SoftInputFilter filter{};
			if (this->type == Type::Float)
				filter = SoftInputFilter::Float;
			else if (this->type == Type::Integer)
				filter = SoftInputFilter::Integer;
			else if (this->type == Type::UnsignedInteger)
				filter = SoftInputFilter::UnsignedInteger;
			ctx.TakeInputConnection(
				*this,
				filter,
				text);
			this->inputConnectionCtx = &ctx;
		}
		else if (!cursorIsInside && event.clicked && inputConnectionCtx)
		{
			DENGINE_IMPL_GUI_ASSERT(this->inputConnectionCtx == &ctx);
			this->inputConnectionCtx = nullptr;
			ctx.ClearInputConnection(*this);
			if (text.empty())
			{
				text = "0";
				if (textChangedPfn)
					textChangedPfn(*this);
			}
		}
	}
}

void LineEdit::TouchEvent(
	Context& ctx,
	WindowID windowId,
	Rect widgetRect,
	Rect visibleRect,
	Gui::TouchEvent event)
{
	bool cursorIsInside = widgetRect.PointIsInside(event.position) && visibleRect.PointIsInside(event.position);

	if (event.id == 0 && event.type == TouchEventType::Down && cursorIsInside && !inputConnectionCtx)
	{
		SoftInputFilter filter{};
		if (this->type == Type::Float)
			filter = SoftInputFilter::Float;
		else if (this->type == Type::Integer)
			filter = SoftInputFilter::Integer;
		else if (this->type == Type::UnsignedInteger)
			filter = SoftInputFilter::UnsignedInteger;
		ctx.TakeInputConnection(
			*this,
			filter,
			text);
		this->inputConnectionCtx = &ctx;
	}

	if (event.id == 0 && event.type == TouchEventType::Down && !cursorIsInside && inputConnectionCtx)
	{
		if (text.empty())
		{
			text = "0";
			if (textChangedPfn)
				textChangedPfn(*this);
		}
		this->inputConnectionCtx = nullptr;
		ctx.ClearInputConnection(*this);
	}
}
