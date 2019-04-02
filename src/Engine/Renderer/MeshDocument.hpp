#pragma once

#include <cstddef>
#include <array>

#include <cassert>

namespace Engine
{
	namespace Renderer
	{
		class MeshDocument
		{
		public:
			using PositionType = std::array<float, 3>;
			using UVType = std::array<float, 2>;
			using NormalType = std::array<float, 3>;
			using TangentType = std::array<float, 3>;

			enum class IndexType
			{
				UInt16,
				UInt32
			};

			enum class Attribute
			{
				Position,
				TexCoord,
				Normal,
				Tangent,
				Index,
				COUNT
			};

			struct CreateInfo
			{
				std::vector<uint8_t> byteArray;
				uint32_t vertexCount;
				size_t posByteOffset;
				size_t uvByteOffset;
				size_t normalByteOffset;
				size_t tangentByteOffset;
				size_t indexByteOffset;
				IndexType indexType;
				uint32_t indexCount;
			};

			MeshDocument(CreateInfo&& createInfo);
			MeshDocument(MeshDocument&&) = default;
			MeshDocument(const MeshDocument&) = default;

			const std::vector<uint8_t>& GetByteArray() const;
			const size_t& GetByteOffset(Attribute attr) const;
			size_t GetByteLength(Attribute attr) const;
			const uint8_t* GetDataPtr(Attribute attr) const;

			uint32_t GetVertexCount() const;

			IndexType GetIndexType() const;
			uint32_t GetIndexCount() const;

			size_t GetTotalSizeRequired() const;

			static uint8_t ToByteSize(IndexType type);
			static CreateInfo ToCreateInfo(MeshDocument&& input);

		private:
			size_t& GetByteOffset(Attribute attr);

			IndexType indexType;
			uint32_t indexCount;
			uint32_t vertexCount;
			std::vector<uint8_t> byteArray;
			std::array<size_t, static_cast<size_t>(Attribute::COUNT)> data;
		};

		inline MeshDocument::MeshDocument(CreateInfo&& createInfo) :
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

		inline const size_t& MeshDocument::GetByteOffset(Attribute attr) const
		{
			return data.at(size_t(attr));
		}

		inline size_t MeshDocument::GetByteLength(Attribute attr) const
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

		inline size_t& MeshDocument::GetByteOffset(Attribute attr)
		{
			return data.at(static_cast<size_t>(attr));
		}

		inline const uint8_t* MeshDocument::GetDataPtr(Attribute attr) const
		{
			return byteArray.data() + GetByteOffset(attr);
		}

		inline uint32_t MeshDocument::GetVertexCount() const
		{
			return vertexCount;
		}

		inline MeshDocument::IndexType MeshDocument::GetIndexType() const
		{
			return indexType;
		}

		inline uint32_t MeshDocument::GetIndexCount() const
		{
			return indexCount;
		}

		inline size_t MeshDocument::GetTotalSizeRequired() const
		{
			return data.size();
		}

		inline uint8_t MeshDocument::ToByteSize(IndexType type)
		{
			return type == IndexType::UInt16 ? uint8_t(2) : uint8_t(4);
		}

		inline MeshDocument::CreateInfo MeshDocument::ToCreateInfo(MeshDocument&& input)
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