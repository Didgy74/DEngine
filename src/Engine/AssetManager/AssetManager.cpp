#include "DEngine/AssetManager/AssetManager.hpp"

#include <cassert>
#include <map>
#include <type_traits>
#include <iostream>
#include <string_view>
#include <memory>

#include "fx/gltf.h"

namespace Engine
{
	namespace AssetManager
	{
		namespace Core
		{
			struct Data
			{
				size_t meshIDCounter = 0;
				std::unordered_map<size_t, MeshInfo> meshAssetDatabase;

				size_t textureIDCounter = 0;
				std::unordered_map<size_t, TextureInfo> textureAssetDatabase;

				std::map<std::string, fx::gltf::Document> openGLTFDocs;
			};

			static std::unique_ptr<Data> data;
		}
	}
}

namespace Engine
{
	namespace AssetManager
	{
		bool MeshInfo::operator==(const MeshInfo& right) const
		{
			return path == right.path && meshIndex == right.meshIndex && primitiveIndex == right.primitiveIndex;
		}

		bool TextureInfo::operator==(const TextureInfo& right) const
		{
			return path == right.path;
		}

		MeshInfo GetMeshInfo(size_t i)
		{
			assert(Core::data && "Error. Cannot call AssetManager::GetMeshInfo without having initialized AssetManager.");
			auto meshMap = Core::data->meshAssetDatabase;
			auto iterator = meshMap.find(i);
			if (iterator == meshMap.end())
				return {};
			else
				return iterator->second;
		}

		size_t AddMesh(MeshInfo&& input)
		{
			Core::Data& data = *Core::data;

			for (const auto& item : data.meshAssetDatabase)
			{
				if (item.second == input)
					return item.first;
			}

			size_t returnValue = data.meshIDCounter;
			data.meshAssetDatabase.insert({ data.meshIDCounter, std::move(input) });
			data.meshIDCounter++;
			return returnValue;
		}

		TextureInfo GetTextureInfo(size_t i)
		{
			auto textureMap = Core::data->textureAssetDatabase;
			auto iterator = textureMap.find(i);
			if (iterator == textureMap.end())
				return {};
			else
				return iterator->second;
		}

		size_t AddTexture(TextureInfo&& input)
		{
			Core::Data& data = *Core::data;

			for (const auto& item : data.textureAssetDatabase)
			{
				if (item.second == input)
					return item.first;
			}

			size_t returnValue = data.textureIDCounter;
			data.textureAssetDatabase.insert({ data.textureIDCounter, std::move(input) });
			data.textureIDCounter++;
			return returnValue;
		}

		std::optional<Renderer::MeshDoc> GetRendererMeshDoc(size_t id)
		{
			auto meshInfo = GetMeshInfo(id);
			if (meshInfo.path == "")
				return {};

			auto gltfDocIterator = Core::data->openGLTFDocs.find(meshInfo.path);
			if (gltfDocIterator == Core::data->openGLTFDocs.end())
			{
				// Load new gltfDoc
				auto insertResult = Core::data->openGLTFDocs.insert({ meshInfo.path, fx::gltf::LoadFromText(meshInfo.path) });
				assert(insertResult.second);
				gltfDocIterator = insertResult.first;
			}

			Renderer::MeshDoc::CreateInfo meshDoc;

			const auto& gltfDoc = gltfDocIterator->second;

			const auto& primitive = gltfDoc.meshes[meshInfo.meshIndex].primitives[meshInfo.primitiveIndex];

			// Position info
			auto& posAcc = gltfDoc.accessors[primitive.attributes.at("POSITION")];
			assert(posAcc.type == fx::gltf::Accessor::Type::Vec3 && "Error. Model's position attribute must be a Vec3.");
			assert(posAcc.componentType == fx::gltf::Accessor::ComponentType::Float && "Error. Model's position attribute must consist of floats.");
			auto& posBufferView = gltfDoc.bufferViews[posAcc.bufferView];

			meshDoc.byteArrayRef = reinterpret_cast<const std::byte*>(gltfDoc.buffers[posBufferView.buffer].data.data());
			meshDoc.vertexCount = posAcc.count;

			meshDoc.posByteOffset = posAcc.byteOffset + posBufferView.byteOffset;

			// TexCoord info
			auto& uvAcc = gltfDoc.accessors[primitive.attributes.at("TEXCOORD_0")];
			assert(uvAcc.type == fx::gltf::Accessor::Type::Vec2 && "Error. Model's TexCoord attribute must be a Vec2.");
			assert(uvAcc.componentType == fx::gltf::Accessor::ComponentType::Float && "Error. Model's TexCoord attribute must consist of floats.");
			auto& uvBufferView = gltfDoc.bufferViews[uvAcc.bufferView];
			meshDoc.uvByteOffset = uvAcc.byteOffset + uvBufferView.byteOffset;

			// Normal info
			auto& normalAcc = gltfDoc.accessors[primitive.attributes.at("NORMAL")];
			assert(normalAcc.type == fx::gltf::Accessor::Type::Vec3 && "Error. Model's Normal attribute must be a Vec3.");
			assert(normalAcc.componentType == fx::gltf::Accessor::ComponentType::Float && "Error. Model's Normal attribute must consist of floats.");
			auto& normalBufferView = gltfDoc.bufferViews[normalAcc.bufferView];
			meshDoc.normalByteOffset = normalAcc.byteOffset + normalBufferView.byteOffset;

			/*
			// Tangent info
			auto& tangentAcc = gltfDoc.accessors[primitive.attributes.at("TANGENT")];
			assert(tangentAcc.type == fx::gltf::Accessor::Type::Vec3 && "Error. Model's Tangent attribute must be a Vec3.");
			assert(tangentAcc.componentType == fx::gltf::Accessor::ComponentType::Float && "Error. Model's Tangent attribute must consist of floats.");
			auto& tangentBufferView = gltfDoc.bufferViews[tangentAcc.bufferView];
			meshDoc.tangentByteOffset = tangentAcc.byteOffset + tangentBufferView.byteOffset;
			*/

			// Index info
			auto& indexAcc = gltfDoc.accessors[primitive.indices];
			assert(indexAcc.type == fx::gltf::Accessor::Type::Scalar && "Error. Model's index attribute must be a scalar.");
			assert((indexAcc.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort 
				|| indexAcc.componentType == fx::gltf::Accessor::ComponentType::UnsignedInt) 
				&& "Error. Model's Index attribute must consist of shorts or ints.");
			auto& indexBufferView = gltfDoc.bufferViews[indexAcc.bufferView];
			meshDoc.indexByteOffset = indexAcc.byteOffset + indexBufferView.byteOffset;

			meshDoc.indexCount = indexAcc.count;
			if (indexAcc.componentType == fx::gltf::Accessor::ComponentType::UnsignedShort)
				meshDoc.indexType = Renderer::MeshDoc::IndexType::UInt16;
			else if (indexAcc.componentType == fx::gltf::Accessor::ComponentType::UnsignedInt)
				meshDoc.indexType = Renderer::MeshDoc::IndexType::UInt32;

			return Renderer::MeshDoc{ std::move(meshDoc) };
		}

		void Renderer_AssetLoadEndEvent()
		{
			Core::data->openGLTFDocs.clear();
		}

		namespace Core
		{
			void Initialize()
			{
				data = std::make_unique<Data>();
			}

			void Terminate()
			{
				data = nullptr;
			}
		}
	}
}