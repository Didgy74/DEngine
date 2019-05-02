#pragma once

#include <cstddef>
#include <array>

#include <cassert>

namespace DRenderer
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
			Invalid,
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
			const std::byte* byteArrayRef{};
			size_t posByteOffset;
			size_t uvByteOffset;
			size_t normalByteOffset;
			size_t tangentByteOffset;
			size_t indexByteOffset;
			IndexType indexType{};
			uint32_t vertexCount;
			uint32_t indexCount;
		};

		explicit MeshDocument(CreateInfo&& info);
		MeshDocument(MeshDocument&&) = default;
		MeshDocument(const MeshDocument&) = delete;

		const size_t& GetByteOffset(Attribute attr) const;
		size_t GetByteLength(Attribute attr) const;
		const std::byte* GetDataPtr(Attribute attr) const;

		uint32_t GetVertexCount() const;
		IndexType GetIndexType() const;
		uint32_t GetIndexCount() const;

		size_t GetTotalSizeRequired() const;

		static uint8_t ToByteSize(IndexType indexType);
		static CreateInfo ToCreateInfo(MeshDocument&& input);

	private:
		size_t& GetByteOffset(Attribute attr);

		const std::byte* byteArrayRef;
		std::array<size_t, static_cast<size_t>(Attribute::COUNT)> byteOffsets;
		uint32_t vertexCount{};
		IndexType indexType{};
		uint32_t indexCount{};
	};

	using MeshDoc = MeshDocument;
}