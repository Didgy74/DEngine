#pragma once

#include <cstddef>
#include <array>

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

			static uint8_t ToByteSize(IndexType type);

			struct DataInfo
			{
				size_t byteOffset;
				size_t byteLength;
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
				DataInfo posData;
				DataInfo uvData;
				DataInfo normalData;
				DataInfo tangentData;
				DataInfo indexData;
				IndexType indexType;
			};

			MeshDocument(CreateInfo&& createInfo);
			MeshDocument() = default;

			DataInfo GetData(Attribute attr) const;
			const uint8_t* GetDataPtr(Attribute attr) const;

			IndexType GetIndexType() const;
			uint32_t GetIndexCount() const;

			size_t GetTotalByteLength() const;

			IndexType indexType;
			std::vector<uint8_t> byteArray;
			std::array<DataInfo, static_cast<size_t>(Attribute::COUNT)> data;	
		};

		inline MeshDocument::DataInfo MeshDocument::GetData(Attribute attr) const
		{
			return data.at(static_cast<size_t>(attr));
		}

		inline const uint8_t* MeshDocument::GetDataPtr(Attribute attr) const
		{
			return byteArray.data() + GetData(attr).byteOffset;
		}

		inline MeshDocument::IndexType MeshDocument::GetIndexType() const
		{
			return indexType;
		}

		inline uint32_t MeshDocument::GetIndexCount() const
		{
			return uint32_t( GetData( Attribute::Index).byteLength / ToByteSize(GetIndexType() ) );
		}

		inline size_t MeshDocument::GetTotalByteLength() const
		{
			return data.size();
		}

		inline uint8_t MeshDocument::ToByteSize(IndexType type)
		{
			return type == IndexType::UInt16 ? uint8_t(2) : uint8_t(4);
		}
	}
}