#include <DEngine/Gui/Text.hpp>

#include <DEngine/Gui/Context.hpp>

#include <DEngine/Gui/impl/ImplData.hpp>

using namespace DEngine;
using namespace DEngine::Gui;

void Text::Render(
	Context& ctx,
	Extent framebufferExtent,
	Rect widgetRect,
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