#include "HandleCustomEvent.hpp"

#include "BackendData.hpp"
#include "HandleInputEvent.hpp"

#include <DEngine/impl/AppAssert.hpp>
#include <DEngine/Std/Utility.hpp>

#include <sys/eventfd.h>

namespace DEngine::Application::impl
{

	void ProcessCustomEvent(
		Context::Impl& implData,
		BackendData& backendData,
		Std::Opt<CustomEvent_CallbackFnT> const& customCallback)
	{
		auto lock = std::lock_guard{ backendData.customEventQueueLock };

		DENGINE_IMPL_APPLICATION_ASSERT(!backendData.customEventQueue2.empty());
		auto event = Std::Move(backendData.customEventQueue2.front());
		backendData.customEventQueue2.erase(backendData.customEventQueue2.begin());

		if (customCallback.Has()) {
			customCallback.Get().Invoke(event.type);
		}
		event.fn(implData, backendData);

		// If we ran out of events now, we can safely reset the underlying allocator
		// the event lambdas rely on.
		if (backendData.customEventQueue2.empty()) {
			backendData.queuedEvents_InnerBuffer.Reset(false);
		}
	}
}


using namespace DEngine;
using namespace DEngine::Application;
using namespace DEngine::Application::impl;

int Application::impl::looperCallback_CustomEvent(int fd, int events, void* data)
{
	DENGINE_IMPL_APPLICATION_ASSERT(events == ALOOPER_EVENT_INPUT);

	auto& pollSource = *(PollSource*)data;
	DENGINE_IMPL_APPLICATION_ASSERT(pollSource.implData);
	DENGINE_IMPL_APPLICATION_ASSERT(pollSource.backendData);

	auto& implData = *pollSource.implData;
	auto& backendData = *pollSource.backendData;

	ProcessCustomEvent(implData, backendData, pollSource.customEvent_CallbackFnOpt);

	eventfd_t value = 0;
	eventfd_read(fd, &value);

	return 1; // Returning 1 means we want to continue having this event registered.
}
