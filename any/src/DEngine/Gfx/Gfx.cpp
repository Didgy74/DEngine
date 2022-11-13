#include <DEngine/Gfx/Gfx.hpp>
#include "APIDataBase.hpp"

#include <DEngine/Std/Utility.hpp>
#include <DEngine/Gfx/impl/Assert.hpp>

using namespace DEngine;

Gfx::Context::Context(Context&& other) noexcept
{
	logger = other.logger;
	other.logger = nullptr;

	texAssetInterface = other.texAssetInterface;
	other.texAssetInterface = nullptr;

	apiDataBase = other.apiDataBase;
	other.apiDataBase = nullptr;
}

Gfx::Context::~Context()
{
	auto* apiData = static_cast<APIDataBase*>(apiDataBase);

	if (apiData)
		delete apiData;
}

namespace DEngine::Gfx::Vk
{
	APIDataBase* InitializeBackend(
		Context& gfxData,
		InitInfo const& initInfo);
}

Std::Opt<Gfx::Context> Gfx::Initialize(InitInfo const& initInfo)
{
	Gfx::Context returnVal{};

	returnVal.logger = initInfo.optional_logger;
	returnVal.texAssetInterface = initInfo.texAssetInterface;

	returnVal.apiDataBase = Vk::InitializeBackend(returnVal, initInfo);
	if (!returnVal.apiDataBase)
		return Std::nullOpt;

	return Std::Opt<Gfx::Context>{ Std::Move(returnVal) };
}

void Gfx::Context::Draw(DrawParams const& params)
{
	DENGINE_IMPL_GFX_ASSERT(!params.nativeWindowUpdates.empty());

	auto& apiData = *static_cast<APIDataBase*>(apiDataBase);

	apiData.Draw(params);
}

Gfx::ViewportRef Gfx::Context::NewViewport()
{
	ViewportRef returnVal{};

	auto& apiData = *static_cast<APIDataBase*>(apiDataBase);

	apiData.NewViewport(returnVal.viewportID);

	return returnVal;
}

void Gfx::Context::DeleteViewport(ViewportID viewportID)
{
	auto& apiData = *static_cast<APIDataBase*>(apiDataBase);
	apiData.DeleteViewport(viewportID);
}

void Gfx::Context::NewFontFace(FontFaceId fontFaceId)
{
	auto& apiData = *static_cast<APIDataBase*>(GetApiData());
	return apiData.NewFontFace(fontFaceId);
}

void Gfx::Context::NewFontTexture(
	FontFaceId fontFaceId,
	u32 id,
	u32 width,
	u32 height,
	u32 pitch,
	Std::Span<std::byte const> data)
{
	auto& apiData = *static_cast<APIDataBase*>(apiDataBase);

	return apiData.NewFontTexture(
		fontFaceId,
		id,
		width,
		height,
		pitch,
		data);
}

void Gfx::Context::AdoptNativeWindow(Gfx::NativeWindowID in)
{
	auto& apiData = *static_cast<APIDataBase*>(apiDataBase);

	apiData.NewNativeWindow(in);
}

void Gfx::Context::DeleteNativeWindow(Gfx::NativeWindowID in)
{
	auto& apiData = *static_cast<APIDataBase*>(apiDataBase);

	apiData.DeleteNativeWindow(in);
}
