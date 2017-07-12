#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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
	VkResult result = (f);																	\
	if (result != VK_SUCCESS)																\
	{																					\
		std::cout << "Fatal in " << __FILE__ << " at line " << __LINE__ << std::endl;   \
		assert(result == VK_SUCCESS);														\
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

    VkSurfaceKHR m_surface;
    bool prepared;
    bool use_staging_buffer;
    bool save_images;

    std::vector<const char *> instance_layer_names;
    std::vector<const char *> instance_extension_names;
    std::vector<layer_properties> instance_layer_properties;
    std::vector<VkExtensionProperties> instance_extension_properties;
    VkInstance m_instance;

    std::vector<const char *> device_extension_names;
    std::vector<VkExtensionProperties> device_extension_properties;
    VkPhysicalDevice m_physical_device;
    VkDevice m_device;
    
	VkSurfaceFormatKHR surface_format;
	VkPresentModeKHR present_mode;
	VkExtent2D extent;

    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkPhysicalDeviceProperties gpu_properties;
    std::vector<VkQueueFamilyProperties> queue_properties;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VkFramebuffer *framebuffers;
    uint32_t window_width;
    uint32_t window_height;
    VkFormat format;

    VkSwapchainKHR m_swap_chain;
    std::vector<swap_chain_buffer> buffers;
    VkSemaphore image_acquired_semaphore;

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
    VkDeviceMemory staging_memory;
    VkImage staging_image;

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
    VkPipelineCache pipeline_cache;
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

    #if defined(__LINUX__)
	    xcb_connection_t *connection;
	    xcb_screen_t *screen;
	    xcb_window_t window;
	    xcb_intern_atom_reply_t *atom_wm_delete_window;
	#endif

    VkResult initLayersExtensions();
    VkResult initInstance();
    VkResult initDevice();
    VkResult initWindow(uint32_t width, uint32_t height);
    VkResult initSwapchain();
};