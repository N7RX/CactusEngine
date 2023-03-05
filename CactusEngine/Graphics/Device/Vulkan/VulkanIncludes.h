#pragma once
#ifndef VK_INCLUDES_H_
#define VK_INCLUDES_H_

#include "Configuration.h"

#if defined(PLATFORM_WINDOWS_CE)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VK_NO_PROTOTYPES

#include <volk/volk.h>

#define VULKAN_VERSION_CE VK_API_VERSION_1_2

#endif