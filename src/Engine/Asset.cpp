#define ASSET_TEXTURE_COUNT
#define ASSET_MESH_COUNT
#include "Asset.hpp"

#include <cassert>
#include <map>
#include <type_traits>

#include "fx-gltf/gltf.h"

//#pragma warning( push, 0 )
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
//#pragma warning( pop )

struct AssetInfo
{
	std::string name;
	std::string relativePath;
};

static const std::map<Asset::Sprite, AssetInfo> textureInfos
{
	{ Asset::Sprite::None, {"None" , ""} },
	{ Asset::Sprite::Default, {"Default", "defaultTexture.png"} },
	{ Asset::Sprite::Test, {"Test", "test.png"} },
	{ Asset::Sprite::Circle, {"Circle", "circle.png"} },
};

static const std::map<Asset::Mesh, AssetInfo> meshInfos
{
	{ Asset::Mesh::None, {"None", ""} },
	{ Asset::Mesh::Cube, {"Cube", "Cube/Cube.gltf"} },
	{ Asset::Mesh::SpritePlane, {"SpritePlane", "SpritePlane/SpritePlane.gltf"} },
};

bool Asset::CheckValid(Sprite sprite)
{
	using Type = std::underlying_type_t<Sprite>;
	return (0 <= static_cast<Type>(sprite) && static_cast<Type>(sprite) < static_cast<Type>(Sprite::COUNT)) || sprite == Sprite::None;
}

std::string Asset::GetPath(Sprite texture)
{
	assert(CheckValid(texture));
	auto iterator = textureInfos.find(texture);
	assert(iterator != textureInfos.end());
	return static_cast<std::string>(textureFolderPath) + iterator->second.relativePath;
}

std::string Asset::GetName(Sprite texture)
{
	assert(CheckValid(texture));
	auto iterator = textureInfos.find(texture);
	assert(iterator != textureInfos.end());
	return iterator->second.name;
}

bool Asset::CheckValid(Mesh mesh)
{
	using Type = std::underlying_type_t<Mesh>;
	return (0 <= static_cast<Type>(mesh) && static_cast<Type>(mesh) < static_cast<Type>(Mesh::COUNT)) || mesh == Mesh::None;
}

std::string Asset::GetPath(Mesh mesh)
{
	assert(CheckValid(mesh));
	auto iterator = meshInfos.find(mesh);
	assert(iterator != meshInfos.end());
	return static_cast<std::string>(meshFolderPath) + iterator->second.relativePath;
}

std::string Asset::GetName(Mesh mesh)
{
	assert(CheckValid(mesh));
	auto iterator = meshInfos.find(mesh);
	assert(iterator != meshInfos.end());
	return iterator->second.name;
}

Asset::MeshDocument Asset::LoadMeshDocument(Mesh mesh)
{
	const auto& path = GetPath(mesh);
	return LoadMeshDocument(path);
}

Asset::MeshDocument Asset::LoadMeshDocument(const std::string &path)
{
	MeshDocument::CreateInfo createInfo{};

	const auto gltfDocument = fx::gltf::LoadFromText(path);
	auto& primitive = gltfDocument.meshes[0].primitives[0];

	auto& posAccessor = gltfDocument.accessors[primitive.attributes.find("POSITION")->second];
	assert(posAccessor.type == fx::gltf::Accessor::Type::Vec3 && posAccessor.componentType == fx::gltf::Accessor::ComponentType::Float);
	auto& posBufferView = gltfDocument.bufferViews[posAccessor.bufferView];
	createInfo.posData.byteOffset = posBufferView.byteOffset + posAccessor.byteOffset;
	createInfo.posData.byteLength = posAccessor.count * (sizeof(float) * 3);

	auto& uvAccessor = gltfDocument.accessors[primitive.attributes.find("TEXCOORD_0")->second];
	assert(uvAccessor.type == fx::gltf::Accessor::Type::Vec2 && uvAccessor.componentType == fx::gltf::Accessor::ComponentType::Float);
	auto& uvBufferView = gltfDocument.bufferViews[uvAccessor.bufferView];
	createInfo.uvData.byteOffset = uvBufferView.byteOffset + uvAccessor.byteOffset;
	createInfo.uvData.byteLength = uvAccessor.count * (sizeof(float) * 2);

	auto& normalAccessor = gltfDocument.accessors[primitive.attributes.find("NORMAL")->second];
	assert(normalAccessor.type == fx::gltf::Accessor::Type::Vec3 && normalAccessor.componentType == fx::gltf::Accessor::ComponentType::Float);
	auto& normalBufferView = gltfDocument.bufferViews[normalAccessor.bufferView];
	createInfo.normalData.byteOffset = normalBufferView.byteOffset + normalAccessor.byteOffset;
	createInfo.normalData.byteLength = normalAccessor.count * (sizeof(float) * 3);

	assert(posAccessor.count == uvAccessor.count);

	auto& indexAccessor = gltfDocument.accessors[primitive.indices];
	assert(indexAccessor.type == fx::gltf::Accessor::Type::Scalar);
	assert(indexAccessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort
		   || indexAccessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedInt);
	auto& indexBufferView = gltfDocument.bufferViews[indexAccessor.bufferView];
	createInfo.indexData.byteOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
	createInfo.indexData.byteLength = indexBufferView.byteLength;
	createInfo.indexType = indexAccessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort ? MeshDocument::IndexType::Uint16 : MeshDocument::IndexType::Uint32;

	createInfo.byteArray = std::move(gltfDocument.buffers[posBufferView.buffer].data);

	return MeshDocument(std::move(createInfo));
}

