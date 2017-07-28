#include "VPCViewer.h"

#define VALIDAITON_LAYERS 1

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

    #if defined(__linux__)
        printf("a\n");
        initxcbConnection();
            printf("xcb\n");

    #endif

    VK_CHECK_RESULT(initInstance());

    printf("initInstance\n");

    // VK_CHECK_RESULT(initDevice());
    // VK_CHECK_RESULT(initWindow(500, 500));
    // VK_CHECK_RESULT(initSwapchain());
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

#if defined(__linux__)

static inline xcb_intern_atom_reply_t* intern_atom_helper(xcb_connection_t *conn, bool only_if_exists, const char *str)
{
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_if_exists, strlen(str), str);
    return xcb_intern_atom_reply(conn, cookie, NULL);
}

// Set up a window using XCB and request event types
xcb_window_t VPCViewer::setupWindow()
{
    uint32_t value_mask, value_list[32];

    window = xcb_generate_id(connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = screen->black_pixel;
    value_list[1] =
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_POINTER_MOTION |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE;

    // if (settings.fullscreen)
    // {
    //     width = destWidth = screen->width_in_pixels;
    //     height = destHeight = screen->height_in_pixels;
    // }

    xcb_create_window(connection,
        XCB_COPY_FROM_PARENT,
        window, screen->root,
        0, 0, width, height, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_reply_t* reply = intern_atom_helper(connection, true, "WM_PROTOCOLS");
    atom_wm_delete_window = intern_atom_helper(connection, false, "WM_DELETE_WINDOW");

    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
        window, (*reply).atom, 4, 32, 1,
        &(*atom_wm_delete_window).atom);

    //std::string windowTitle = getWindowTitle();
    std::string windowTitle = "Window Title Temp";
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
        window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
        title.size(), windowTitle.c_str());

    free(reply);

    // if (settings.fullscreen)
    // {
    //     xcb_intern_atom_reply_t *atom_wm_state = intern_atom_helper(connection, false, "_NET_WM_STATE");
    //     xcb_intern_atom_reply_t *atom_wm_fullscreen = intern_atom_helper(connection, false, "_NET_WM_STATE_FULLSCREEN");
    //     xcb_change_property(connection,
    //             XCB_PROP_MODE_REPLACE,
    //             window, atom_wm_state->atom,
    //             XCB_ATOM_ATOM, 32, 1,
    //             &(atom_wm_fullscreen->atom));
    //     free(atom_wm_fullscreen);
    //     free(atom_wm_state);
    // }   

    xcb_map_window(connection, window);

    return(window);
}

// Initialize XCB connection
void VPCViewer::initxcbConnection()
{
    printf("0\n");
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

printf("011\n");
    connection = xcb_connect(NULL, &scr);
    printf("022\n");
    if (connection == NULL) {
        printf("Could not find a compatible Vulkan ICD!\n");
        fflush(stdout);
        exit(1);
    }
    printf("1\n");
    setup = xcb_get_setup(connection);
    printf("2\n");
    iter = xcb_setup_roots_iterator(setup);
    printf("3\n");
    while (scr-- > 0)
        xcb_screen_next(&iter);
    printf("4\n");
    screen = iter.data;
}

