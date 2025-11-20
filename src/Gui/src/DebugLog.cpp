#include <DEngine/Gui/DebugLog.hpp>
#include <functional>

#include <mutex>

static std::mutex delegateMutex;
static std::function<void(DEngine::Std::Span<char const>)> debugLogOutput;

void DEngine::Gui::SetDebugLogOutput(std::function<void(Std::Span<char const>)> &&newOutput) {
	std::lock_guard lock(delegateMutex);
	debugLogOutput = std::move(newOutput);
}

void DEngine::Gui::impl::DebugLogInternal(Std::Span<char const> const& msg)
{
	std::lock_guard lock(delegateMutex);
	if (debugLogOutput) {
		debugLogOutput(msg);
	}
}
