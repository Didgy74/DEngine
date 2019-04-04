#include "Renderer.hpp"
#include "RendererData.hpp"
#include "MeshDocument.hpp"
#include "TextureDocument.hpp"
#include "OpenGL.hpp"

#include "GL/glew.h"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <array>
#include <optional>

namespace Engine
{
	namespace Renderer
	{
		namespace OpenGL
		{
			struct VertexBufferObject;
			using VBO = VertexBufferObject;

			struct ImageBufferObject;
			using IBO = ImageBufferObject;

			struct Data;
			struct CameraDataUBO;
			struct LightDataUBO;

			void UpdateVBODatabase(Data& data, const std::vector<MeshID>& loadQueue);
			void UpdateIBODatabase(Data& data, const std::vector<SpriteID>& loadQueue);
			std::optional<VBO> GetVBOFromID(size_t id);
			std::optional<IBO> GetIBOFromID(size_t id);

			Data& GetAPIData();

			void Draw_SpritePass(const Data& data, const std::vector<SpriteID>& sprites, const std::vector<Math::Matrix4x4>& transforms);
			void Draw_MeshPass(const Data& data, const std::vector<MeshID>& meshes, const std::vector<Math::Matrix4x4>& transforms);

			void CreateStandardUBOs(Data& data);
			void LoadSpriteShader(Data& data);
			void LoadMeshShader(Data& data);

			void GLDebugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

			GLint ToGLType(MeshDocument::IndexType indexType);
			GLint ToGLFormat(TextureDocument::Format format);
			GLint ToGLType(TextureDocument::Type type);
		}
	}

	struct Renderer::OpenGL::CameraDataUBO
	{
		Math::Vector3D wsPosition;
		float padding1;
		Math::Matrix<4, 4, float> viewProjection;
	};

	struct Renderer::OpenGL::LightDataUBO
	{
		Math::Vector4D ambientLight;
		uint32_t pointLightCount;
		std::array<uint32_t, 3> padding1;
		std::array<Math::Vector4D, 10> pointLightIntensity;
		std::array<Math::Vector4D, 10> pointLightPos;
	};

	struct Renderer::OpenGL::VertexBufferObject
	{
		enum class Attribute
		{
			Position,
			TexCoord,
			Normal,
			Index,
			COUNT
		};

		void DeallocateDeviceBuffers();

		GLuint vertexArrayObject;
		std::array<GLuint, static_cast<size_t>(Attribute::COUNT)> attributeBuffers;
		uint32_t numIndices;
		GLenum indexType;
	};

	struct Renderer::OpenGL::ImageBufferObject
	{
		void DeallocateDeviceBuffers();
		GLuint texture;
	};

	struct Renderer::OpenGL::Data
	{
		std::function<void(void*)> glSwapBuffers;

		std::unordered_map<MeshID, VBO> vboDatabase;
		VBO quadVBO;

		std::unordered_map<SpriteID, IBO> iboDatabase;

		GLuint cameraDataUBO;
		GLuint lightDataUBO;
		GLuint samplerObject;

		GLuint spriteProgram;
		GLint spriteModelUniform;
		GLint spriteSamplerUniform;

		GLuint meshProgram;
		GLint meshModelUniform;

		IBO testIBO;
		GLint meshTextureUniform;
	};

	Renderer::OpenGL::Data& Renderer::OpenGL::GetAPIData() { return std::any_cast<Data&>(Core::GetAPIData()); }

	static std::string LoadShader(const std::string& fileName)
	{
		std::ifstream file;
		file.open((fileName).c_str());

		std::string output;
		std::string line;

		if (file.is_open())
		{
			while (file.good())
			{
				getline(file, line);
				output.append(line + "\n");
			}
		}
		else
		{
			Renderer::LogDebugMessage("Unable to load shader : " + fileName);
		}

		return output;
	}