Asset::TextureDocument Asset::LoadTextureDocument(Sprite texture)
{
	TextureDocument::CreateInfo createInfo{};

	// Grab pixels as char array from image.
	int32_t x = 0;
	int32_t y = 0;
	int32_t channelCount = 0;
	constexpr int32_t desiredChannelCount = 4;
	auto pixels = stbi_load(Asset::GetPath(texture).c_str(), &x, &y, &channelCount, desiredChannelCount);
	assert(pixels);
	// Weird STBI API stuff
	if constexpr (desiredChannelCount != 0)
		channelCount = desiredChannelCount;

	createInfo.byteArray = pixels;
	createInfo.channelCount = static_cast<uint8_t>(channelCount);

	using ValueType = decltype(createInfo.dimensions)::ValueType;
	createInfo.dimensions = { static_cast<ValueType>(x), static_cast<ValueType>(y) };

	return TextureDocument(std::move(createInfo));
}

static Asset::MeshDocument::IndexType ToIndexType(fx::gltf::Accessor::ComponentType componentType)
{
	switch (componentType)
	{
	case fx::gltf::Accessor::ComponentType::UnsignedShort:
		return Asset::MeshDocument::IndexType::Uint16;
	case fx::gltf::Accessor::ComponentType::UnsignedInt:
		return Asset::MeshDocument::IndexType::Uint32;
	default:
		return static_cast<Asset::MeshDocument::IndexType>(-1);
	}
}

Asset::MeshDocument::MeshDocument(CreateInfo&& info) :
	byteArray(std::move(info.byteArray))
{
	data[static_cast<size_t>(Attribute::Position)] = info.posData;
	data[static_cast<size_t>(Attribute::TexCoord)] = info.uvData;
	data[static_cast<size_t>(Attribute::Normal)] = info.normalData;
	data[static_cast<size_t>(Attribute::Index)] = info.indexData;
	indexType = info.indexType;
}

const std::vector<uint8_t>& Asset::MeshDocument::GetByteArray() const { return byteArray; }

size_t Asset::MeshDocument::GetTotalByteLength() const
{
	size_t totalLength = 0;
	for (const auto& item : data)
		totalLength += item.byteLength;
	return totalLength;
}

Asset::MeshDocument::IndexType Asset::MeshDocument::GetIndexType() const { return indexType; }

uint32_t Asset::MeshDocument::GetIndexCount() const
{
	return static_cast<uint32_t>(data[static_cast<size_t>(Attribute::Index)].byteLength / IndexTypeToByteSize(indexType));
}

uint32_t Asset::MeshDocument::GetVertexCount() const
{
	return static_cast<uint32_t>(data[static_cast<size_t>(Attribute::Position)].byteLength / sizeof(PositionType));
}

const uint8_t* Asset::MeshDocument::GetDataPtr(Attribute type) const
{
	return byteArray.data() + data[static_cast<size_t>(type)].byteOffset;
}

Asset::MeshDocument::Data Asset::MeshDocument::GetData(Attribute type) const { return data[static_cast<size_t>(type)]; }

uint8_t Asset::MeshDocument::IndexTypeToByteSize(IndexType indexType)
{
	return indexType == IndexType::Uint16 ? uint8_t(2) : uint8_t(4);
}

Asset::TextureDocument::TextureDocument(CreateInfo&& right) :
	byteArray(right.byteArray),
	dimensions(right.dimensions),
	channelCount(right.channelCount),
	owner(true)
{
}

Asset::TextureDocument::TextureDocument(TextureDocument&& right) :
	byteArray(right.byteArray),
	dimensions(right.dimensions),
	channelCount(right.channelCount),
	owner(true)
{
	right.owner = false;
}

Asset::TextureDocument::~TextureDocument()
{
	if (owner)
		delete[] byteArray;
}

Utility::ImgDim Asset::TextureDocument::GetDimensions() const { return dimensions; }

const uint8_t* Asset::TextureDocument::GetData() const { return byteArray; }

uint8_t Asset::TextureDocument::GetChannelCount() const { return channelCount; }

size_t Asset::TextureDocument::GetByteLength() const
{
	return dimensions.width * dimensions.height * channelCount;
}
