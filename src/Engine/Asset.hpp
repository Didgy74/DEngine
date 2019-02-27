#pragma once

#include "Utility/ImgDim.hpp"

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
#include <array>

namespace Asset
{
	constexpr std::string_view textureFolderPath = "Data/Textures/";
	constexpr std::string_view meshFolderPath = "Data/Meshes/";

	class MeshDocument;
	class TextureDocument;

	enum class Sprite : uint32_t;
	bool CheckValid(Sprite texture);
	std::string GetPath(Sprite texture);
	std::string GetName(Sprite texture);

	enum class Mesh : uint32_t;
	bool CheckValid(Mesh mesh);
	std::string GetPath(Mesh mesh);
	std::string GetName(Mesh mesh);

	MeshDocument LoadMeshDocument(Mesh mesh);
	TextureDocument LoadTextureDocument(Sprite texture);
}

class Asset::MeshDocument
{
public:
	enum class IndexType
	{
		Uint16,
		Uint32
	};

	enum class Attribute
	{
		Position,
		TexCoord,
		Normal,
		Index,
		COUNT
	};

	struct Data
	{
		size_t byteOffset;
		size_t byteLength;
	};

	struct CreateInfo
	{
		std::vector<uint8_t> byteArray;
		Data posData;
		Data uvData;
		Data normalData;
		Data indexData;
		IndexType indexType;
	};

	using PositionType = std::array<float, 3>;
	using UVType = std::array<float, 2>;
	using NormalType = std::array<float, 3>;

	MeshDocument(CreateInfo&& info);
	MeshDocument(const MeshDocument&) = delete;

	const std::vector<uint8_t>& GetByteArray() const;
	size_t GetTotalByteLength() const;
	IndexType GetIndexType() const;
	uint32_t GetIndexCount() const;
	uint32_t GetVertexCount() const;
	const uint8_t* GetDataPtr(Attribute type) const;
	Data GetData(Attribute type) const;

private:
	std::vector<uint8_t> byteArray;
	std::array<Data, static_cast<size_t>(Attribute::COUNT)> data;
	IndexType indexType;

	static uint8_t IndexTypeToByteSize(IndexType indexType);
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
	TextureDocument(TextureDocument&& right);
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
#ifdef ASSET_MESH_COUNT
	COUNT
#endif
};