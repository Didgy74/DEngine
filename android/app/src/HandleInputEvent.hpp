#pragma once

#include "BackendData.hpp"

#include <android/input.h>

namespace DEngine::Application::impl {
	int looperCallback_InputEvent(int fd, int events, void* data);

	bool HandleInputEvent_Motion(
		Context::Impl& appData,
		BackendData& backendData,
		AInputEvent const* event);

	bool HandleInputEvent_Motion_Touch(
		Context::Impl& implData,
		BackendData& backendData,
		AInputEvent const* event);
}