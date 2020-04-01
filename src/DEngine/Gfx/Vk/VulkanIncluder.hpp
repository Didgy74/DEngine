#pragma once

#pragma warning( push )
#pragma warning( disable : 28182)
#pragma warning( disable : 26495)

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#undef max
#undef min

#pragma warning( pop )