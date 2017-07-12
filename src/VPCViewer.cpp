#include "VPCViewer.h"

// Main public facing running call
void VPCViewer::run()
{
    initVulkan();
    mainLoop();
    cleanup();
}

// Runs all init calls
void VPCViewer::initVulkan()
{
	printf("initVulkan\n");
	VK_CHECK_RESULT(initLayersExtensions());
	VK_CHECK_RESULT(initInstance());
    VK_CHECK_RESULT(initDevice());
    VK_CHECK_RESULT(initWindow(500, 500));
    VK_CHECK_RESULT(initSwapchain());
}

// Loops through each frame
void VPCViewer::mainLoop()
{
	printf("mainLoop\n");

}

// Need to make sure to clean up my vulkan mess I made
void VPCViewer::cleanup()
{
	printf("cleanup\n");

}

// Get all layers and extensions in one shot
VkResult VPCViewer::initLayersExtensions()
{
	uint32_t instance_layer_count;
    uint32_t instance_extension_count;
    VkLayerProperties *vk_properties = NULL;
    VkExtensionProperties *instance_extensions;
    VkResult result;

    // scan device for layers
    do {
        result = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        if (result) { return result; }

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }

        vk_properties = (VkLayerProperties *)realloc(vk_properties, instance_layer_count * sizeof(VkLayerProperties));

        result = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_properties);
    } while (result == VK_INCOMPLETE);

    // Now gather the extension list for each instance layer.
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = vk_properties[i];
        printf("layer_props.properties.layerName: %s\n", layer_props.properties.layerName);
        do {
        	result = vkEnumerateInstanceExtensionProperties(layer_props.properties.layerName, &instance_extension_count, NULL);
	        if (result) { return result; }

	        if (instance_extension_count == 0) {
	            return VK_SUCCESS;
	        }

	        layer_props.extensions.resize(instance_extension_count);
	        instance_extensions = layer_props.extensions.data();
	        result = vkEnumerateInstanceExtensionProperties(layer_props.properties.layerName, &instance_extension_count, instance_extensions);
	    } while (result == VK_INCOMPLETE);


        instance_layer_properties.push_back(layer_props);
    }

    free(vk_properties);

    // get Instance extensions
   	instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	#ifdef __ANDROID__
	    instance_extension_names.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
	#elif defined(_WIN32)
	    instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_IOS_MVK)
	    instance_extension_names.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	    instance_extension_names.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
	#elif defined(OS_LINUX_XCB)
	    instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	#elif defined(OS_LINUX_XLIB)
	    instance_extension_names.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME
	#endif

    #if defined( OS_LINUX_XLIB )
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_XLIB_SURFACE_EXTENSION_NAME
	#define PFN_vkCreateSurfaceKHR					PFN_vkCreateXlibSurfaceKHR
	#define vkCreateSurfaceKHR						vkCreateXlibSurfaceKHR
	#elif defined( OS_LINUX_XCB )
	#define VK_USE_PLATFORM_XCB_KHR
	#define VK_KHR_PLATFORM_SURFACE_EXTENSION_NAME	VK_KHR_XCB_SURFACE_EXTENSION_NAME
	#define PFN_vkCreateSurfaceKHR					PFN_vkCreateXcbSurfaceKHR
	#define vkCreateSurfaceKHR						vkCreateXcbSurfaceKHR
	#endif

    // get Device extensions
    device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

// Creates an instance of Vulkan
VkResult VPCViewer::initInstance()
{
	VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = "Vulkan Pointcloud Viewer";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "Vulkan Pointcloud Viewer";
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = instance_layer_names.size();
    inst_info.ppEnabledLayerNames = instance_layer_names.size() ? instance_layer_names.data() : NULL;
    inst_info.enabledExtensionCount = instance_extension_names.size();
    inst_info.ppEnabledExtensionNames = instance_extension_names.data();

    VkResult result = vkCreateInstance(&inst_info, NULL, &m_instance);
    assert(result == VK_SUCCESS);

    return result;
}

// Finds a device to be our GPU slave for all this computation
VkResult VPCViewer::initDevice()
{
	uint32_t gpu_count = 0;
	std::vector<VkPhysicalDevice> gpu_list;
	VkResult result;

	// Find device information
    result = vkEnumeratePhysicalDevices(m_instance, &gpu_count, NULL);
    assert(gpu_count);
    gpu_list.resize(gpu_count);

    result = vkEnumeratePhysicalDevices(m_instance, &gpu_count, gpu_list.data());
    assert(!result);

    m_physical_device = gpu_list[0];

    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, NULL);
    assert(queue_family_count >= 1);

    queue_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_family_count, queue_properties.data());
    assert(queue_family_count >= 1);

    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memory_properties);
    vkGetPhysicalDeviceProperties(m_physical_device, &gpu_properties);

    // Create Device now
    VkDeviceQueueCreateInfo queue_info = {};

    float queue_priorities[1] = {0.0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;
    queue_info.queueFamilyIndex = graphics_queue_family_index;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = device_extension_names.size();
    device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? device_extension_names.data() : NULL;
    device_info.pEnabledFeatures = NULL;

    result = vkCreateDevice(m_physical_device, &device_info, NULL, &m_device);
    assert(result == VK_SUCCESS);

    return result;
}

