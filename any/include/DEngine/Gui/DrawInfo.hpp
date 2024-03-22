#pragma once

#include <DEngine/Gui/Utility.hpp>
#include <DEngine/Gui/TextManager.hpp>
#include <DEngine/Std/Containers/FnRef.hpp>

#include <DEngine/Math/Vector.hpp>

#include <DEngine/Gfx/Gfx.hpp>
#include <vector>



// All the code in this document is mostly temporary and to be redesigned.

namespace DEngine::Gui
{
	class DrawInfo
	{
	public:
		std::vector<Gfx::GuiVertex>* vertices;
		std::vector<u32>* indices;
		std::vector<Gfx::GuiDrawCmd>* drawCmds;
		std::vector<u32>* utfValues;
		std::vector<Gfx::GlyphRect>* textGlyphRects;
	protected:
		Extent framebufferExtent;
		std::vector<Rect> scissors;
	public:
		[[nodiscard]] Extent GetFramebufferExtent() const { return framebufferExtent; }

		DrawInfo(
			Extent framebufferExtent,
			std::vector<Gfx::GuiVertex>& verticesIn,
			std::vector<u32>& indicesIn,
			std::vector<Gfx::GuiDrawCmd>& drawCmdsIn,
			std::vector<u32>& utfValuesIn,
			std::vector<Gfx::GlyphRect>& textGlyphRectsIn) :
			vertices(&verticesIn),
			indices(&indicesIn),
			drawCmds(&drawCmdsIn),
			utfValues(&utfValuesIn),
			textGlyphRects(&textGlyphRectsIn),
			framebufferExtent(framebufferExtent)
		{
			BuildQuad();
		}

		u32 quadVertexOffset = 0;
		u32 quadIndexOffset = 0;
		void BuildQuad()
		{
			quadVertexOffset = (u32)vertices->size();
			quadIndexOffset = (u32)indices->size();

			// Insert vertices
			vertices->reserve(vertices->size() + 4);
			Gfx::GuiVertex newVertex{};
			// Top left
			newVertex = { { 0.f, 0.f }, { 0.f, 0.f } };
			vertices->push_back(newVertex);
			// Top right
			newVertex = { { 1.f, 0.f}, { 1.f, 0.f } };
			vertices->push_back(newVertex);
			// Bottom left
			newVertex = { { 0.f, 1.f}, { 0.f, 1.f } };
			vertices->push_back(newVertex);
			// Bottom right
			newVertex = { { 1.f, 1.f}, { 1.f, 1.f } };
			vertices->push_back(newVertex);

			// Insert indices
			indices->reserve(indices->size() + 6);
			indices->push_back(0);
			indices->push_back(2);
			indices->push_back(1);
			indices->push_back(1);
			indices->push_back(2);
			indices->push_back(3);
		}

		[[nodiscard]] Gfx::GuiDrawCmd::MeshSpan GetQuadMesh() const
		{
			Gfx::GuiDrawCmd::MeshSpan newMeshSpan = {};
			newMeshSpan.vertexOffset = quadVertexOffset;
			newMeshSpan.indexOffset = quadIndexOffset;
			newMeshSpan.indexCount = 6;
			return newMeshSpan;
		}

		void PushText(
			u64 fontFaceId,
			Std::Span<char const> text,
			Rect const* rects,
			Math::Vec2Int posOffset,
			Math::Vec4 color)
		{
			DENGINE_IMPL_GUI_ASSERT(fontFaceId != 0);
			DENGINE_IMPL_GUI_ASSERT(fontFaceId != -1);

			auto const fbExtent = GetFramebufferExtent();

			int const length = (int)text.Size();

			auto& utfValues = *this->utfValues;
			int utfStartIndex = (int)utfValues.size();
			utfValues.resize(utfStartIndex + length);
			for (int i = 0; i < length; i++)
				utfValues[i + utfStartIndex] = (u32)(unsigned char)text[i];

			auto& textGlyphRects = *this->textGlyphRects;
			textGlyphRects.resize(utfStartIndex + length);
			for (int i = 0; i < length; i++) {
				auto const& srcRect = rects[i];

				Gfx::GlyphRect outRect {};
				outRect.pos.x = (f32)srcRect.position.x / (f32)fbExtent.width;
				outRect.pos.y = (f32)srcRect.position.y / (f32)fbExtent.height;
				outRect.extent.x = (f32)srcRect.extent.width / (f32)fbExtent.width;
				outRect.extent.y = (f32)srcRect.extent.height / (f32)fbExtent.height;
				textGlyphRects[i + utfStartIndex] = outRect;
			}

			Gfx::GuiDrawCmd cmd {};
			cmd.type = Gfx::GuiDrawCmd::Type::Text;
			cmd.text = {};
			cmd.text.startIndex = utfStartIndex;
			cmd.text.count = length;
			cmd.text.fontFaceId = (Gfx::FontFaceId)fontFaceId;
			cmd.text.posOffset.x = (f32)posOffset.x / (f32)fbExtent.width;
			cmd.text.posOffset.y = (f32)posOffset.y / (f32)fbExtent.height;
			cmd.text.color = color;
			drawCmds->push_back(cmd);
		}

