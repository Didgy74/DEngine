#include "DEngine/Gfx/Gfx.hpp"
#include "VkInterface.hpp"

#include "DEngine/Utility.hpp"

#include "DEngine/Gfx/Assert.hpp"

using namespace DEngine;

Cont::Opt<Gfx::Data> DEngine::Gfx::Initialize(const InitInfo& initInfo)
{
	Gfx::Data returnVal{};

	returnVal.iLog = initInfo.optional_iLog;
	returnVal.iWsi = initInfo.iWsi;

	if (initInfo.optional_iLog)
		initInfo.optional_iLog->log("Logger test");

	Vk::InitializeBackend(returnVal, initInfo, returnVal.apiDataBuffer);

	return Cont::Opt<Gfx::Data>(Util::move(returnVal));
}

void DEngine::Gfx::Data::Draw(Draw_Params const& params)
{
	Vk::Draw(*this, params, apiDataBuffer);
}

DEngine::Gfx::ViewportRef DEngine::Gfx::Data::NewViewport(u8 id)
{
	ViewportRef returnVal{};
	

	Vk::NewViewport(this->apiDataBuffer, returnVal.viewportID, returnVal.imguiTexID);

	return returnVal;
}
