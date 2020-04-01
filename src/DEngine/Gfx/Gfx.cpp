#include "DEngine/Gfx/Gfx.hpp"
#include "APIDataBase.hpp"

#include "DEngine/Utility.hpp"
#include "DEngine/Gfx/Assert.hpp"

using namespace DEngine;

namespace DEngine::Gfx::Vk
{
	bool InitializeBackend(Data& gfxData, InitInfo const& initInfo, void*& apiDataBuffer);
}

Std::Opt<Gfx::Data> DEngine::Gfx::Initialize(const InitInfo& initInfo)
{
	Gfx::Data returnVal{};

	returnVal.iLog = initInfo.optional_iLog;
	returnVal.iWsi = initInfo.iWsi;

	if (initInfo.optional_iLog)
		initInfo.optional_iLog->log("Logger test");

	Vk::InitializeBackend(returnVal, initInfo, returnVal.apiDataBuffer);

	return Std::Opt<Gfx::Data>(Util::Move(returnVal));
}

void DEngine::Gfx::Data::Draw(Draw_Params const& params)
{
	reinterpret_cast<APIDataBase*>(this->apiDataBuffer)->Draw(*this, params);
}

DEngine::Gfx::ViewportRef DEngine::Gfx::Data::NewViewport()
{
	ViewportRef returnVal{};
	
	reinterpret_cast<APIDataBase*>(this->apiDataBuffer)->NewViewport(returnVal.viewportID, returnVal.imguiTexID);

	return returnVal;
}

void DEngine::Gfx::Data::DeleteViewport(uSize viewportID)
{
	reinterpret_cast<APIDataBase*>(this->apiDataBuffer)->DeleteViewport(viewportID);
}
