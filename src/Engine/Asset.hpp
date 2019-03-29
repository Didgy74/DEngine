#pragma once

#include "Utility/ImgDim.hpp"

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
#include <array>
#include <optional>

#include "Renderer/MeshDocument.hpp"

namespace Asset
{
	constexpr std::string_view textureFolderPath = "Data/Textures/";
	constexpr std::string_view meshFolderPath = "Data/Meshes/";

	class MeshDocument;
	class TextureDocument;

	enum class Sprite : uint32_t;
	bool CheckValid(Sprite texture);

	enum class Mesh : uint32_t;
	std::string GetMeshPath(size_t i);

	MeshDocument LoadMeshDocument(Mesh mesh);
	std::optional<MeshDocument> LoadMeshDocument(std::string_view path);
	TextureDocument LoadTextureDocument(Sprite texture);

	std::optional<Engine::Renderer::MeshDocument> LoadMesh(size_t i);
}

class Asset::MeshDocument
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
		size_t posByteOffset;
		size_t uvByteOffset;
		size_t normalByteOffset;
		size_t tangentByteOffset;
		size_t indexByteOffset;
		IndexType indexType;
		uint32_t vertexCount;
		uint32_t indexCount;
	};

	MeshDocument(CreateInfo&& info);
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

	static uint8_t ToByteSize(IndexType indexType);
	static CreateInfo ToCreateInfo(MeshDocument&& input);

private:
	size_t& GetByteOffset(Attribute attr);

	std::vector<uint8_t> byteArray;
	std::array<size_t, static_cast<size_t>(Attribute::COUNT)> byteOffsets;
	uint32_t vertexCount;
	IndexType indexType;
	uint32_t indexCount;
};

class Asset::TextureDocument
{
public:
	struct CreateInfo
	{
		Utility::ImgDim dimensions;
		uint8_t* byteArray;
		uint8_t channelCount;
	};

	TextureDocument(CreateInfo&&);
	TextureDocument(TextureDocument&&);
	TextureDocument(const TextureDocument&) = delete;
	~TextureDocument();

	Utility::ImgDim GetDimensions() const;
	const uint8_t* GetData() const;
	uint8_t GetChannelCount() const;

	size_t GetByteLength() const;

private:
	Utility::ImgDim dimensions;
	bool owner;
	uint8_t* byteArray;
	uint8_t channelCount;
};

enum class Asset::Sprite : uint32_t
{
	None,
	Default,
	Test, 
	Circle,
#ifdef ASSET_TEXTURE_COUNT
	COUNT
#endif
};

enum class Asset::Mesh : uint32_t
{
	None,
	Plane,
	Cube,
	SpritePlane,
	Helmet,
#ifdef ASSET_MESH_COUNT
	COUNT
#endif
};