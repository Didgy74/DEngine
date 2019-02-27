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

			void UpdateVBODatabase();
			void UpdateIBODatabase();

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

	std::unordered_map<Renderer::AssetID, VBO> vboDatabase;

	std::unordered_map<Renderer::AssetID, IBO> iboDatabase;

	GLuint program;
	GLint modelUniform;
	GLint viewProjectionUniform;
	GLint test;
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

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	GetData().program = glCreateProgram();

	std::array<GLint, 2> shaders;
	shaders[0] = CreateShader(LoadShader("Data/Shaders/Sprite/OpenGL/sprite.vert"), GL_VERTEX_SHADER);
	shaders[1] = CreateShader(LoadShader("Data/Shaders/Sprite/OpenGL/sprite.frag"), GL_FRAGMENT_SHADER);

	for (unsigned int i = 0; i < 2; i++)
		glAttachShader(GetData().program, shaders[i]);

	glBindAttribLocation(GetData().program, 0, "position");
	glBindAttribLocation(GetData().program, 1, "texCoord");
	glBindAttribLocation(GetData().program, 2, "normal");

	glLinkProgram(GetData().program);
	CheckShaderError(GetData().program, GL_LINK_STATUS, true, "Error linking shader program");

	glValidateProgram(GetData().program);
	CheckShaderError(GetData().program, GL_LINK_STATUS, true, "Invalid shader program");

	for (unsigned int i = 0; i < 2; i++)
		glDeleteShader(shaders[i]);

	GetData().modelUniform = glGetUniformLocation(GetData().program, "model");
	GetData().viewProjectionUniform = glGetUniformLocation(GetData().program, "viewProjection");

	GetData().test = glGetUniformLocation(GetData().program, "sampler");

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void Engine::Renderer::OpenGL::Terminate(void*& apiData)
{
	for (auto& vboItem : GetData().vboDatabase)
		vboItem.second.DeallocateDeviceBuffers();

	for (auto& iboItem : GetData().vboDatabase)
		iboItem.second.DeallocateDeviceBuffers();

	glDeleteProgram(GetData().program);

	delete static_cast<Data*>(apiData);
	apiData = nullptr;
}

void Engine::Renderer::OpenGL::PrepareRendering()
{
	// Loops through every viewport to see if we need to buffer anything new
	bool updateNeeded = false;
	for (size_t i = 0; i < GetViewportCount(); i++)
	{
		if (GetViewport(i).IsRenderInfoValidated() == false)
		{
			updateNeeded = true;
			break;
		}
	}

	if (updateNeeded == true)
	{
		UpdateVBODatabase();
		UpdateIBODatabase();
	}
}

void Engine::Renderer::OpenGL::Draw()
{
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto& viewport = Renderer::GetViewport(0);

	// Sprite render pass
	const auto& sprites = viewport.GetRenderInfo().sprites;
	if (sprites.size() > 0)
	{
		glDisable(GL_DEPTH_TEST);
		glUseProgram(GetData().program);

		auto viewMat = viewport.GetCameraModel();
		glProgramUniformMatrix4fv(GetData().program, GetData().viewProjectionUniform, 1, GL_FALSE, viewMat.data.data());

		const auto& spriteVBO = GetData().vboDatabase[static_cast<AssetID>(Asset::Mesh::SpritePlane)];
		glBindVertexArray(spriteVBO.vertexArrayObject);

		const auto& spriteTransforms = viewport.GetRenderInfo().spriteTransforms;
		for (size_t i = 0; i < sprites.size(); i++)
		{
			auto model = spriteTransforms[i];
			glProgramUniformMatrix4fv(GetData().program, GetData().modelUniform, 1, GL_FALSE, model.data.data());

			const auto& ibo = GetData().iboDatabase[sprites[i]];
			glBindTexture(GL_TEXTURE_2D, ibo.texture);

			glDrawElements(GL_TRIANGLES, spriteVBO.numIndices, spriteVBO.indexType, 0);
		}
	}

	Engine::Application::Core::GL_SwapWindow(viewport.GetSurfaceHandle());
}

