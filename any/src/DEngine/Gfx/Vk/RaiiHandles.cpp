#include "RaiiHandles.hpp"

#include "DynamicDispatch.hpp"

using namespace DEngine::Gfx;
using namespace DEngine::Gfx::Vk;

vk::Device Vk::impl::BoxVkHandle_DeviceHandle(DeviceDispatch const& in) {
	return in.Handle();
}

DeviceDispatchRaw const& Vk::impl::BoxVkHandle_DeviceDispatchTable(DeviceDispatch const& in) {
	return in.FnTable();
}