VkResult VPCViewer::initWindow(uint32_t width, uint32_t height)
{
	// Get window Size first
	#if defined(__ANDROID__)
		//AndroidGetWindowSize(&window_width, &window_height);
	#else
    window_width  = width;
    window_height = height;
	#endif

    assert(window_width > 0);
    assert(window_height > 0);

    // Get Window connection for Linux
	#if defined(__LINUX__)
	// Do nothing on Android or Windows
    const xcb_setup_t *setup;
    xcb_screen_iterator_t it;
    int scr;

    connection = xcb_connect(NULL, &scr);
    if (connection == NULL || xcb_connection_has_error(connection)) {
        std::cout << "Unable to make an XCB connection\n";
        exit(-1);
    }

    setup = xcb_get_setup(connection);
    it = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) xcb_screen_next(&it);

    screen = it.data;

    // initialize the window finally    
    uint32_t value_mask;
    uint32_t value_list[32];
    
    window = xcb_generate_id(connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, window_width, window_height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);

    // Magic code that will send notification when window is destroyed
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
    atom_wm_delete_window = xcb_intern_atom_reply(connection, cookie2, 0);

    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, (*reply).atom, 4, 32, 1,
                        &(*atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(connection, window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = {100, 100};
    xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
    xcb_flush(connection);

    xcb_generic_event_t *e;
    while ((e = xcb_wait_for_event(connection))) {
        if ((e->response_type & ~0x80) == XCB_EXPOSE) break;
    }
	#endif

	return VK_SUCCESS;
}

VkResult VPCViewer::initSwapchain()
{
	VkResult result;

	// Construct the surface description
	#if defined(__ANDROID__)
    GET_INSTANCE_PROC_ADDR(m_instance, CreateAndroidSurfaceKHR);

    VkAndroidSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = AndroidGetApplicationWindow();
    result = info.fpCreateAndroidSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
	#elif defined(__LINUX__)
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.connection = connection;
    createInfo.window = window;
    result = vkCreateXcbSurfaceKHR(m_instance, &createInfo, NULL, &m_surface);
	#endif 
    std::cout << "TE1ST 115" << std::endl;

    assert(result == VK_SUCCESS);

    // Find the best options for swapchain
    VkSurfaceCapabilitiesKHR surface_capabilities;
    std::vector<VkSurfaceFormatKHR> surface_formats_list;
    std::vector<VkPresentModeKHR> present_modes_list;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &surface_capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &formatCount, nullptr);

    if (formatCount != 0) {
        surface_formats_list.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &formatCount, surface_formats_list.data());
    }

    // uint32_t presentModeCount;
    // vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &presentModeCount, nullptr);
    //     std::cout << "TEST 114" << std::endl;

    // if (presentModeCount != 0) {
    //     present_modes_list.resize(presentModeCount);
    //     vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &presentModeCount, present_modes_list.data());
    // }
    std::cout << "TEST 115" << std::endl;

    // Find a valid Surface Format
    if (surface_formats_list.size() == 1 && surface_formats_list[0].format == VK_FORMAT_UNDEFINED) {
        surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    std::cout << "TEST 21" << std::endl;

    for (const auto& temp_surface_format : surface_formats_list) {
        if (temp_surface_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            temp_surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = temp_surface_format;
        }
    }
    std::cout << "TEST 2" << std::endl;

    // Find a valid Present Mode
    present_mode = VK_PRESENT_MODE_FIFO_KHR;

    // for (const auto& temp_present_mode : present_modes_list) {
    //     if (temp_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
    //         bestMode = temp_present_mode;
    //         break; // we want Mailbox if possible
    //     } else if (temp_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //         bestMode = temp_present_mode;
    //     }
    // }
        std::cout << "TEST 22" << std::endl;

    // present_mode = bestMode;

    // Find a valid VkExtent2D capability
    if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = surface_capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {window_width, window_height};

        actualExtent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, actualExtent.height));

        extent = actualExtent;
    }

    std::cout << "TEST 3" << std::endl;

    uint32_t imageCount = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && imageCount > surface_capabilities.maxImageCount) {
        imageCount = surface_capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surface_format.format;
    createInfo.imageColorSpace = surface_format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    std::cout << "TEST 4" << std::endl;

    // Find device queues
	graphics_queue_family_index = UINT32_MAX;
    present_queue_family_index = UINT32_MAX;
    int i = 0;
    for (const auto& queue_family : queue_properties) {
        if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_family_index = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, i, m_surface, &present_support);

        if (queue_family.queueCount > 0 && present_support) {
			present_queue_family_index = i;
	    }

        if (graphics_queue_family_index != UINT32_MAX && present_queue_family_index != UINT32_MAX) {
            break;
        }
        i++;
    }

    std::cout << "graphics: " << graphics_queue_family_index << "\nPresent: " << present_queue_family_index << std::endl;
    if (graphics_queue_family_index == UINT32_MAX || present_queue_family_index == UINT32_MAX) {
        std::cout << "Could not find a queues for both graphics and present" << std::endl;
        exit(-1);
    }

   	uint32_t queue_family_indices[] = {(uint32_t) graphics_queue_family_index, (uint32_t) present_queue_family_index};
    if (graphics_queue_family_index != present_queue_family_index) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queue_family_indices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = surface_capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = present_mode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swap_chain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }


    // vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    // swapChainImages.resize(imageCount);
    // vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    // swapChainImageFormat = surfaceFormat.format;
    // swapChainExtent = extent;

  
	return VK_SUCCESS;
}

int main(int argc, char* argv[]) 
{
    VPCViewer app;

    try {
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}