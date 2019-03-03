#include "Renderer.hpp"
#include "OpenGL.hpp"

#include "../Application.hpp"

#include "../Asset.hpp"

#include "GL/glew.h"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <iostream>
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

			void UpdateVBODatabase(Data& data, const std::vector<MeshID>& loadQueue);
			std::optional<VBO> VBOFromPath(const std::string& path);
			void UpdateIBODatabase(Data& data, const std::vector<SpriteID>& loadQueue);

			Data& GetData();
		}
	}
}

struct Engine::Renderer::OpenGL::VertexBufferObject
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

struct Engine::Renderer::OpenGL::ImageBufferObject
{
	void DeallocateDeviceBuffers();
	GLuint texture;
};

struct Engine::Renderer::OpenGL::Data
{
	Data() = default;
	Data(const Data&) = delete;
	Data(Data&&) = delete;

	std::unordered_map<MeshID, VBO> vboDatabase;
	VBO quadVBO;

	std::unordered_map<SpriteID, IBO> iboDatabase;

	GLuint program;
	GLint modelUniform;
	GLint viewProjectionUniform;
	GLint samplerUniform;

	GLuint samplerObject;
};

Engine::Renderer::OpenGL::Data& Engine::Renderer::OpenGL::GetData() { return *static_cast<Data*>(Core::GetAPIData()); }

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
		std::cerr << "Unable to load shader: " << fileName << std::endl;
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

		std::cerr << errorMessage << ": '" << error << "'" << std::endl;
	}
}

