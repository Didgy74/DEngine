#include <DEngine/Gfx/Gfx.hpp>
#include "APIDataBase.hpp"

#include <DEngine/Utility.hpp>
#include <DEngine/Gfx/detail/Assert.hpp>

using namespace DEngine;

Gfx::Context::Context(Context&& other) noexcept
{
	logger = other.logger;
	other.logger = nullptr;

	texAssetInterface = other.texAssetInterface;
	other.texAssetInterface = nullptr;

	apiDataBuffer = other.apiDataBuffer;
	other.apiDataBuffer = nullptr;
}

Gfx::Context::~Context()
{
	delete static_cast<APIDataBase*>(apiDataBuffer);
}

namespace DEngine::Gfx::Vk
{
	bool InitializeBackend(
		Context& gfxData, 
		InitInfo const& initInfo, 
		void*& apiDataBuffer);
}

Std::Opt<Gfx::Context> Gfx::Initialize(InitInfo const& initInfo)
{
	Gfx::Context returnVal{};

	returnVal.logger = initInfo.optional_logger;
	returnVal.texAssetInterface = initInfo.texAssetInterface;

	Vk::InitializeBackend(returnVal, initInfo, returnVal.apiDataBuffer);

	return Std::Opt<Gfx::Context>{ Std::Move(returnVal) };
}

void Gfx::Context::Draw(DrawParams const& params)
{
	static_cast<APIDataBase*>(this->apiDataBuffer)->Draw(*this, params);
}

Gfx::NativeWindowID Gfx::Context::NewNativeWindow(WsiInterface& wsiConnection)
{
	return static_cast<APIDataBase*>(this->apiDataBuffer)->NewNativeWindow(wsiConnection);
}

Gfx::ViewportRef Gfx::Context::NewViewport()
{
	ViewportRef returnVal{};
	
	static_cast<APIDataBase*>(this->apiDataBuffer)->NewViewport(returnVal.viewportID);

	return returnVal;
}

void Gfx::Context::DeleteViewport(ViewportID viewportID)
{
	static_cast<APIDataBase*>(this->apiDataBuffer)->DeleteViewport(viewportID);
}

void Gfx::Context::NewFontTexture(
	u32 id,
	u32 width,
	u32 height,
	u32 pitch,
	Std::Span<std::byte const> data)
{
	return static_cast<APIDataBase*>(this->apiDataBuffer)->NewFontTexture(
		id,
		width,
		height,
		pitch,
		data);
}
