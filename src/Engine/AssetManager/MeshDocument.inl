#pragma once
#include "MeshDocument.hpp"

#include "fx/gltf.h"

namespace Engine
{
	namespace AssetManager
	{
		static MeshDocument::IndexType ToIndexType(fx::gltf::Accessor::ComponentType componentType)
		{
			switch (componentType)
			{
			case fx::gltf::Accessor::ComponentType::UnsignedShort:
				return MeshDocument::IndexType::UInt16;
			case fx::gltf::Accessor::ComponentType::UnsignedInt:
				return MeshDocument::IndexType::UInt32;
			default:
				assert(false);
				return static_cast<MeshDocument::IndexType>(-1);
			}
		}

		std::optional<MeshDocument> LoadMeshDocument(std::string_view path)
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
				auto & uvBufferView = gltfDocument.bufferViews[uvAccessor.bufferView];



				auto & normalAccessor = gltfDocument.accessors[primitive.attributes.find("NORMAL")->second];
				assert(normalAccessor.type == fx::gltf::Accessor::Type::Vec3 && normalAccessor.componentType == fx::gltf::Accessor::ComponentType::Float);
				auto & normalBufferView = gltfDocument.bufferViews[normalAccessor.bufferView];


				assert(posAcc.count == uvAccessor.count && posAcc.count == normalAccessor.count);

				auto& indexAccessor = gltfDocument.accessors[primitive.indices];
				assert(indexAccessor.type == fx::gltf::Accessor::Type::Scalar);
				assert(indexAccessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort
					|| indexAccessor.componentType == fx::gltf::Accessor::ComponentType::UnsignedInt);
				auto & indexBufferView = gltfDocument.bufferViews[indexAccessor.bufferView];


				MeshDocument::CreateInfo createInfo{};
				createInfo.byteArray = std::move(gltfDocument.buffers[posBufferView.buffer].data);
				createInfo.vertexCount = posAcc.count;
				createInfo.posByteOffset = posByteOffset;
				createInfo.uvByteOffset = uvBufferView.byteOffset + uvAccessor.byteOffset;
				createInfo.normalByteOffset = normalBufferView.byteOffset + normalAccessor.byteOffset;
				createInfo.indexByteOffset = indexBufferView.byteOffset + indexAccessor.byteOffset;
				createInfo.indexCount = indexAccessor.count;
				createInfo.indexType = ToIndexType(indexAccessor.componentType);

				return std::optional<MeshDocument>{ MeshDocument(std::move(createInfo)) };
			}
			catch (std::exception e)
			{
				std::cerr << "AssetManager System Error: Could not load file. Message: " << e.what() << std::endl;
				return {};
			}
		}

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