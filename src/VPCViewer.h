#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <functional>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined( OS_LINUX_XLIB )
	#include <X11/Xlib.h>
	#include <X11/Xatom.h>
	#include <X11/extensions/xf86vmode.h>	// for fullscreen video mode
	#include <X11/extensions/Xrandr.h>		// for resolution changes
	#define VK_USE_PLATFORM_XLIB_KHR
#elif defined( OS_LINUX_XCB )
	#include <X11/keysym.h>
	#include <xcb/xcb.h>
	#include <xcb/xcb_keysyms.h>
	#include <xcb/xcb_icccm.h>
	#include <xcb/randr.h>
	#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>

#define VK_CHECK_RESULT(f)																\
{																						\
	VkResult res = (f);																	\
	if (res != VK_SUCCESS)																\
	{																					\
		std::cout << "Fatal in " << __FILE__ << " at line " << __LINE__ << std::endl;   \
		assert(res == VK_SUCCESS);														\
	}																					\
}

typedef struct _swap_chain_buffers {
    VkImage image;
    VkImageView view;
} swap_chain_buffer;

// A layer can expose extensions, keep track of those extensions here.
typedef struct {
    VkLayerProperties properties;
    std::vector<VkExtensionProperties> extensions;
} layer_properties;


class VPCViewer {
public:

	VPCViewer() {}
	~VPCViewer() {}

    void run();

private:

    void initVulkan();
    void mainLoop();
    void cleanup();

    VkSurfaceKHR surface;
    bool prepared;
    bool use_staging_buffer;
    bool save_images;

    std::vector<const char *> instance_layer_names;
    std::vector<const char *> instance_extension_names;
    std::vector<layer_properties> instance_layer_properties;
    std::vector<VkExtensionProperties> instance_extension_properties;
    VkInstance mInstance;

    std::vector<const char *> device_extension_names;
    std::vector<VkExtensionProperties> device_extension_properties;
    std::vector<VkPhysicalDevice> gpu_list;
    VkDevice mDevice;
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkPhysicalDeviceProperties gpu_properties;
    std::vector<VkQueueFamilyProperties> queue_properties;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VkFramebuffer *framebuffers;
    int width, height;
    VkFormat format;

    uint32_t swapchainImageCount;
    VkSwapchainKHR swap_chain;
    std::vector<swap_chain_buffer> buffers;
    VkSemaphore imageAcquiredSemaphore;

    VkCommandPool cmd_pool;

    struct {
        VkFormat format;

        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    struct {
        VkBuffer buf;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
    } uniform_data;

    struct {
        VkDescriptorImageInfo image_info;
    } texture_data;
    VkDeviceMemory stagingMemory;
    VkImage stagingImage;

    struct {
        VkBuffer buf;
        VkDeviceMemory mem;
        VkDescriptorBufferInfo buffer_info;
    } vertex_buffer;
    VkVertexInputBindingDescription vi_binding;
    VkVertexInputAttributeDescription vi_attribs[2];

    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 Model;
    glm::mat4 Clip;
    glm::mat4 MVP;

    VkCommandBuffer cmd; // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    std::vector<VkDescriptorSetLayout> desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    VkPipelineShaderStageCreateInfo shaderStages[2];

    VkDescriptorPool desc_pool;
    std::vector<VkDescriptorSet> desc_set;

    PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;
    PFN_vkDebugReportMessageEXT dbgBreakCallback;
    std::vector<VkDebugReportCallbackEXT> debug_report_callbacks;

    uint32_t current_buffer;
    uint32_t queue_family_count;

    VkViewport viewport;
    VkRect2D scissor;

    VkResult initLayersExtensions();
    VkResult initInstance();
    VkResult initDevice();
    VkResult initWindow();
    VkResult initSwapchain();
};