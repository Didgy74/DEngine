#pragma once

#include <DEngine/Gfx/Gfx.hpp>

#include <DEngine/Containers/Optional.hpp>

#include <vector>

namespace DEngine::Gui
{
	struct DrawInfo
	{
		std::vector<Gfx::GuiVertex>& vertices;
		std::vector<u32>& indices;
		std::vector<Gfx::GuiDrawCmd>& drawCmds;



		DrawInfo(
			std::vector<Gfx::GuiVertex>& verticesIn,
			std::vector<u32>& indicesIn,
			std::vector<Gfx::GuiDrawCmd>& drawCmdsIn) :
			vertices(verticesIn),
			indices(indicesIn),
			drawCmds(drawCmdsIn)
		{
			vertices.clear();
			indices.clear();
			drawCmds.clear();

			BuildQuad();
		}

		u32 quadVertexOffset = 0;
		u32 quadIndexOffset = 0;
		void BuildQuad()
		{
			quadVertexOffset = (u32)vertices.size();
			quadIndexOffset = (u32)indices.size();

			// Insert vertices
			vertices.reserve(vertices.size() + 4);
			Gfx::GuiVertex newVertex{};
			// Top left
			newVertex = { { 0.f, 0.f }, { 0.f, 0.f } };
			vertices.push_back(newVertex);
			// Top right
			newVertex = { { 1.f, 0.f}, { 1.f, 0.f } };
			vertices.push_back(newVertex);
			// Bottom left
			newVertex = { { 0.f, 1.f}, { 0.f, 1.f } };
			vertices.push_back(newVertex);
			// Bottom right
			newVertex = { { 1.f, 1.f}, { 1.f, 1.f } };
			vertices.push_back(newVertex);

			// Insert indices
			indices.reserve(indices.size() + 6);
			indices.push_back(0);
			indices.push_back(2);
			indices.push_back(1);
			indices.push_back(1);
			indices.push_back(2);
			indices.push_back(3);
		}

		Gfx::GuiDrawCmd::MeshSpan GetQuadMesh() const
		{
			Gfx::GuiDrawCmd::MeshSpan newMeshSpan;
			newMeshSpan.vertexOffset = quadVertexOffset;
			newMeshSpan.indexOffset = quadIndexOffset;
			newMeshSpan.indexCount = 6;
			return newMeshSpan;
		}
	};
}