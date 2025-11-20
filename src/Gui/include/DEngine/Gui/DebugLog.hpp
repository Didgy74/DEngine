#pragma once

#include <format>
#include <functional>

import DEngine.Std.Span;

namespace DEngine::Gui::impl {
	void DebugLogInternal(Std::Span<char const> const& msg);
}

namespace DEngine::Gui {
	// Thread safe
	void SetDebugLogOutput(std::function<void(Std::Span<char const>)> &&);

	// Thread safe logger for debug operations.
	// Don't use in release builds.
	template<typename... Args>
	void DebugLog(const std::format_string<Args...> fmt, Args&&... args) {
		std::string msg = std::format(fmt, std::forward<Args>(args)...);

		impl::DebugLogInternal({ msg.data(), msg.size() });
	}
}