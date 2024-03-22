#pragma once

#include <DEngine/FixedWidthTypes.hpp>

#include <DEngine/Gui/Utility.hpp>

namespace DEngine::Gui
{
	struct SizeHint
	{
		Extent minimum = {};
		bool expandX = {};
		bool expandY = {};
	};
}