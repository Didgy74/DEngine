#pragma once

#include "DEngine/FixedWidthTypes.hpp"

namespace DEngine::Time
{
	f32 Delta();
	u64 TickCount();

	void Initialize();
	void TickStart();
}