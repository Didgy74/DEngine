#pragma once

#include <DEngine/Platform/Platform.hpp>
#include "DEngine/Gfx/Gfx.hpp"

namespace DEngine::impl {
	class GfxLogger : public Gfx::LogInterface {
	public:
		Platform::Context* appCtx = nullptr;

		virtual void Log(Level level, Std::Span<char const> msg) override {
			appCtx->Log(Platform::LogSeverity::Error, msg);
		}

		virtual ~GfxLogger() override {
		}
	};
}