void VPCViewer::handleEvent(const xcb_generic_event_t *event)
{
    // switch (event->response_type & 0x7f)
    // {
    // case XCB_CLIENT_MESSAGE:
    //     if ((*(xcb_client_message_event_t*)event).data.data32[0] ==
    //         (*atom_wm_delete_window).atom) {
    //         quit = true;
    //     }
    //     break;
    // case XCB_MOTION_NOTIFY:
    // {
    //     xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
    //     if (mouseButtons.left)
    //     {
    //         rotation.x += (mousePos.y - (float)motion->event_y) * 1.25f;
    //         rotation.y -= (mousePos.x - (float)motion->event_x) * 1.25f;
    //         camera.rotate(glm::vec3((mousePos.y - (float)motion->event_y) * camera.rotationSpeed, -(mousePos.x - (float)motion->event_x) * camera.rotationSpeed, 0.0f));
    //         viewUpdated = true;
    //     }
    //     if (mouseButtons.right)
    //     {
    //         zoom += (mousePos.y - (float)motion->event_y) * .005f;
    //         camera.translate(glm::vec3(-0.0f, 0.0f, (mousePos.y - (float)motion->event_y) * .005f * zoomSpeed));
    //         viewUpdated = true;
    //     }
    //     if (mouseButtons.middle)
    //     {
    //         cameraPos.x -= (mousePos.x - (float)motion->event_x) * 0.01f;
    //         cameraPos.y -= (mousePos.y - (float)motion->event_y) * 0.01f;
    //         camera.translate(glm::vec3(-(mousePos.x - (float)(float)motion->event_x) * 0.01f, -(mousePos.y - (float)motion->event_y) * 0.01f, 0.0f));
    //         viewUpdated = true;
    //         mousePos.x = (float)motion->event_x;
    //         mousePos.y = (float)motion->event_y;
    //     }
    //     mousePos = glm::vec2((float)motion->event_x, (float)motion->event_y);
    // }
    // break;
    // case XCB_BUTTON_PRESS:
    // {
    //     xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
    //     if (press->detail == XCB_BUTTON_INDEX_1)
    //         mouseButtons.left = true;
    //     if (press->detail == XCB_BUTTON_INDEX_2)
    //         mouseButtons.middle = true;
    //     if (press->detail == XCB_BUTTON_INDEX_3)
    //         mouseButtons.right = true;
    // }
    // break;
    // case XCB_BUTTON_RELEASE:
    // {
    //     xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
    //     if (press->detail == XCB_BUTTON_INDEX_1)
    //         mouseButtons.left = false;
    //     if (press->detail == XCB_BUTTON_INDEX_2)
    //         mouseButtons.middle = false;
    //     if (press->detail == XCB_BUTTON_INDEX_3)
    //         mouseButtons.right = false;
    // }
    // break;
    // case XCB_KEY_PRESS:
    // {
    //     const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
    //     switch (keyEvent->detail)
    //     {
    //         case KEY_W:
    //             camera.keys.up = true;
    //             break;
    //         case KEY_S:
    //             camera.keys.down = true;
    //             break;
    //         case KEY_A:
    //             camera.keys.left = true;
    //             break;
    //         case KEY_D:
    //             camera.keys.right = true;
    //             break;
    //         case KEY_P:
    //             paused = !paused;
    //             break;
    //         case KEY_F1:
    //             if (enableTextOverlay)
    //             {
    //                 textOverlay->visible = !textOverlay->visible;
    //             }
    //             break;              
    //     }
    // }
    // break;  
    // case XCB_KEY_RELEASE:
    // {
    //     const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
    //     switch (keyEvent->detail)
    //     {
    //         case KEY_W:
    //             camera.keys.up = false;
    //             break;
    //         case KEY_S:
    //             camera.keys.down = false;
    //             break;
    //         case KEY_A:
    //             camera.keys.left = false;
    //             break;
    //         case KEY_D:
    //             camera.keys.right = false;
    //             break;          
    //         case KEY_ESCAPE:
    //             quit = true;
    //             break;
    //     }
    //     keyPressed(keyEvent->detail);
    // }
    // break;
    // case XCB_DESTROY_NOTIFY:
    //     quit = true;
    //     break;
    // case XCB_CONFIGURE_NOTIFY:
    // {
    //     const xcb_configure_notify_event_t *cfgEvent = (const xcb_configure_notify_event_t *)event;
    //     if ((prepared) && ((cfgEvent->width != width) || (cfgEvent->height != height)))
    //     {
    //             destWidth = cfgEvent->width;
    //             destHeight = cfgEvent->height;
    //             if ((destWidth > 0) && (destHeight > 0))
    //             {
    //                 windowResize();
    //             }
    //     }
    // }
    // break;
    // default:
    //     break;
    // }
}
#endif

