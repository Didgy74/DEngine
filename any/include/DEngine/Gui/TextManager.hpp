#pragma once

#include <DEngine/Gui/Utility.hpp>
#include <DEngine/Gui/SizeHint.hpp>
#include <DEngine/Gui/DrawInfo.hpp>

#include <DEngine/Std/Containers/Span.hpp>
#include <DEngine/Std/Containers/Vec.hpp>
#include <DEngine/Std/BumpAllocator.hpp>
#include <DEngine/Math/Vector.hpp>

// Temporary
#include <DEngine/Application.hpp>
#include <DEngine/Gfx/Gfx.hpp>

namespace DEngine::Gui
{
	class DrawInfo;

	// Here there be no null-terminated strings allowed
	class TextManager
	{
	public:
		void* m_implData = nullptr;
		[[nodiscard]] void* GetImplData() { return m_implData; }
		[[nodiscard]] void const* GetImplData() const { return m_implData; }

		[[nodiscard]] u32 GetLineheight();

		void RenderText(
			Std::Span<char const> str,
			Math::Vec4 color,
			Rect widgetRect,
			DrawInfo& drawInfo);


		[[nodiscard]] Extent GetOuterExtent(
			Std::Span<char const> str);

		// The outputted rects are relative to (0, 0).
		// The rects do not need to be initialized.
		Extent GetOuterExtent(
			Std::Span<char const> str,
			Math::Vec2Int posOffset,
			Rect* outRects);
	};

	namespace impl
	{
		void InitializeTextManager(
			TextManager& manager,
			App::Context* appCtx,
			Gfx::Context* gfxCtx);
	}
}