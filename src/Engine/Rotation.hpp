#pragma once

#include "Math/Vector/Vector.hpp"

class Rotation
{
public:
	Math::Vector3D GetEulerAngles() const { return rotation; }

private:
	Math::Vector3D rotation;
};