static GLuint CreateShader(const std::string& text, GLuint type)
{
	GLuint shader = glCreateShader(type);

	if (shader == 0)
		std::cerr << "Error compiling shader type " << type << std::endl;

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

void Engine::Renderer::OpenGL::Initialize(void*& apiData)
{
	auto glInitResult = glewInit();
	assert(glInitResult == 0);

	apiData = new Data();
	Data& data = *static_cast<Data*>(apiData);

	// Gen sampler
	glGenSamplers(1, &data.samplerObject);
	glSamplerParameteri(data.samplerObject, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glSamplerParameteri(data.samplerObject, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glSamplerParameteri(data.samplerObject, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(data.samplerObject, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load sprite VBO
	auto quadVBOOpt = VBOFromPath(static_cast<std::string>(Renderer::Setup::assetPath) + "/Meshes/SpritePlane/SpritePlane.gltf");
	if (quadVBOOpt.has_value())
		data.quadVBO = quadVBOOpt.value();
	else
		assert(false);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Create shader
	data.program = glCreateProgram();

	std::array<GLuint, 2> shaders{};
	shaders[0] = CreateShader(LoadShader("Data/Shaders/Sprite/OpenGL/sprite.vert"), GL_VERTEX_SHADER);
	shaders[1] = CreateShader(LoadShader("Data/Shaders/Sprite/OpenGL/sprite.frag"), GL_FRAGMENT_SHADER);

	for (unsigned int i = 0; i < 2; i++)
		glAttachShader(data.program, shaders[i]);

	glBindAttribLocation(data.program, 0, "position");
	glBindAttribLocation(data.program, 1, "texCoord");
	glBindAttribLocation(data.program, 2, "normal");

	glLinkProgram(data.program);
	CheckShaderError(data.program, GL_LINK_STATUS, true, "Error linking shader program");

	glValidateProgram(data.program);
	CheckShaderError(data.program, GL_LINK_STATUS, true, "Invalid shader program");

	for (unsigned int i = 0; i < 2; i++)
		glDeleteShader(shaders[i]);

	// Grab uniforms
	data.modelUniform = glGetUniformLocation(GetData().program, "model");
	data.viewProjectionUniform = glGetUniformLocation(GetData().program, "viewProjection");

	data.samplerUniform = glGetUniformLocation(GetData().program, "sampler");
}

void Engine::Renderer::OpenGL::Terminate(void*& apiData)
{
	GetData().quadVBO.DeallocateDeviceBuffers();
	for (auto& vboItem : GetData().vboDatabase)
		vboItem.second.DeallocateDeviceBuffers();

	for (auto& iboItem : GetData().vboDatabase)
		iboItem.second.DeallocateDeviceBuffers();

	glDeleteProgram(GetData().program);

	delete static_cast<Data*>(apiData);
	apiData = nullptr;
}

void Engine::Renderer::OpenGL::PrepareRenderingEarly(const std::vector<SpriteID>& spriteLoadQueue)
{
	UpdateIBODatabase(GetData(), spriteLoadQueue);
}

void Engine::Renderer::OpenGL::PrepareRenderingLate()
{
}

void Engine::Renderer::OpenGL::Draw()
{
	Data& data = GetData();

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto& viewport = Renderer::GetViewport(0);

	// Sprite render pass
	const auto& sprites = Core::GetRenderGraph().sprites;
	if (!sprites.empty())
	{
		glUseProgram(data.program);

		auto viewMat = Core::GetCameraInfo().GetModel(viewport.GetDimensions().GetAspectRatio());
		glProgramUniformMatrix4fv(data.program, data.viewProjectionUniform, 1, GL_FALSE, viewMat.data.data());

		glProgramUniform1ui(data.program, data.samplerUniform, 0);
		glBindSampler(0, data.samplerObject);

		const auto& spriteVBO = data.quadVBO;
		glBindVertexArray(spriteVBO.vertexArrayObject);

		const auto& spriteTransforms = Core::GetRenderGraphTransform().sprites;
		for (size_t i = 0; i < sprites.size(); i++)
		{
			glProgramUniformMatrix4fv(GetData().program, GetData().modelUniform, 1, GL_FALSE, spriteTransforms[i].data.data());

			glActiveTexture(GL_TEXTURE0);
			const auto& ibo = data.iboDatabase[sprites[i]];
			glBindTexture(GL_TEXTURE_2D, ibo.texture);

			glDrawElements(GL_TRIANGLES, spriteVBO.numIndices, spriteVBO.indexType, 0);
		}
	}

	Engine::Application::Core::GL_SwapWindow(viewport.GetSurfaceHandle());
}

void Engine::Renderer::OpenGL::UpdateVBODatabase(Data& data, const std::vector<MeshID>& loadQueue)
{
	for (const auto& id : loadQueue)
	{
		VBO vbo;


		data.vboDatabase.insert({ id, vbo });
	}
}

void Engine::Renderer::OpenGL::UpdateIBODatabase(Data& data, const std::vector<SpriteID>& loadQueue)
{
	glActiveTexture(GL_TEXTURE0);
	for (const auto& id : loadQueue)
	{
		auto texDocument = Asset::LoadTextureDocument(static_cast<Asset::Sprite>(id));

		IBO ibo;

		glGenTextures(1, &ibo.texture);
		glBindTexture(GL_TEXTURE_2D, ibo.texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texDocument.GetDimensions().width, texDocument.GetDimensions().height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texDocument.GetData());

		data.iboDatabase.insert({ id, ibo });
	}
}

void Engine::Renderer::OpenGL::VBO::DeallocateDeviceBuffers()
{
	glDeleteBuffers(static_cast<GLint>(attributeBuffers.size()), attributeBuffers.data());
	glDeleteVertexArrays(1, &vertexArrayObject);
}

void Engine::Renderer::OpenGL::IBO::ImageBufferObject::DeallocateDeviceBuffers()
{
	glDeleteTextures(1, &texture);
}

std::optional<Engine::Renderer::OpenGL::VBO> Engine::Renderer::OpenGL::VBOFromPath(const std::string& path)
{
	using namespace Asset;
	auto meshDocument = LoadMeshDocument(path);

	VBO vbo;

	vbo.indexType = meshDocument.GetIndexType() == MeshDocument::IndexType::Uint16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
	vbo.numIndices = meshDocument.GetIndexCount();

	glGenVertexArrays(1, &vbo.vertexArrayObject);
	glBindVertexArray(vbo.vertexArrayObject);

	glGenBuffers(static_cast<GLint>(vbo.attributeBuffers.size()), vbo.attributeBuffers.data());

	glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[static_cast<size_t>(VBO::Attribute::Position)]);
	glBufferData(GL_ARRAY_BUFFER, meshDocument.GetData(MeshDocument::Attribute::Position).byteLength, meshDocument.GetDataPtr(Asset::MeshDocument::Attribute::Position), GL_STATIC_DRAW);
	glEnableVertexAttribArray(static_cast<size_t>(VBO::Attribute::Position));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[static_cast<size_t>(VBO::Attribute::TexCoord)]);
	glBufferData(GL_ARRAY_BUFFER, meshDocument.GetData(MeshDocument::Attribute::TexCoord).byteLength, meshDocument.GetDataPtr(Asset::MeshDocument::Attribute::TexCoord), GL_STATIC_DRAW);
	glEnableVertexAttribArray(static_cast<size_t>(VBO::Attribute::TexCoord));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[static_cast<size_t>(VBO::Attribute::Normal)]);
	glBufferData(GL_ARRAY_BUFFER, meshDocument.GetData(MeshDocument::Attribute::Normal).byteLength, meshDocument.GetDataPtr(Asset::MeshDocument::Attribute::Normal), GL_STATIC_DRAW);
	glEnableVertexAttribArray(static_cast<size_t>(VBO::Attribute::Normal));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.attributeBuffers[static_cast<size_t>(VBO::Attribute::Index)]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshDocument.GetData(Asset::MeshDocument::Attribute::Index).byteLength, meshDocument.GetDataPtr(Asset::MeshDocument::Attribute::Index), GL_STATIC_DRAW);

	return vbo;
}