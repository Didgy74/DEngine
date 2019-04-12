#pragma once

#include "DRenderer/MeshDocument.hpp"

namespace Engine
{
	namespace Renderer
	{
		MeshDocument::MeshDocument(CreateInfo&& createInfo) :
			byteArray(std::move(createInfo.byteArray)),
			indexType(std::move(createInfo.indexType)),
			indexCount(std::move(createInfo.indexCount)),
			vertexCount(std::move(createInfo.vertexCount))
		{
			GetByteOffset(Attribute::Position) = std::move(createInfo.posByteOffset);
			GetByteOffset(Attribute::TexCoord) = std::move(createInfo.uvByteOffset);
			GetByteOffset(Attribute::Normal) = std::move(createInfo.normalByteOffset);
			GetByteOffset(Attribute::Tangent) = std::move(createInfo.tangentByteOffset);
			GetByteOffset(Attribute::Index) = std::move(createInfo.indexByteOffset);
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
				return GetIndexCount()* ToByteSize(GetIndexType());
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

		const uint8_t* MeshDocument::GetDataPtr(Attribute attr) const
		{
			return byteArray.data() + GetByteOffset(attr);
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
			return byteArray.size();
		}

		uint8_t MeshDocument::ToByteSize(IndexType type)
		{
			return type == IndexType::UInt16 ? uint8_t(2) : uint8_t(4);
		}

		MeshDocument::CreateInfo MeshDocument::ToCreateInfo(MeshDocument&& input)
		{
			CreateInfo returnValue;

			returnValue.byteArray = std::move(input.byteArray);
			returnValue.vertexCount = std::move(input.vertexCount);
			returnValue.indexType = std::move(input.indexType);
			returnValue.indexCount = std::move(input.indexCount);

			returnValue.posByteOffset = std::move(input.GetByteOffset(Attribute::Position));
			returnValue.uvByteOffset = std::move(input.GetByteOffset(Attribute::TexCoord));
			returnValue.normalByteOffset = std::move(input.GetByteOffset(Attribute::Normal));
			returnValue.tangentByteOffset = std::move(input.GetByteOffset(Attribute::Tangent));
			returnValue.indexByteOffset = std::move(input.GetByteOffset(Attribute::Index));

			return returnValue;
		}
	}
}