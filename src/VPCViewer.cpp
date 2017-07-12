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
   // VK_CHECK_RESULT(initWindow());
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
		VK_USE_PLATFORM_XLIB_KHR
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

    VkResult result = vkCreateInstance(&inst_info, NULL, &mInstance);
    assert(result == VK_SUCCESS);

    return result;
}

// Finds a device to be our GPU slave for all this computation
VkResult VPCViewer::initDevice()
{
	uint32_t gpu_count = 0;
	VkResult result;

	// Find device information
    result = vkEnumeratePhysicalDevices(mInstance, &gpu_count, NULL);
    assert(gpu_count);
    gpu_list.resize(gpu_count);

    result = vkEnumeratePhysicalDevices(mInstance, &gpu_count, gpu_list.data());
    assert(!result);

    vkGetPhysicalDeviceQueueFamilyProperties(gpu_list[0], &queue_family_count, NULL);
    assert(queue_family_count >= 1);

    queue_properties.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu_list[0], &queue_family_count, queue_properties.data());
    assert(queue_family_count >= 1);

    vkGetPhysicalDeviceMemoryProperties(gpu_list[0], &memory_properties);
    vkGetPhysicalDeviceProperties(gpu_list[0], &gpu_properties);

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

    result = vkCreateDevice(gpu_list[0], &device_info, NULL, &mDevice);
    assert(result == VK_SUCCESS);

    return result;
}

VkResult VPCViewer::initWindow()
{
	return VK_SUCCESS;
}

VkResult VPCViewer::initSwapchain()
{
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