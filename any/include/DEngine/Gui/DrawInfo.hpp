#pragma once

#include <DEngine/Gui/Utility.hpp>

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
			Rect rectToUse{};
			if (!scissors.empty())
				rectToUse = Rect::Intersection(rect, scissors.back());
			else
				rectToUse = Rect::Intersection(rect, Rect{ {}, framebufferExtent });
			scissors.push_back(rectToUse);
			Gfx::GuiDrawCmd cmd{};
			cmd.type = Gfx::GuiDrawCmd::Type::Scissor;
			cmd.rectPosition.x = (f32)rectToUse.position.x / framebufferExtent.width;
			cmd.rectPosition.y = (f32)rectToUse.position.y / framebufferExtent.height;
			cmd.rectExtent.x = (f32)rectToUse.extent.width / framebufferExtent.width;
			cmd.rectExtent.y = (f32)rectToUse.extent.height / framebufferExtent.height;
			drawCmds->push_back(cmd);
		}

		void PopScissor()
		{
			DENGINE_IMPL_GUI_ASSERT(!scissors.empty());
			scissors.pop_back();
			Gfx::GuiDrawCmd cmd = {};
			cmd.type = Gfx::GuiDrawCmd::Type::Scissor;
			if (!scissors.empty())
			{
				Rect rect = scissors.back();
				cmd.rectPosition.x = (f32)rect.position.x / framebufferExtent.width;
				cmd.rectPosition.y = (f32)rect.position.y / framebufferExtent.height;
				cmd.rectExtent.x = (f32)rect.extent.width / framebufferExtent.width;
				cmd.rectExtent.y = (f32)rect.extent.height / framebufferExtent.height;
			}
			else
			{
				cmd.rectPosition = {};
				cmd.rectExtent = { 1.f, 1.f };
			}
			drawCmds->push_back(cmd);
		}

		void PushFilledQuad(Rect rect, Math::Vec4 color)
		{
			Gfx::GuiDrawCmd cmd{};
			cmd.type = Gfx::GuiDrawCmd::Type::FilledMesh;
			cmd.filledMesh.color = color;
			cmd.filledMesh.mesh = GetQuadMesh();
			cmd.rectPosition.x = f32(rect.position.x) / framebufferExtent.width;
			cmd.rectPosition.y = f32(rect.position.y) / framebufferExtent.height;
			cmd.rectExtent.x = f32(rect.extent.width) / framebufferExtent.width;
			cmd.rectExtent.y = f32(rect.extent.height) / framebufferExtent.height;
			drawCmds->push_back(cmd);
		}

		class ScopedScissor
		{
		public:
			ScopedScissor(
				DrawInfo& drawInfoIn,
				Rect const& widgetRect,
				Rect const& visibleRect)
			{
				if (!widgetRect.FitsInside(visibleRect)) {
					drawInfo = &drawInfoIn;
					drawInfo->PushScissor(Intersection(widgetRect, visibleRect));
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