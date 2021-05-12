#pragma once

#include <DEngine/Gui/Utility.hpp>

#include <DEngine/Gfx/Gfx.hpp>
#include <vector>

namespace DEngine::Gui
{
	struct DrawInfo
	{
		std::vector<Gfx::GuiVertex>* vertices;
		std::vector<u32>* indices;
		std::vector<Gfx::GuiDrawCmd>* drawCmds;
	protected:
		Extent framebufferExtent;
		std::vector<Rect> scissors;
	public:
		Extent GetFramebufferExtent() const { return framebufferExtent; }

		DrawInfo(
			Extent framebufferExtent,
			std::vector<Gfx::GuiVertex>& verticesIn,
			std::vector<u32>& indicesIn,
			std::vector<Gfx::GuiDrawCmd>& drawCmdsIn) :
			vertices(&verticesIn),
			indices(&indicesIn),
			drawCmds(&drawCmdsIn),
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

		Gfx::GuiDrawCmd::MeshSpan GetQuadMesh() const
		{
			Gfx::GuiDrawCmd::MeshSpan newMeshSpan;
			newMeshSpan.vertexOffset = quadVertexOffset;
			newMeshSpan.indexOffset = quadIndexOffset;
			newMeshSpan.indexCount = 6;
			return newMeshSpan;
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
			Gfx::GuiDrawCmd cmd{};
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
	};
}