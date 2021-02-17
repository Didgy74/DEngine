#pragma once

#pragma warning( push )
#pragma warning( disable : 28182)
#pragma warning( disable : 26495)

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#undef max
#undef min
#undef CreateWindow

#pragma warning( pop )