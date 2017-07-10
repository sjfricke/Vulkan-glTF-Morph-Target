#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#elif defined(__linux__)
#include <xcb/xcb.h>
#endif

#include <vulkan/vulkan.h>

#define VK_CHECK_RESULT(f)																\
{																						\
	VkResult res = (f);																	\
	if (res != VK_SUCCESS)																\
	{																					\
		std::cout << "Fatal in " << __FILE__ << " at line " << __LINE__ << std::endl; \
		assert(res == VK_SUCCESS);														\
	}																					\
}
