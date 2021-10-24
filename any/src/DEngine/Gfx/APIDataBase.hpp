#pragma once

#include <DEngine/Gfx/Gfx.hpp>
#include <DEngine/FixedWidthTypes.hpp>

namespace DEngine::Gfx
{
	struct APIDataBase
	{
		inline APIDataBase() {}
		// This data can never be copied or moved
		APIDataBase(APIDataBase const&) = delete;
		APIDataBase(APIDataBase&&) = delete;
		inline virtual ~APIDataBase() = 0;

		APIDataBase& operator=(APIDataBase const&) = delete;
		APIDataBase& operator=(APIDataBase&&) = delete;

		virtual void Draw(DrawParams const& drawParams) = 0;

		// Needs to be thread-safe
		virtual void NewNativeWindow(NativeWindowID windowId) = 0;

		// Needs to be thread-safe
		virtual void NewViewport(ViewportID& viewportID) = 0;
		// Needs to be thread-safe.
		virtual void DeleteViewport(ViewportID id) = 0;

		// Needs to be thread-safe
		virtual void NewFontTexture(
			u32 id,
			u32 width,
			u32 height,
			u32 pitch,
			Std::Span<std::byte const> data) = 0;
	};

	inline APIDataBase::~APIDataBase() {}
}