// Creates an instance of Vulkan
VkResult VPCViewer::initInstance()
{
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = title.c_str();
    app_info.applicationVersion = 1;
    app_info.pEngineName = title.c_str();
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> instance_extensions = { VK_KHR_SURFACE_EXTENSION_NAME };

    // Enable surface extensions depending on os
    #if defined(_WIN32)
        instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    #elif defined(__ANDROID__)
        instance_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
    #elif defined(_DIRECT2DISPLAY)
        instance_extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
        printf("22");
    #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        instance_extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
        printf("33");
    #elif defined(__linux__)
        instance_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
        printf("EST");
    #endif


    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.pApplicationInfo = &app_info;
    if (instance_extensions.size() > 0)
    {
        if (VALIDAITON_LAYERS)
        {
            instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }
        instance_create_info.enabledExtensionCount = (uint32_t)instance_extensions.size();
        instance_create_info.ppEnabledExtensionNames = instance_extensions.data();
    }
    if (VALIDAITON_LAYERS)
    {
        instance_create_info.enabledLayerCount = debug::validationLayerCount;
        instance_create_info.ppEnabledLayerNames = debug::validationLayerNames;
    }

    for (int i = 0; i < instance_create_info.enabledLayerCount; i++) {
        std::cout << "Layer enabled: " << instance_create_info.ppEnabledLayerNames[i] << std::endl;
    }
    for (int i = 0; i < instance_create_info.enabledExtensionCount; i++) {
        std::cout << "Ext enabled: " << instance_create_info.ppEnabledExtensionNames[i] << std::endl;
    }

    VkResult result = vkCreateInstance(&instance_create_info, NULL, &m_instance);
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
    #if defined(__linux__)
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

// VkResult VPCViewer::initSwapchain()
// {
//     VkResult result;

//     // Construct the surface description
//     #if defined(__ANDROID__)
//     GET_INSTANCE_PROC_ADDR(m_instance, CreateAndroidSurfaceKHR);

//     VkAndroidSurfaceCreateInfoKHR createInfo;
//     createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
//     createInfo.pNext = nullptr;
//     createInfo.flags = 0;
//     createInfo.window = AndroidGetApplicationWindow();
//     result = info.fpCreateAndroidSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
//     #elif defined(__linux__)
//     VkXcbSurfaceCreateInfoKHR createInfo = {};
//     createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
//     createInfo.pNext = NULL;
//     createInfo.connection = connection;
//     createInfo.window = window;
//     result = vkCreateXcbSurfaceKHR(m_instance, &createInfo, NULL, &m_surface);
//     #endif 
//     std::cout << "TE1ST 115" << std::endl;

//     assert(result == VK_SUCCESS);

//     // Find the best options for swapchain
//     VkSurfaceCapabilitiesKHR surface_capabilities;
//     std::vector<VkSurfaceFormatKHR> surface_formats_list;
//     std::vector<VkPresentModeKHR> present_modes_list;

//     vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &surface_capabilities);

//     uint32_t formatCount;
//     vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &formatCount, nullptr);

//     if (formatCount != 0) {
//         surface_formats_list.resize(formatCount);
//         vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &formatCount, surface_formats_list.data());
//     }

//     // uint32_t presentModeCount;
//     // vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &presentModeCount, nullptr);
//     //     std::cout << "TEST 114" << std::endl;

//     // if (presentModeCount != 0) {
//     //     present_modes_list.resize(presentModeCount);
//     //     vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &presentModeCount, present_modes_list.data());
//     // }
//     std::cout << "TEST 115" << std::endl;

//     // Find a valid Surface Format
//     if (surface_formats_list.size() == 1 && surface_formats_list[0].format == VK_FORMAT_UNDEFINED) {
//         surface_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
//     }

//     std::cout << "TEST 21" << std::endl;

//     for (const auto& temp_surface_format : surface_formats_list) {
//         if (temp_surface_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
//             temp_surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//             surface_format = temp_surface_format;
//         }
//     }
//     std::cout << "TEST 2" << std::endl;

//     // Find a valid Present Mode
//     present_mode = VK_PRESENT_MODE_FIFO_KHR;

//     // for (const auto& temp_present_mode : present_modes_list) {
//     //     if (temp_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
//     //         bestMode = temp_present_mode;
//     //         break; // we want Mailbox if possible
//     //     } else if (temp_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
//     //         bestMode = temp_present_mode;
//     //     }
//     // }
//         std::cout << "TEST 22" << std::endl;

//     // present_mode = bestMode;

//     // Find a valid VkExtent2D capability
//     if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
//         extent = surface_capabilities.currentExtent;
//     } else {
//         VkExtent2D actualExtent = {window_width, window_height};

//         actualExtent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, actualExtent.width));
//         actualExtent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, actualExtent.height));

//         extent = actualExtent;
//     }

//     std::cout << "TEST 3" << std::endl;

//     uint32_t imageCount = surface_capabilities.minImageCount + 1;
//     if (surface_capabilities.maxImageCount > 0 && imageCount > surface_capabilities.maxImageCount) {
//         imageCount = surface_capabilities.maxImageCount;
//     }

//     VkSwapchainCreateInfoKHR createInfo = {};
//     createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//     createInfo.surface = m_surface;

//     createInfo.minImageCount = imageCount;
//     createInfo.imageFormat = surface_format.format;
//     createInfo.imageColorSpace = surface_format.colorSpace;
//     createInfo.imageExtent = extent;
//     createInfo.imageArrayLayers = 1;
//     createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//     std::cout << "TEST 4" << std::endl;

//     // Find device queues
//     graphics_queue_family_index = UINT32_MAX;
//     present_queue_family_index = UINT32_MAX;
//     int i = 0;
//     for (const auto& queue_family : queue_properties) {
//         if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
//             graphics_queue_family_index = i;
//         }

//         VkBool32 present_support = false;
//         vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, i, m_surface, &present_support);

//         if (queue_family.queueCount > 0 && present_support) {
//             present_queue_family_index = i;
//         }

//         if (graphics_queue_family_index != UINT32_MAX && present_queue_family_index != UINT32_MAX) {
//             break;
//         }
//         i++;
//     }

//     std::cout << "graphics: " << graphics_queue_family_index << "\nPresent: " << present_queue_family_index << std::endl;
//     if (graphics_queue_family_index == UINT32_MAX || present_queue_family_index == UINT32_MAX) {
//         std::cout << "Could not find a queues for both graphics and present" << std::endl;
//         exit(-1);
//     }

//     uint32_t queue_family_indices[] = {(uint32_t) graphics_queue_family_index, (uint32_t) present_queue_family_index};
//     if (graphics_queue_family_index != present_queue_family_index) {
//         createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
//         createInfo.queueFamilyIndexCount = 2;
//         createInfo.pQueueFamilyIndices = queue_family_indices;
//     } else {
//         createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//     }

//     createInfo.preTransform = surface_capabilities.currentTransform;
//     createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
//     createInfo.presentMode = present_mode;
//     createInfo.clipped = VK_TRUE;

//     createInfo.oldSwapchain = VK_NULL_HANDLE;

//     if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swap_chain) != VK_SUCCESS) {
//         throw std::runtime_error("failed to create swap chain!");
//     }


//     // vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
//     // swapChainImages.resize(imageCount);
//     // vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

//     // swapChainImageFormat = surfaceFormat.format;
//     // swapChainExtent = extent;

  
//     return VK_SUCCESS;
// }

#if defined(__linux__)
VPCViewer* app;

static void handleEvent(const xcb_generic_event_t *event)
{ 
    if (app != NULL) {
        app->handleEvent(event);
    }
}
#endif

int main(int argc, char* argv[]) 
{
    try {
        app->run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}