#pragma once

#include "DRenderer/MeshDocument.hpp"

namespace DRenderer
{
	MeshDocument::MeshDocument(CreateInfo&& createInfo) :
		byteArrayRef(createInfo.byteArrayRef),
		indexType(createInfo.indexType),
		indexCount(createInfo.indexCount),
		vertexCount(createInfo.vertexCount),
		byteOffsets()
	{
		GetByteOffset(Attribute::Position) = createInfo.posByteOffset;
		GetByteOffset(Attribute::TexCoord) = createInfo.uvByteOffset;
		GetByteOffset(Attribute::Normal) = createInfo.normalByteOffset;
		GetByteOffset(Attribute::Tangent) = createInfo.tangentByteOffset;
		GetByteOffset(Attribute::Index) = createInfo.indexByteOffset;
	}

	size_t MeshDocument::GetByteLength(Attribute attr) const
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

	const size_t& MeshDocument::GetByteOffset(Attribute attr) const
	{
		return byteOffsets.at(size_t(attr));
	}

	size_t& MeshDocument::GetByteOffset(Attribute attr)
	{
		return byteOffsets.at(static_cast<size_t>(attr));
	}

	const std::byte* MeshDocument::GetDataPtr(Attribute attr) const
	{
		return byteArrayRef + GetByteOffset(attr);
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

		returnValue.byteArrayRef = input.byteArrayRef;
		returnValue.vertexCount = input.vertexCount;
		returnValue.indexType = input.indexType;
		returnValue.indexCount = input.indexCount;

		returnValue.posByteOffset = input.GetByteOffset(Attribute::Position);
		returnValue.uvByteOffset = input.GetByteOffset(Attribute::TexCoord);
		returnValue.normalByteOffset = input.GetByteOffset(Attribute::Normal);
		returnValue.tangentByteOffset = input.GetByteOffset(Attribute::Tangent);
		returnValue.indexByteOffset = input.GetByteOffset(Attribute::Index);

		return returnValue;
	}
}
