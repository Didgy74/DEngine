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
			const std::byte* posBuffer = nullptr;
			const std::byte* uvBuffer = nullptr;
			const std::byte* normalBuffer = nullptr;
			const std::byte* tangentBuffer = nullptr;
			const std::byte* indexBuffer = nullptr;
			uint32_t vertexCount = 0;
			IndexType indexType{};
			uint32_t indexCount = 0;
		};

		explicit MeshDocument(CreateInfo&& info);
		MeshDocument() = delete;
		MeshDocument(const MeshDocument&) = default;
		MeshDocument(MeshDocument&&) = default;

		MeshDocument& operator=(const MeshDocument&) = default;
		MeshDocument& operator=(MeshDocument&&) = default;

		size_t GetBufferSize(Attribute attr) const;
		const std::byte* GetDataPtr(Attribute attr) const;

		uint32_t GetVertexCount() const;
		IndexType GetIndexType() const;
		uint32_t GetIndexCount() const;

		size_t GetTotalSizeRequired() const;

		static uint8_t ToByteSize(IndexType indexType);
		static CreateInfo ToCreateInfo(MeshDocument&& input);

	private:
		const std::byte*& GetDataPtr(Attribute attr);

		std::array<const std::byte*, static_cast<size_t>(Attribute::COUNT)> bufferRefs;
		uint32_t vertexCount{};
		IndexType indexType{};
		uint32_t indexCount{};
	};

	using MeshDoc = MeshDocument;
}