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

	DENGINE_GFX_ASSERT_MSG(Vk::GetAPIDataSize() <= returnVal.apiDataBuffer.size(), "Error. Buffer inside DEngine::Gfx::Data made for the API data is too small.");

	Vk::InitializeBackend(returnVal, initInfo, returnVal.apiDataBuffer.data());

	return Cont::Opt<Gfx::Data>(Util::move(returnVal));
}

void DEngine::Gfx::Draw(Data& data, float scale)
{
	Vk::Draw(data, data.apiDataBuffer.data(), scale);
}