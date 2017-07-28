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

#include "debug.h"
#include "camera.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

    #include <xcb/xcb.h>
    #define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"

    // #define VK_USE_PLATFORM_XCB_KHR
    // #define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME  VK_KHR_XCB_SURFACE_EXTENSION_NAME
    // #define PFN_vkCreateSurfaceKHR                  PFN_vkCreateXcbSurfaceKHR
    // #define vkCreateSurfaceKHR                      vkCreateXcbSurfaceKHR

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

    #if defined(__linux__)
        struct {
            bool left = false;
            bool right = false;
            bool middle = false;
        } mouseButtons;
        bool quit = false;
        xcb_connection_t *connection;
        xcb_screen_t *screen;
        xcb_window_t window;
        xcb_intern_atom_reply_t *atom_wm_delete_window;

        xcb_window_t setupWindow();
        
        void handleEvent(const xcb_generic_event_t *event);
    #endif

private:

void initxcbConnection();
    void initVulkan();
    void mainLoop();
    void cleanup();

    uint32_t width = 1280;
    uint32_t height = 720;

    float zoom = 0;

    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float timerSpeed = 0.25f;
    
    bool paused = false;

    // Use to adjust mouse rotation speed
    float rotationSpeed = 1.0f;
    // Use to adjust mouse zoom speed
    float zoomSpeed = 1.0f;

    Camera camera;

    glm::vec3 rotation = glm::vec3();
    glm::vec3 cameraPos = glm::vec3();
    glm::vec2 mousePos;

    std::string title = "Vulkan Pointcloud Viewer";
    std::string name = "VulkanPointcloudViewer";

    struct 
    {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depthStencil;

    // Gamepad state (only one pad supported)
    struct
    {
        glm::vec2 axisLeft = glm::vec2(0.0f);
        glm::vec2 axisRight = glm::vec2(0.0f);
    } gamePadState;


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

    VkResult initInstance();
    VkResult initDevice();
    VkResult initWindow(uint32_t width, uint32_t height);
    VkResult initSwapchain();
};