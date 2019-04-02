#define ASSET_TEXTURE_COUNT
#define ASSET_MESH_COUNT
#include "Asset.hpp"

#include <cassert>
#include <map>
#include <type_traits>
#include <iostream>

#include "fx/gltf.h"

struct AssetInfo
{
	std::string_view name;
	std::string_view relativePath;
};

static const std::map<Asset::Sprite, AssetInfo> textureInfos
{
	{ Asset::Sprite::None, {"None" , ""} },
	{ Asset::Sprite::Default, {"Default", "defaultTexture.png"} },
	{ Asset::Sprite::Test, {"Test", "test.png"} },
	{ Asset::Sprite::Circle, {"Circle", "circle.png"} },
};

static const std::map<size_t, AssetInfo> meshInfos
{
	{ size_t(Asset::Mesh::None), {"None", ""} },
	{ size_t(Asset::Mesh::Cube), {"Cube", "Cube/Cube.gltf"} },
	{ size_t(Asset::Mesh::SpritePlane), {"SpritePlane", "SpritePlane/SpritePlane.gltf"} },
	{ size_t(Asset::Mesh::Helmet), {"Helmet", "Helmet/Helmet.gltf"} },
};

bool Asset::CheckValid(Sprite sprite)
{
	using Type = std::underlying_type_t<Sprite>;
	return (0 <= static_cast<Type>(sprite) && static_cast<Type>(sprite) < static_cast<Type>(Sprite::COUNT)) || sprite == Sprite::None;
}

std::string Asset::GetMeshPath(size_t i)
{
	auto iterator = meshInfos.find(i);
	if (iterator == meshInfos.end())
		return {};
	else
		return std::string(meshFolderPath) + std::string(iterator->second.relativePath);
}

std::optional<Asset::MeshDocument> Asset::LoadMeshDocument(std::string_view path)
{
	try
	{
		const auto gltfDocument = fx::gltf::LoadFromText(std::string(path));

		auto& primitive = gltfDocument.meshes[0].primitives[0];

		auto posAccIterator = primitive.attributes.find("POSITION");
		if (posAccIterator == primitive.attributes.end())
		{
			std::cerr << "Error. Found no position attribute for mesh." << std::endl;
			return {};
		}
		auto& posAcc = gltfDocument.accessors[posAccIterator->second];
		bool validAccessor = posAcc.type == fx::gltf::Accessor::Type::Vec3 && posAcc.componentType == fx::gltf::Accessor::ComponentType::Float;
		if (!validAccessor)
		{
			std::cerr << "Error. Position attribute of mesh is in wrong format." << std::endl;
			return {};
		}
		auto& posBufferView = gltfDocument.bufferViews[posAcc.bufferView];
		const auto posByteOffset = posAcc.byteOffset + posBufferView.byteOffset;
		

		auto& uvAccessor = gltfDocument.accessors[primitive.attributes.find("TEXCOORD_0")->second];
		assert(uvAccessor.type == fx::gltf::Accessor::Type::Vec2 && uvAccessor.componentType == fx::gltf::Accessor::ComponentType::Float);
		auto& uvBufferView = gltfDocument.bufferViews[uvAccessor.bufferView];

		

		auto& normalAccessor = gltfDocument.accessors[primitive.attributes.find("NORMAL")->second];
		assert(normalAccessor.type == fx::gltf::Accessor::Type::Vec3 && normalAccessor.componentType == fx::gltf::Accessor::ComponentType::Float);
		auto& normalBufferView = gltfDocument.bufferViews[normalAccessor.bufferView];
		

		assert(posAcc.count == uvAccessor.count && posAcc.count == normalAccessor.count);

		auto& indexAccessor = gltfDocument.accessors[primitive.indices];
		assert(indexAccessor.type == fx::gltf::Accessor::Type::Scalar);
		assert(indexAccessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort
			|| indexAccessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedInt);
		auto& indexBufferView = gltfDocument.bufferViews[indexAccessor.bufferView];

		
		MeshDocument::CreateInfo createInfo{};
		createInfo.byteArray = std::move(gltfDocument.buffers[posBufferView.buffer].data);
		createInfo.vertexCount = posAcc.count;
		createInfo.posByteOffset = posByteOffset;
		createInfo.uvByteOffset = uvBufferView.byteOffset + uvAccessor.byteOffset;
		createInfo.normalByteOffset = normalBufferView.byteOffset + normalAccessor.byteOffset;
		createInfo.indexByteOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
		createInfo.indexCount = indexAccessor.count;
		createInfo.indexType = indexAccessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort ? MeshDocument::IndexType::UInt16 : MeshDocument::IndexType::UInt32;

		return { MeshDocument(std::move(createInfo)) };
	}
	catch (std::exception e)
	{
		std::cerr << "Asset System Error: Could not load file. Message: " << e.what() << std::endl;
		return {};
	}
}

