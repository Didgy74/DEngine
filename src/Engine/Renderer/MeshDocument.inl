#pragma once

#include "DRenderer/MeshDocument.hpp"

namespace DRenderer
{
	MeshDocument::MeshDocument(CreateInfo&& createInfo) :
		indexType(createInfo.indexType),
		indexCount(createInfo.indexCount),
		vertexCount(createInfo.vertexCount)
	{
		GetDataPtr(Attribute::Position) = createInfo.posBuffer;
		GetDataPtr(Attribute::TexCoord) = createInfo.uvBuffer;
		GetDataPtr(Attribute::Normal) = createInfo.normalBuffer;
		GetDataPtr(Attribute::Tangent) = createInfo.tangentBuffer;
		GetDataPtr(Attribute::Index) = createInfo.indexBuffer;
	}

	size_t MeshDocument::GetBufferSize(Attribute attr) const
	{
		switch (attr)
		{
		case Attribute::Position:
			return GetVertexCount() * sizeof(PositionType);
		case Attribute::TexCoord:
			return GetVertexCount() * sizeof(UVType);
		case Attribute::Normal:
			return GetVertexCount() * sizeof(NormalType);
		case Attribute::Tangent:
			return GetVertexCount() * sizeof(TangentType);
		case Attribute::Index:
			return GetIndexCount() * ToByteSize(GetIndexType());
		default:
			assert(false);
			return 0;
		}
	}

	const std::byte* MeshDocument::GetDataPtr(Attribute attr) const
	{
		return bufferRefs[static_cast<size_t>(attr)];
	}

	const std::byte*& MeshDocument::GetDataPtr(Attribute attr)
	{
		return bufferRefs[static_cast<size_t>(attr)];
	}

	uint32_t MeshDocument::GetVertexCount() const
	{
		return vertexCount;
	}

	MeshDocument::IndexType MeshDocument::GetIndexType() const
	{
		return indexType;
	}

	uint32_t MeshDocument::GetIndexCount() const
	{
		return indexCount;
	}

	size_t MeshDocument::GetTotalSizeRequired() const
	{
		size_t temp = sizeof(PositionType) + sizeof(UVType) + sizeof(NormalType) + sizeof(TangentType);
		temp *= GetVertexCount();
		temp += GetIndexCount() * ToByteSize(GetIndexType());
		return temp;
	}

	uint8_t MeshDocument::ToByteSize(IndexType type)
	{
		return type == IndexType::UInt16 ? uint8_t(2) : uint8_t(4);
	}

	MeshDocument::CreateInfo MeshDocument::ToCreateInfo(MeshDocument&& input)
	{
		CreateInfo returnValue{};

		returnValue.vertexCount = input.vertexCount;
		returnValue.indexType = input.indexType;
		returnValue.indexCount = input.indexCount;

		returnValue.posBuffer = input.GetDataPtr(Attribute::Position);
		returnValue.uvBuffer = input.GetDataPtr(Attribute::TexCoord);
		returnValue.normalBuffer = input.GetDataPtr(Attribute::Normal);
		returnValue.tangentBuffer = input.GetDataPtr(Attribute::Tangent);
		returnValue.indexBuffer = input.GetDataPtr(Attribute::Index);

		return returnValue;
	}
}