void Engine::Renderer::OpenGL::UpdateVBODatabase()
{
	const auto& loadQueue = Renderer::Core::GetMeshLoadQueue();

	for (const auto& id : loadQueue)
	{
		using namespace Asset;
		auto meshDocument = LoadMeshDocument(static_cast<Mesh>(id));

		VBO vbo;

		vbo.indexType = meshDocument.GetIndexType() == Asset::MeshDocument::IndexType::Uint16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		vbo.numIndices = meshDocument.GetIndexCount();

		glGenVertexArrays(1, &vbo.vertexArrayObject);
		glBindVertexArray(vbo.vertexArrayObject);

		glGenBuffers(static_cast<GLint>(vbo.attributeBuffers.size()), vbo.attributeBuffers.data());

		glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[static_cast<size_t>(VBO::Attribute::Position)]);
		glBufferData(GL_ARRAY_BUFFER, meshDocument.GetData(Asset::MeshDocument::Attribute::Position).byteLength, meshDocument.GetDataPtr(Asset::MeshDocument::Attribute::Position), GL_STATIC_DRAW);
		glEnableVertexAttribArray(static_cast<size_t>(VBO::Attribute::Position));
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[static_cast<size_t>(VBO::Attribute::TexCoord)]);
		glBufferData(GL_ARRAY_BUFFER, meshDocument.GetData(Asset::MeshDocument::Attribute::TexCoord).byteLength, meshDocument.GetDataPtr(Asset::MeshDocument::Attribute::TexCoord), GL_STATIC_DRAW);
		glEnableVertexAttribArray(static_cast<size_t>(VBO::Attribute::TexCoord));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo.attributeBuffers[static_cast<size_t>(VBO::Attribute::Normal)]);
		glBufferData(GL_ARRAY_BUFFER, meshDocument.GetData(Asset::MeshDocument::Attribute::Normal).byteLength, meshDocument.GetDataPtr(Asset::MeshDocument::Attribute::Normal), GL_STATIC_DRAW);
		glEnableVertexAttribArray(static_cast<size_t>(VBO::Attribute::Normal));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.attributeBuffers[static_cast<size_t>(VBO::Attribute::Index)]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshDocument.GetData(Asset::MeshDocument::Attribute::Index).byteLength, meshDocument.GetDataPtr(Asset::MeshDocument::Attribute::Index), GL_STATIC_DRAW);

		GetData().vboDatabase.insert({ id, vbo });
	}

	const auto& unloadQueue = Renderer::Core::GetMeshUnloadQueue();
	for (const auto& item : unloadQueue)
	{
		const auto& iterator = GetData().vboDatabase.find(item);
		iterator->second.DeallocateDeviceBuffers();
		GetData().vboDatabase.erase(iterator);
	}
}

void Engine::Renderer::OpenGL::UpdateIBODatabase()
{
	const auto& loadQueue = Renderer::Core::GetSpriteLoadQueue();

	for (const auto& id : loadQueue)
	{
		auto texDocument = Asset::LoadTextureDocument(static_cast<Asset::Sprite>(id));

		IBO ibo;

		glGenTextures(1, &ibo.texture);
		glBindTexture(GL_TEXTURE_2D, ibo.texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texDocument.GetDimensions().width, texDocument.GetDimensions().height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texDocument.GetData());

		GetData().iboDatabase.insert({ id, ibo });
	}

	const auto& unloadQueue = Renderer::Core::GetSpriteUnloadQueue();
	for (const auto& item : unloadQueue)
	{
		const auto& iterator = GetData().iboDatabase.find(item);
		iterator->second.DeallocateDeviceBuffers();
		GetData().iboDatabase.erase(iterator);
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