Asset::TextureDocument Asset::LoadTextureDocument(Sprite texture)
{
	TextureDocument::CreateInfo createInfo{};

	return TextureDocument(std::move(createInfo));
}

std::optional<Engine::Renderer::MeshDocument> Asset::LoadMesh(size_t i)
{
	auto path = GetMeshPath(i);
	if (path == "")
		return {};

	auto assetMeshDocOpt = LoadMeshDocument(path);
	if (assetMeshDocOpt.has_value() == false)
		return {};

	auto oldInfo = MeshDocument::ToCreateInfo(std::move(assetMeshDocOpt.value()));

	using namespace Engine;
	Renderer::MeshDocument::CreateInfo newInfo;
	newInfo.byteArray = std::move(oldInfo.byteArray);
	newInfo.vertexCount = std::move(oldInfo.vertexCount);
	newInfo.indexCount = std::move(oldInfo.indexCount);
	newInfo.indexType = oldInfo.indexType == MeshDocument::IndexType::UInt16 ? Renderer::MeshDocument::IndexType::UInt16 : Renderer::MeshDocument::IndexType::UInt32;

	newInfo.posByteOffset = std::move(oldInfo.posByteOffset);
	newInfo.uvByteOffset = std::move(oldInfo.uvByteOffset);
	newInfo.normalByteOffset = std::move(oldInfo.normalByteOffset);
	newInfo.tangentByteOffset = std::move(oldInfo.tangentByteOffset);
	newInfo.indexByteOffset = std::move(oldInfo.indexByteOffset);

	return Renderer::MeshDocument(std::move(newInfo));
}

static Asset::MeshDocument::IndexType ToIndexType(fx::gltf::Accessor::ComponentType componentType)
{
	switch (componentType)
	{
	case fx::gltf::Accessor::ComponentType::UnsignedShort:
		return Asset::MeshDocument::IndexType::UInt16;
	case fx::gltf::Accessor::ComponentType::UnsignedInt:
		return Asset::MeshDocument::IndexType::UInt32;
	default:
		return static_cast<Asset::MeshDocument::IndexType>(-1);
	}
}

Asset::MeshDocument::CreateInfo Asset::MeshDocument::ToCreateInfo(MeshDocument&& input)
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

Asset::MeshDocument::MeshDocument(CreateInfo&& info) :
	byteArray(std::move(info.byteArray)),
	indexType(info.indexType),
	vertexCount(info.vertexCount),
	indexCount(info.indexCount)
{
	GetByteOffset(Attribute::Position) = info.posByteOffset;
	GetByteOffset(Attribute::TexCoord) = info.uvByteOffset;
	GetByteOffset(Attribute::Normal) = info.normalByteOffset;
	GetByteOffset(Attribute::Tangent) = info.normalByteOffset;
	GetByteOffset(Attribute::Index) = info.indexByteOffset;
}

const std::vector<uint8_t>& Asset::MeshDocument::GetByteArray() const { return byteArray; }

Asset::MeshDocument::IndexType Asset::MeshDocument::GetIndexType() const { return indexType; }

const uint8_t* Asset::MeshDocument::GetDataPtr(Attribute type) const
{
	return byteArray.data() + byteOffsets.at(size_t(type));
}

size_t& Asset::MeshDocument::GetByteOffset(Attribute type) { return byteOffsets.at(size_t(type)); }

const size_t& Asset::MeshDocument::GetByteOffset(Attribute type) const { return byteOffsets.at(size_t(type)); }

uint8_t Asset::MeshDocument::ToByteSize(IndexType indexType)
{
	return indexType == IndexType::UInt16 ? uint8_t(2) : uint8_t(4);
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
