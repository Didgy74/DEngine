#pragma once

#include <DEngine/Gui/Widget.hpp>

#include <string>

namespace DEngine::Gui::impl { struct MenuBtnImpl; }

namespace DEngine::Gui
{
	class MenuButton : public Widget
	{
	public:
		std::string title;
		u32 margin = 0;
	};
}