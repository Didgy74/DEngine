export module DEngine.Gui.SizeHint;

import DEngine.Gui.Utility;

export namespace DEngine::Gui {
	struct SizeHint {
		Extent minimum = {};
		bool expandX = {};
		bool expandY = {};
	};
}
