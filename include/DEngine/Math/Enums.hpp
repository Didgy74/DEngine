#pragma once

namespace DEngine::Math
{
	enum class ElementaryAxis : unsigned char
	{
		X,
		Y,
		Z
	};

	enum class ElementaryPlane3D : unsigned char
	{
		XY,
		XZ,
		YZ
	};

	enum class API3D
	{
		OpenGL,
		Vulkan
	};
}