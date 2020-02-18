#include "DEngine/Gfx/Gfx.hpp"
#include "DEngine/Gfx/VkInterface.hpp"

#include "DEngine/Utility.hpp"

#include "DEngine/Gfx/Assert.hpp"

using namespace DEngine;

Cont::Opt<Gfx::Data> DEngine::Gfx::Initialize(const InitInfo& initInfo)
{
	DENGINE_GFX_ASSERT(initInfo.createVkSurfaceUserData != nullptr);
	DENGINE_GFX_ASSERT(initInfo.createVkSurfacePFN != nullptr);

	Gfx::Data returnVal{};

	returnVal.iLog = initInfo.iLog;

	if (initInfo.iLog)
		initInfo.iLog->log("Logger test");

	Vk::InitializeBackend(returnVal, initInfo, returnVal.apiDataBuffer);

	return Cont::Opt<Gfx::Data>(Util::move(returnVal));
}

void DEngine::Gfx::Draw(Data& data)
{
	Vk::Draw(data, data.apiDataBuffer);

	// Resize event is handled, we reset it.
	data.resizeEvent = false;
	data.rebuildVkSurface = false;
}

DEngine::Gfx::ViewportRef DEngine::Gfx::Data::NewViewport()
{
	ViewportRef returnVal{};
	

	Vk::NewViewport(this->apiDataBuffer, returnVal.viewportID, returnVal.imguiTexID);

	return returnVal;
}