	static void CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage)
	{
		GLint success = 0;
		GLchar error[1024] = { 0 };

		if (isProgram)
			glGetProgramiv(shader, flag, &success);
		else
			glGetShaderiv(shader, flag, &success);

		if (success == GL_FALSE)
		{
			if (isProgram)
				glGetProgramInfoLog(shader, sizeof(error), NULL, error);
			else
				glGetShaderInfoLog(shader, sizeof(error), NULL, error);

			Renderer::LogDebugMessage(errorMessage + ": '" + error);
		}
	}

	static GLuint CreateShader(const std::string& text, GLuint type)
	{
		GLuint shader = glCreateShader(type);

		if (shader == 0)
			Renderer::LogDebugMessage("Error compiling shader type " + type);

		const GLchar* p[1];
		p[0] = text.c_str();
		GLint lengths[1];
		lengths[0] = static_cast<GLint>(text.length());

		glShaderSource(shader, 1, p, lengths);
		glCompileShader(shader);

		std::string test = "Error compiling ";
		if (type == GL_VERTEX_SHADER)
			test += "vertex";
		else if (type == GL_FRAGMENT_SHADER)
			test += "fragment";
		test += " shader";
		CheckShaderError(shader, GL_COMPILE_STATUS, false, test);

		return shader;
	}

	void Renderer::OpenGL::LoadSpriteShader(Data& data)
	{
		// Create shader
		data.spriteProgram = glCreateProgram();

		std::array<GLuint, 2> shaders{};
		shaders[0] = CreateShader(LoadShader("Data/Shaders/Sprite/sprite.vert"), GL_VERTEX_SHADER);
		shaders[1] = CreateShader(LoadShader("Data/Shaders/Sprite/sprite.frag"), GL_FRAGMENT_SHADER);

		for (unsigned int i = 0; i < 2; i++)
			glAttachShader(data.spriteProgram, shaders[i]);

		glLinkProgram(data.spriteProgram);
		CheckShaderError(data.spriteProgram, GL_LINK_STATUS, true, "Error linking shader program");

		glValidateProgram(data.spriteProgram);
		CheckShaderError(data.spriteProgram, GL_LINK_STATUS, true, "Invalid shader program");

		for (unsigned int i = 0; i < 2; i++)
			glDeleteShader(shaders[i]);

		// Grab uniforms
		data.spriteModelUniform = glGetUniformLocation(data.spriteProgram, "model");

		data.spriteSamplerUniform = glGetUniformLocation(data.spriteProgram, "sampler");
	}

	void Renderer::OpenGL::LoadMeshShader(Engine::Renderer::OpenGL::Data &data)
	{
		// Create shader
		data.meshProgram = glCreateProgram();

		std::array<GLuint, 2> shader{};
		shader[0] = CreateShader(LoadShader("Data/Shaders/Mesh/Mesh.vert"), GL_VERTEX_SHADER);
		shader[1] = CreateShader(LoadShader("Data/Shaders/Mesh/Mesh.frag"), GL_FRAGMENT_SHADER);

		for (unsigned int i = 0; i < 2; i++)
			glAttachShader(data.meshProgram, shader[i]);

		glLinkProgram(data.meshProgram);
		CheckShaderError(data.meshProgram, GL_LINK_STATUS, true, "Error linking shader program");

		glValidateProgram(data.meshProgram);
		CheckShaderError(data.meshProgram, GL_LINK_STATUS, true, "Invalid shader program");

		for (unsigned int i = 0; i < 2; i++)
			glDeleteShader(shader[i]);

		// Grab uniforms
		data.meshModelUniform = glGetUniformLocation(data.meshProgram, "model");
		data.meshTextureUniform = glGetUniformLocation(data.meshProgram, "texture");


		glUniformBlockBinding(data.meshProgram, 1, 1);
	}

	void Renderer::OpenGL::CreateStandardUBOs(Data& data)
	{
		std::array<GLuint, 2> buffers;
		glGenBuffers(2, buffers.data());

		data.cameraDataUBO = buffers[0];
		data.lightDataUBO = buffers[1];

		// Make camera ubo
		glBindBuffer(GL_UNIFORM_BUFFER, data.cameraDataUBO);
		glNamedBufferData(data.cameraDataUBO, sizeof(CameraDataUBO), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, data.cameraDataUBO, 0, sizeof(CameraDataUBO));

		// Make light ubo
		glBindBuffer(GL_UNIFORM_BUFFER, data.lightDataUBO);
		glNamedBufferData(data.lightDataUBO, sizeof(LightDataUBO), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferRange(GL_UNIFORM_BUFFER, 1, data.lightDataUBO, 0, sizeof(LightDataUBO));
	}

	void Renderer::OpenGL::GLDebugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
	{
		LogDebugMessage(message);
	}

	void Renderer::OpenGL::Initialize(std::any& apiData, const InitInfo& createInfo)
	{
		auto glInitResult = glewInit();
		assert(glInitResult == 0);

		apiData = std::make_any<Data>();
		Data& data = std::any_cast<Data&>(apiData);

		data.glSwapBuffers = std::move(createInfo.glSwapBuffers);

		// Initialize debug stuff
		if constexpr (Setup::enableDebugging)
		{
			if (Core::GetData().debugData.useDebugging)
			{
				GLint flags;
				glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
				if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
				{
					glEnable(GL_DEBUG_OUTPUT);
					glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
					glDebugMessageCallback(&GLDebugOutputCallback, nullptr);
					glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
				}
				else
					LogDebugMessage("Error. Couldn't make GL Debug Output");
			}
		}

		CreateStandardUBOs(data);

		// Gen sampler
		glGenSamplers(1, &data.samplerObject);
		glSamplerParameteri(data.samplerObject, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glSamplerParameteri(data.samplerObject, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glSamplerParameteri(data.samplerObject, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glSamplerParameteri(data.samplerObject, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindSampler(0, data.samplerObject);

		glEnable(GL_DEPTH_TEST);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		data.testIBO = GetIBOFromID(2).value();


		//LoadSpriteShader(data);
		LoadMeshShader(data);
	}

	void Renderer::OpenGL::Terminate(std::any& apiData)
	{
		Data& data = std::any_cast<Data&>(apiData);

		data.quadVBO.DeallocateDeviceBuffers();
		for (auto& vboItem : data.vboDatabase)
			vboItem.second.DeallocateDeviceBuffers();

		for (auto& iboItem : data.vboDatabase)
			iboItem.second.DeallocateDeviceBuffers();

		glDeleteProgram(data.spriteProgram);
		glDeleteProgram(data.meshProgram);

		//delete static_cast<Data*>(apiData);
		//apiData = nullptr;
	}

	void Renderer::OpenGL::PrepareRenderingEarly(const std::vector<SpriteID>& spriteLoadQueue, const std::vector<MeshID>& meshLoadQueue)
	{
		Data& data = GetAPIData();

		UpdateIBODatabase(data, spriteLoadQueue);
		UpdateVBODatabase(data, meshLoadQueue);

		const auto& renderGraph = Core::GetRenderGraph();

		// Update ambient light
		constexpr GLintptr ambientLightOffset = offsetof(LightDataUBO, LightDataUBO::ambientLight);
		glNamedBufferSubData(data.lightDataUBO, ambientLightOffset, sizeof(renderGraph.ambientLight), renderGraph.ambientLight.Data());

		// Update light count
		auto pointLightCount = static_cast<uint32_t>(renderGraph.pointLights.size());
		constexpr GLintptr pointLightCountDataOffset = offsetof(LightDataUBO, LightDataUBO::pointLightCount);
		glNamedBufferSubData(data.lightDataUBO, pointLightCountDataOffset, sizeof(pointLightCount), &pointLightCount);

		// Update intensities
		const size_t elements = Math::Min(10, renderGraph.pointLights.size());
		std::array<Math::Vector4D, 10> intensityData;
		for (size_t i = 0; i < elements; i++)
			intensityData[i] = renderGraph.pointLights[i].AsVec4();
		size_t byteLength = sizeof(Math::Vector4D) * elements;
		constexpr GLintptr pointLightIntensityOffset = offsetof(LightDataUBO, LightDataUBO::pointLightIntensity);
		glNamedBufferSubData(data.lightDataUBO, pointLightIntensityOffset, byteLength, intensityData.data());
	}

	void Renderer::OpenGL::PrepareRenderingLate()
	{
		Data& data = GetAPIData();

		const auto& renderGraphTransform = Core::GetRenderGraphTransform();

		// Update lights positions
		const size_t elements = Math::Min(10, renderGraphTransform.pointLights.size());
		std::array<Math::Vector4D, 10> posData;
		for (size_t i = 0; i < elements; i++)
			posData[i] = renderGraphTransform.pointLights[i].AsVec4();
		size_t byteLength = sizeof(Math::Vector4D) * elements;
		constexpr GLintptr pointLightPosDataOffset = offsetof(LightDataUBO, LightDataUBO::pointLightPos);
		glNamedBufferSubData(data.lightDataUBO, pointLightPosDataOffset, byteLength, posData.data());

		auto& viewport = Renderer::GetViewport(0);

		// Update camera UBO
		auto& cameraInfo = Renderer::Core::GetCameraInfo();
		const Math::Vector3D& cameraWSPosition = cameraInfo.worldSpacePos;
		constexpr GLintptr cameraWSPositionDataOffset = offsetof(CameraDataUBO, CameraDataUBO::wsPosition);
		glBindBuffer(GL_UNIFORM_BUFFER, data.cameraDataUBO);
		glBufferSubData(GL_UNIFORM_BUFFER, cameraWSPositionDataOffset, sizeof(cameraWSPosition), cameraWSPosition.Data());

		auto viewMatrix = cameraInfo.GetModel(viewport.GetDimensions().GetAspectRatio());
		constexpr GLintptr cameraViewProjectionDataOffset = offsetof(CameraDataUBO, CameraDataUBO::viewProjection);
		glBufferSubData(GL_UNIFORM_BUFFER, cameraViewProjectionDataOffset, sizeof(viewMatrix), viewMatrix.Data());
	}

	void Renderer::OpenGL::Draw()
	{
		Data& data = GetAPIData();

		glClearColor(0.25f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		const auto& renderGraph = Core::GetRenderGraph();
		const auto& renderGraphTransform = Core::GetRenderGraphTransform();

		Draw_SpritePass(data, renderGraph.sprites, renderGraphTransform.sprites);
		Draw_MeshPass(data, renderGraph.meshes, renderGraphTransform.meshes);

		auto& viewport = Renderer::GetViewport(0);

		data.glSwapBuffers(viewport.GetSurfaceHandle());
	}

	void Renderer::OpenGL::Draw_SpritePass(const Data& data,
										   const std::vector<SpriteID>& sprites,
										   const std::vector<Math::Matrix4x4>& transforms)
	{
		auto& viewport = Renderer::GetViewport(0);

		// Sprite render pass
		if (!sprites.empty())
		{
			glUseProgram(data.spriteProgram);

			glProgramUniform1ui(data.spriteProgram, data.spriteSamplerUniform, 0);
			glBindSampler(0, data.samplerObject);

			const auto& spriteVBO = data.quadVBO;
			glBindVertexArray(spriteVBO.vertexArrayObject);

			for (size_t i = 0; i < sprites.size(); i++)
			{
				glProgramUniformMatrix4fv(data.spriteProgram, data.spriteModelUniform, 1, GL_FALSE, transforms[i].data.data());

				glActiveTexture(GL_TEXTURE0);
				const IBO& ibo = data.iboDatabase.at(sprites[i]);
				glBindTexture(GL_TEXTURE_2D, ibo.texture);

				glDrawElements(GL_TRIANGLES, spriteVBO.numIndices, spriteVBO.indexType, nullptr);
			}
		}
	}

	void Renderer::OpenGL::Draw_MeshPass(const Data &data,
										 const std::vector<Engine::Renderer::MeshID> &meshes,
									     const std::vector<Math::Matrix4x4> &transforms)
	{
		auto& viewport = Renderer::GetViewport(0);

		if (!meshes.empty())
		{
			glUseProgram(data.meshProgram);

			glUniform1ui(data.meshTextureUniform, 0);

			glActiveTexture(GL_TEXTURE0);

			glBindTexture(GL_TEXTURE_2D, data.testIBO.texture);

			for (size_t i = 0; i < meshes.size(); i++)
			{
				glProgramUniformMatrix4fv(data.meshProgram, data.meshModelUniform, 1, GL_FALSE, transforms[i].data.data());

				const VBO& vbo = data.vboDatabase.at(meshes[i]);
				glBindVertexArray(vbo.vertexArrayObject);

				glDrawElements(GL_TRIANGLES, vbo.numIndices, vbo.indexType, nullptr);
			}
		}
	}

	void Renderer::OpenGL::UpdateVBODatabase(Data& data, const std::vector<MeshID>& loadQueue)
	{
		for (const auto& id : loadQueue)
		{
			auto vboOpt = GetVBOFromID(static_cast<size_t>(id));
			assert(vboOpt.has_value());

			data.vboDatabase.insert({ id, vboOpt.value() });
		}
	}

	void Renderer::OpenGL::UpdateIBODatabase(Data& data, const std::vector<SpriteID>& loadQueue)
	{
		glActiveTexture(GL_TEXTURE0);
		for (const auto& id : loadQueue)
		{
			auto iboOpt = GetIBOFromID(static_cast<size_t>(id));
			assert(iboOpt.has_value());

			data.iboDatabase.insert({ id, iboOpt.value() });
		}
	}

	std::optional<Renderer::OpenGL::IBO> Renderer::OpenGL::GetIBOFromID(size_t id)
	{
		auto texDocumentOpt = Core::GetData().assetLoadData.textureLoader(id);
		assert(texDocumentOpt.has_value());

		auto& texDoc = texDocumentOpt.value();

		IBO ibo;

		glGenTextures(1, &ibo.texture);
		glBindTexture(GL_TEXTURE_2D, ibo.texture);

		GLint internalFormat = ToGLFormat(texDoc.GetInternalFormat());
		GLint baseInternalFormat = ToGLFormat(texDoc.GetBaseInternalFormat());
		GLsizei width = texDoc.GetDimensions()[0];
		GLsizei height = texDoc.GetDimensions()[1];
		GLint type = ToGLType(texDoc.GetType());
		GLsizei byteLength = texDoc.GetByteLength();
		const uint8_t* data = texDoc.GetData();

		if (texDoc.IsCompressed())
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, byteLength, data);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, baseInternalFormat, width, height, 0, baseInternalFormat, type, data);

		return ibo;
	}

	void Renderer::OpenGL::VBO::DeallocateDeviceBuffers()
	{
		glDeleteBuffers(static_cast<GLint>(attributeBuffers.size()), attributeBuffers.data());
		glDeleteVertexArrays(1, &vertexArrayObject);
	}

	void Renderer::OpenGL::IBO::ImageBufferObject::DeallocateDeviceBuffers()
	{
		glDeleteTextures(1, &texture);
	}

	std::optional<Renderer::OpenGL::VBO> Renderer::OpenGL::GetVBOFromID(size_t id)
	{
		const auto meshDocumentOpt = Core::GetData().assetLoadData.meshLoader(id);

		assert(meshDocumentOpt.has_value());

		const auto& meshDocument = meshDocumentOpt.value();

		VBO vbo;

		vbo.indexType = ToGLType(meshDocument.GetIndexType());
		vbo.numIndices = meshDocument.GetIndexCount();

		glGenVertexArrays(1, &vbo.vertexArrayObject);
		glBindVertexArray(vbo.vertexArrayObject);

		glGenBuffers(GLint(vbo.attributeBuffers.size()), vbo.attributeBuffers.data());

		glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[size_t(VBO::Attribute::Position)]);
		glBufferData(GL_ARRAY_BUFFER, meshDocument.GetByteLength(MeshDocument::Attribute::Position), meshDocument.GetDataPtr(MeshDocument::Attribute::Position), GL_STATIC_DRAW);
		glEnableVertexAttribArray(size_t(VBO::Attribute::Position));
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[size_t(VBO::Attribute::TexCoord)]);
		glBufferData(GL_ARRAY_BUFFER, meshDocument.GetByteLength(MeshDocument::Attribute::TexCoord), meshDocument.GetDataPtr(MeshDocument::Attribute::TexCoord), GL_STATIC_DRAW);
		glEnableVertexAttribArray(size_t(VBO::Attribute::TexCoord));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[size_t(VBO::Attribute::Normal)]);
		glBufferData(GL_ARRAY_BUFFER, meshDocument.GetByteLength(MeshDocument::Attribute::Normal), meshDocument.GetDataPtr(MeshDocument::Attribute::Normal), GL_STATIC_DRAW);
		glEnableVertexAttribArray(size_t(VBO::Attribute::Normal));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.attributeBuffers[size_t(VBO::Attribute::Index)]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshDocument.GetByteLength(MeshDocument::Attribute::Index), meshDocument.GetDataPtr(MeshDocument::Attribute::Index), GL_STATIC_DRAW);

		return vbo;
	}

	GLint Renderer::OpenGL::ToGLType(MeshDocument::IndexType indexType)
	{
		using IndexType = MeshDocument::IndexType;
		switch (indexType)
		{
		case IndexType::UInt16:
			return GL_UNSIGNED_SHORT;
		case IndexType::UInt32:
			return GL_UNSIGNED_INT;
		default:
			return 0;
		}
	}

	GLint Renderer::OpenGL::ToGLFormat(TextureDocument::Format format)
	{
		using Format = TextureDocument::Format;

		// Uncompressed
		switch (format)
		{
		case Format::RGB:
			return GL_RGB;
		case Format::RGBA:
			return GL_RGBA;
		case Format::R8G8B8A8:
			return GL_RGBA8;
		};

		// DXT
		switch (format)
		{
		case Format::Compressed_RGB_S3TC_DXT1_ANGLE:
			return GL_COMPRESSED_RGB_S3TC_DXT1_ANGLE;
		case Format::Compressed_RGBA_S3TC_DXT1_ANGLE:
			return GL_COMPRESSED_RGBA_S3TC_DXT1_ANGLE;
		case Format::Compressed_RGBA_S3TC_DXT3_ANGLE:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE;
		case Format::Compressed_RGBA_S3TC_DXT5_ANGLE:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE;
		}

		switch (format)
		{
		case Format::Compressed_RGBA_BPTC_UNORM:
			return GL_COMPRESSED_RGBA_BPTC_UNORM;
		case Format::Compressed_SRGB_ALPHA_BPTC_UNORM:
			return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
		case Format::Compressed_RGB_BPTC_SIGNED_FLOAT:
			return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
		case Format::Compressed_RGB_BPTC_UNSIGNED_FLOAT:
			return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
		}

		return 0;
	}

	GLint Renderer::OpenGL::ToGLType(TextureDocument::Type type)
	{
		using Type = TextureDocument::Type;
		switch (type)
		{
		case Type::UnsignedByte:
			return GL_UNSIGNED_BYTE;
		default:
			return 0;
		}
	}
}