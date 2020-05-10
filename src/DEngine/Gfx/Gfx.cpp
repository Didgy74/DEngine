#include "DEngine/Gfx/Gfx.hpp"
#include "APIDataBase.hpp"

#include "DEngine/Utility.hpp"
#include "Assert.hpp"

DEngine::Gfx::Data::Data(Data&& other) noexcept
{
	iLog = other.iLog;
	other.iLog = nullptr;
	iWsi = other.iWsi;
	other.iWsi = nullptr;
	texAssetInterface = other.texAssetInterface;
	other.texAssetInterface = nullptr;

	apiDataBuffer = other.apiDataBuffer;
	other.apiDataBuffer = nullptr;
}

DEngine::Gfx::Data::~Data()
{
	if (apiDataBuffer != nullptr)
		delete static_cast<APIDataBase*>(apiDataBuffer);
}

DEngine::Gfx::APIDataBase::~APIDataBase()
{

}

namespace DEngine::Gfx::Vk
{
	bool InitializeBackend(Data& gfxData, InitInfo const& initInfo, void*& apiDataBuffer);
}

DEngine::Std::Opt<DEngine::Gfx::Data> DEngine::Gfx::Initialize(const InitInfo& initInfo)
{
	Gfx::Data returnVal{};

	returnVal.iLog = initInfo.optional_iLog;
	returnVal.iWsi = initInfo.iWsi;
	returnVal.texAssetInterface = initInfo.texAssetInterface;

	if (initInfo.optional_iLog)
		initInfo.optional_iLog->log("Logger test");

	Vk::InitializeBackend(returnVal, initInfo, returnVal.apiDataBuffer);

	return Std::Opt<Gfx::Data>(Std::Move(returnVal));
}

void DEngine::Gfx::Data::Draw(DrawParams const& params)
{
	static_cast<APIDataBase*>(this->apiDataBuffer)->Draw(*this, params);
}

DEngine::Gfx::ViewportRef DEngine::Gfx::Data::NewViewport()
{
	ViewportRef returnVal{};
	
	static_cast<APIDataBase*>(this->apiDataBuffer)->NewViewport(returnVal.viewportID, returnVal.imguiTexID);

	return returnVal;
}

void DEngine::Gfx::Data::DeleteViewport(uSize viewportID)
{
	static_cast<APIDataBase*>(this->apiDataBuffer)->DeleteViewport(viewportID);
}
