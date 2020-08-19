#include <DEngine/Gui/Text.hpp>

#include <DEngine/Gui/Context.hpp>

#include <DEngine/Gui/impl/ImplData.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

void Text::String_Set(char const* string)
{
	invalidated = true;
	text = string;
}

void Text::String_PushBack(u8 value)
{
	invalidated = true;
	text.push_back(value);
}

void Text::String_PopBack()
{
	invalidated = true;
	text.pop_back();
}

std::string_view Text::StringView() const
{
	return text;
}

Gui::SizeHint Text::SizeHint(
	Context const& ctx) const
{
	if (invalidated)
	{
		impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

		cachedSizeHint = impl::TextManager::GetSizeHint(
			implData.textManager,
			text);

		invalidated = false;
	}
	return cachedSizeHint;
}

SizeHint Text::SizeHint_Tick(Context const& ctx)
{
	return SizeHint(ctx);
}

void Text::Render(
	Context const& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
	Rect visibleRect,
	DrawInfo& drawInfo) const
{
	impl::ImplData& implData = *static_cast<impl::ImplData*>(ctx.Internal_ImplData());

	impl::TextManager::RenderText(
		implData.textManager,
		this->text,
		this->color,
		framebufferExtent,
		widgetRect,
		drawInfo);
}