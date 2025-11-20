#pragma once

import DEngine.FixedWidthTypes;

namespace DEngine::Time
{
	f32 Delta();
	u64 TickCount();

	void Initialize();
	void TickStart();
}