module;

#include <DEngine/Gui/impl/Assert.hpp>

export module DEngine.Gui.WindowInsets;

import DEngine.FixedWidthTypes;
import DEngine.Gui.Utility;
import DEngine.Std.Array;
import DEngine.Std.Common;

export namespace DEngine::Gui {
	enum class WindowInsetSource {
		SystemBar,
		TextInputSystem,
		COUNT, };

	struct WindowInsetValues {
		u32 left;
		u32 right;
		u32 top;
		u32 bottom;

		[[nodiscard]] Rect ApplyToRect(Rect const& rect) const noexcept {
			if (this->bottom == 0)
				return rect;
			if (this->left + this->right >= rect.extent.width)
				return {};
			if (this->top + this->bottom >= rect.extent.height)
				return {};

			return Rect {
				.position = {
					.x = (i32)((i64)rect.position.x + (i64)this->left),
					.y = (i32)((i64)rect.position.y + (i64)this->top) },
				.extent = {
					.width = rect.extent.width - this->left - this->right,
					.height = rect.extent.height - this->top - this->bottom, }, };
		}
	};

	struct WindowInsets {
		Std::Array<WindowInsetValues, (int)WindowInsetSource::COUNT> m_insets = {};

		[[nodiscard]] WindowInsetValues GetSourceInsets(WindowInsetSource source) const noexcept {
			DENGINE_IMPL_GUI_ASSERT((int)source < m_insets.Size());
			return m_insets[(int)source];
		}

		[[nodiscard]] WindowInsetValues All() const noexcept {
			WindowInsetValues ret = {};
			for (int i = 0; i < m_insets.Size(); i++) {
				auto const& other = m_insets[i];
				ret.bottom = Std::Max(ret.bottom, other.bottom);
				ret.left = Std::Max(ret.left, other.left);
				ret.top = Std::Max(ret.top, other.top);
				ret.right = Std::Max(ret.right, other.right);
			}
			return ret;
		}

	};
}