		void PushScissor(Rect rect)
		{
			Rect rectToUse = {};
			if (!scissors.empty())
				rectToUse = Rect::Intersection(rect, scissors.back());
			else
				rectToUse = Rect::Intersection(rect, Rect{ {}, framebufferExtent });
			scissors.push_back(rectToUse);

			Gfx::GuiDrawCmd cmd{};
			cmd.type = Gfx::GuiDrawCmd::Type::Scissor;
			cmd.rectPosition.x = (f32)rectToUse.position.x / (f32)framebufferExtent.width;
			cmd.rectPosition.y = (f32)rectToUse.position.y / (f32)framebufferExtent.height;
			cmd.rectExtent.x = (f32)rectToUse.extent.width / (f32)framebufferExtent.width;
			cmd.rectExtent.y = (f32)rectToUse.extent.height / (f32)framebufferExtent.height;
			DENGINE_IMPL_GUI_ASSERT(cmd.rectPosition.x + cmd.rectExtent.x <= 1);
			DENGINE_IMPL_GUI_ASSERT(cmd.rectPosition.y + cmd.rectExtent.y <= 1);

			drawCmds->push_back(cmd);
		}

		void PopScissor()
		{
			DENGINE_IMPL_GUI_ASSERT(!scissors.empty());
			scissors.pop_back();
			Gfx::GuiDrawCmd cmd = {};
			cmd.type = Gfx::GuiDrawCmd::Type::Scissor;
			if (!scissors.empty()) {
				Rect rect = scissors.back();
				cmd.rectPosition.x = (f32)rect.position.x / (f32)framebufferExtent.width;
				cmd.rectPosition.y = (f32)rect.position.y / (f32)framebufferExtent.height;
				cmd.rectExtent.x = (f32)rect.extent.width / (f32)framebufferExtent.width;
				cmd.rectExtent.y = (f32)rect.extent.height / (f32)framebufferExtent.height;
			}
			else {
				cmd.rectPosition = {};
				cmd.rectExtent = { 1.f, 1.f };
			}
			drawCmds->push_back(cmd);
		}

		void PushFilledQuad(Rect rect, Math::Vec4 color, Math::Vec4Int radius) {
			Gfx::GuiDrawCmd cmd{};
			cmd.type = Gfx::GuiDrawCmd::Type::Rectangle;
			cmd.rectangle = Gfx::GuiDrawCmd::Rectangle{};
			cmd.rectangle.color = color;
			cmd.rectangle.pos.x = (f32)rect.position.x / (f32)framebufferExtent.width;
			cmd.rectangle.pos.y = (f32)rect.position.y / (f32)framebufferExtent.height;
			cmd.rectangle.extent.x = (f32)rect.extent.width / (f32)framebufferExtent.width;
			cmd.rectangle.extent.y = (f32)rect.extent.height / (f32)framebufferExtent.height;
			for (int i = 0; i < 4; i++) {
				cmd.rectangle.radius[i] = (f32)radius[i] / (f32)framebufferExtent.height;
			}

			cmd.rectPosition.x = (f32)rect.position.x / (f32)framebufferExtent.width;
			cmd.rectPosition.y = (f32)rect.position.y / (f32)framebufferExtent.height;
			cmd.rectExtent.x = (f32)rect.extent.width / (f32)framebufferExtent.width;
			cmd.rectExtent.y = (f32)rect.extent.height / (f32)framebufferExtent.height;
			drawCmds->push_back(cmd);
		}

		void PushFilledQuad(Rect rect, Math::Vec4 color, int radius) {
			PushFilledQuad(rect, color, { radius, radius, radius, radius });
		}

		void PushFilledQuad(Rect rect, Math::Vec4 color) {
			PushFilledQuad(rect, color, {});
		}

		class ScopedScissor
		{
		public:
			// Only activates if the target rect cannot fit within the visible rect.
			ScopedScissor(
				DrawInfo& drawInfoIn,
				Rect const& targetRect,
				Rect const& visibleRect)
			{
				if (!targetRect.FitsInside(visibleRect)) {
					drawInfo = &drawInfoIn;
					drawInfo->PushScissor(Intersection(targetRect, visibleRect));
				}
			}
			ScopedScissor(DrawInfo& drawInfo, Rect const& scissor) : drawInfo{ &drawInfo }
			{
				drawInfo.PushScissor(scissor);
			}
			ScopedScissor(ScopedScissor const&) = delete;
			ScopedScissor(ScopedScissor&& other) noexcept : drawInfo{ other.drawInfo }
			{
				other.drawInfo = nullptr;
			}
			
			ScopedScissor& operator=(ScopedScissor const&) = delete;
			ScopedScissor& operator=(ScopedScissor&& other) noexcept
			{
				if (this == &other)
					return *this;
				drawInfo = other.drawInfo;
				other.drawInfo = nullptr;
				return *this;
			}

			~ScopedScissor()
			{
				if (drawInfo)
					drawInfo->PopScissor();
			}

		protected:
			DrawInfo* drawInfo = nullptr;
		};
	};
}