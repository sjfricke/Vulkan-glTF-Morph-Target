#include "PointCloud.h"

PointCloud::PointCloud() {

}

PointCloud::~PointCloud() {

}

void PointCloud::CreateVulkanDevice(ANativeWindow* platformWindow, VkApplicationInfo* appInfo) {
    std::vector<const char *> instance_extensions;
    std::vector<const char *> device_extensions;

    instance_extensions.push_back("VK_KHR_surface");
    instance_extensions.push_back("VK_KHR_android_surface");

    device_extensions.push_back("VK_KHR_swapchain");

    // **********************************************************
    // Create the Vulkan instance
    VkInstanceCreateInfo instanceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
            .ppEnabledExtensionNames = instance_extensions.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
    };
    CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_device.instance_));
    VkAndroidSurfaceCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .window = platformWindow};

    CALL_VK(vkCreateAndroidSurfaceKHR(m_device.instance_, &createInfo, nullptr,
                                      &m_device.surface_));
    // Find one GPU to use:
    // On Android, every GPU device is equal -- supporting graphics/compute/present
    // for this sample, we use the very first GPU device found on the system
    uint32_t  gpuCount = 0;
    CALL_VK(vkEnumeratePhysicalDevices(m_device.instance_, &gpuCount, nullptr));
    VkPhysicalDevice tmpGpus[gpuCount];
    CALL_VK(vkEnumeratePhysicalDevices(m_device.instance_, &gpuCount, tmpGpus));
    m_device.gpuDevice_ = tmpGpus[0];     // Pick up the first GPU Device

    // Create a logical device (vulkan device)
    float priorities[] = { 1.0f, };
    VkDeviceQueueCreateInfo queueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCount = 1,
            .queueFamilyIndex = 0,
            .pQueuePriorities = priorities,
    };

    VkDeviceCreateInfo deviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
            .ppEnabledExtensionNames = device_extensions.data(),
            .pEnabledFeatures = nullptr,
    };

    CALL_VK(vkCreateDevice(m_device.gpuDevice_, &deviceCreateInfo, nullptr,
                           &m_device.device_));
    vkGetDeviceQueue(m_device.device_, 0, 0, &m_device.queue_);
}

void PointCloud::CreateSwapChain(void) {
    LOGI("->createSwapChain");
    memset(&m_swapchain, 0, sizeof(m_swapchain));

    // **********************************************************
    // Get the surface capabilities because:
    //   - It contains the minimal and max length of the chain, we will need it
    //   - It's necessary to query the supported surface format (R8G8B8A8 for
    //   instance ...)
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.gpuDevice_, m_device.surface_,
                                              &surfaceCapabilities);
    // Query the list of supported surface format and choose one we like
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.gpuDevice_, m_device.surface_,
                                         &formatCount, nullptr);
    VkSurfaceFormatKHR *formats = new VkSurfaceFormatKHR [formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.gpuDevice_, m_device.surface_,
                                         &formatCount, formats);
    LOGI("Got %d formats", formatCount);

    uint32_t chosenFormat;
    for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++) {
        if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM)
            break;
    }
    assert(chosenFormat < formatCount);

    m_swapchain.displaySize_ = surfaceCapabilities.currentExtent;
    m_swapchain.displayFormat_ = formats[chosenFormat].format;

    // **********************************************************
    // Create a swap chain (here we choose the minimum available number of surface
    // in the chain)
    uint32_t queueFamily = 0;
    VkSwapchainCreateInfoKHR swapchainCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .surface = m_device.surface_,
            .minImageCount = surfaceCapabilities.minImageCount,
            .imageFormat = formats[chosenFormat].format,
            .imageColorSpace = formats[chosenFormat].colorSpace,
            .imageExtent = surfaceCapabilities.currentExtent,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .imageArrayLayers = 1,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &queueFamily,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .oldSwapchain = VK_NULL_HANDLE,
            .clipped = VK_FALSE,
    };
    CALL_VK(vkCreateSwapchainKHR(m_device.device_, &swapchainCreateInfo,
                                 nullptr, &m_swapchain.swapchain_));

    // Get the length of the created swap chain
    CALL_VK(vkGetSwapchainImagesKHR(m_device.device_, m_swapchain.swapchain_,
                                    &m_swapchain.swapchainLength_, nullptr));
    delete [] formats;
    LOGI("<-createSwapChain");
}

void PointCloud::DeleteSwapChain(void) {
    for (int i = 0; i < m_swapchain.swapchainLength_; i++) {
        vkDestroyFramebuffer(m_device.device_, m_swapchain.framebuffers_[i], nullptr);
        vkDestroyImageView(m_device.device_, m_swapchain.displayViews_[i], nullptr);
    }
    delete[] m_swapchain.framebuffers_;
    delete[] m_swapchain.displayViews_;

    vkDestroySwapchainKHR(m_device.device_, m_swapchain.swapchain_, nullptr);
}

void PointCloud::CreateFrameBuffers(VkRenderPass& renderPass, VkImageView depthView) {

    // query display attachment to swapchain
    uint32_t SwapchainImagesCount = 0;
    CALL_VK(vkGetSwapchainImagesKHR(m_device.device_, m_swapchain.swapchain_,
                                    &SwapchainImagesCount, nullptr));
    VkImage* displayImages = new VkImage[SwapchainImagesCount];
    CALL_VK(vkGetSwapchainImagesKHR(m_device.device_, m_swapchain.swapchain_,
                                    &SwapchainImagesCount, displayImages));

    // create image view for each swapchain image
    m_swapchain.displayViews_ = new VkImageView[SwapchainImagesCount];
    for (uint32_t i = 0; i < SwapchainImagesCount; i++) {
        VkImageViewCreateInfo viewCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .image = displayImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_swapchain.displayFormat_,
                .components = {
                        .r = VK_COMPONENT_SWIZZLE_R,
                        .g = VK_COMPONENT_SWIZZLE_G,
                        .b = VK_COMPONENT_SWIZZLE_B,
                        .a = VK_COMPONENT_SWIZZLE_A,
                },
                .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                },
                .flags = 0,
        };
        CALL_VK(vkCreateImageView(m_device.device_, &viewCreateInfo, nullptr,
                                  &m_swapchain.displayViews_[i]));
    }
    delete[] displayImages;

    // create a framebuffer from each swapchain image
    m_swapchain.framebuffers_ = new VkFramebuffer[m_swapchain.swapchainLength_];
    for (uint32_t i = 0; i < m_swapchain.swapchainLength_; i++) {
        VkImageView attachments[2] = {
                m_swapchain.displayViews_[i], depthView,
        };
        VkFramebufferCreateInfo fbCreateInfo {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext = nullptr,
                .renderPass = renderPass,
                .layers = 1,
                .attachmentCount = 1,  // 2 if using depth
                .pAttachments = attachments,
                .width = static_cast<uint32_t>(m_swapchain.displaySize_.width),
                .height = static_cast<uint32_t>(m_swapchain.displaySize_.height),
        };
        fbCreateInfo.attachmentCount = (depthView == VK_NULL_HANDLE ? 1 : 2);

        CALL_VK(vkCreateFramebuffer(m_device.device_, &fbCreateInfo, nullptr,
                                    &m_swapchain.framebuffers_[i]));
    }
}

bool PointCloud::MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device.gpuDevice_, &memoryProperties);
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < 32; i++) {
        if ((typeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if ((memoryProperties.memoryTypes[i].propertyFlags &
                 requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

// Create our vertex buffer
bool PointCloud::CreateBuffers(void) {
    // -----------------------------------------------
    // Create the triangle vertex buffer

    // Vertex positions
    const float vertexData[] = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            0.0f,  1.0f, 0.0f,
    };

    // Create a vertex buffer
    uint32_t queueIdx = 0;
    VkBufferCreateInfo createBufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = sizeof(vertexData),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .flags = 0,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .pQueueFamilyIndices = &queueIdx,
            .queueFamilyIndexCount = 1,
    };

    CALL_VK(vkCreateBuffer(m_device.device_, &createBufferInfo, nullptr,
                           &m_buffers.vertexBuf));

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_device.device_, m_buffers.vertexBuf, &memReq);

    VkMemoryAllocateInfo allocInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = sizeof(vertexData),
            .memoryTypeIndex = 0,  // Memory type assigned in the next step
    };

    // Assign the proper memory type for that buffer
    allocInfo.allocationSize = memReq.size;
    MapMemoryTypeToIndex(memReq.memoryTypeBits,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                         &allocInfo.memoryTypeIndex);

    // Allocate memory for the buffer
    VkDeviceMemory deviceMemory;
    CALL_VK(vkAllocateMemory(m_device.device_, &allocInfo, nullptr, &deviceMemory));

    void* data;
    CALL_VK(vkMapMemory(m_device.device_, deviceMemory, 0, sizeof(vertexData), 0,
                        &data));
    memcpy(data, vertexData, sizeof(vertexData));
    vkUnmapMemory(m_device.device_, deviceMemory);

    CALL_VK(vkBindBufferMemory(m_device.device_, m_buffers.vertexBuf, deviceMemory, 0));
    return true;
}

void PointCloud::DeleteBuffers(void) {
    vkDestroyBuffer(m_device.device_, m_buffers.vertexBuf, nullptr);
}

VkResult PointCloud::loadShaderFromFile(const char* filePath, VkShaderModule* shaderOut, ShaderType type) {
    // Read the file
    assert(m_appCtx);
    AAsset* file = AAssetManager_open(m_appCtx->activity->assetManager,
                                      filePath, AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);

    char* fileContent = new char[fileLength];

    AAsset_read(file, fileContent, fileLength);

    VkShaderModuleCreateInfo shaderModuleCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .codeSize = fileLength,
            .pCode = (const uint32_t*)fileContent,
            .flags = 0,
    };
    VkResult result = vkCreateShaderModule(
            m_device.device_, &shaderModuleCreateInfo, nullptr, shaderOut);
    assert(result == VK_SUCCESS);

    delete[] fileContent;

    return result;
}

VkResult PointCloud::CreateGraphicsPipeline(void) {
    memset(&m_gfxPipeline, 0, sizeof(m_gfxPipeline));
    // Create pipeline layout (empty)
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
    };
    CALL_VK(vkCreatePipelineLayout(m_device.device_, &pipelineLayoutCreateInfo,
                                   nullptr, &m_gfxPipeline.layout));

    // No dynamic state in that tutorial
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .dynamicStateCount = 0,
            .pDynamicStates = nullptr};

    VkShaderModule vertexShader,fragmentShader;
    loadShaderFromFile("shaders/tri.vert.spv", &vertexShader, VERTEX_SHADER);
    loadShaderFromFile("shaders/tri.frag.spv", &fragmentShader, FRAGMENT_SHADER);

    // Specify vertex and fragment shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2] {
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertexShader,
                    .pSpecializationInfo = nullptr,
                    .flags = 0,
                    .pName = "main",
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext = nullptr,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragmentShader,
                    .pSpecializationInfo = nullptr,
                    .flags = 0,
                    .pName = "main",
            }
    };

    VkViewport viewports {
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
            .x = 0,
            .y = 0,
            .width = (float)m_swapchain.displaySize_.width,
            .height = (float)m_swapchain.displaySize_.height,
    };

    VkRect2D scissor = {
            .extent = m_swapchain.displaySize_,
            .offset = {.x = 0, .y = 0,}
    };
    // Specify viewport info
    VkPipelineViewportStateCreateInfo viewportInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .viewportCount = 1,
            .pViewports = &viewports,
            .scissorCount = 1,
            .pScissors = &scissor,
    };

    // Specify multisample info
    VkSampleMask sampleMask = ~0u;
    VkPipelineMultisampleStateCreateInfo multisampleInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 0,
            .pSampleMask = &sampleMask,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };

    // Specify color blend state
    VkPipelineColorBlendAttachmentState attachmentStates{
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
    };
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &attachmentStates,
            .flags = 0,
    };

    // Specify rasterizer info
    VkPipelineRasterizationStateCreateInfo rasterInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1,
    };

    // Specify input assembler state
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    // Specify vertex input state
    VkVertexInputBindingDescription vertex_input_bindings{
            .binding = 0,
            .stride = 3 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription vertex_input_attributes[1] {
            {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = 0,
            }
    };
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_input_bindings,
            .vertexAttributeDescriptionCount = 1,
            .pVertexAttributeDescriptions = vertex_input_attributes,
    };

    // Create the pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .pNext = nullptr,
            .initialDataSize = 0,
            .pInitialData = nullptr,
            .flags = 0,  // reserved, must be 0
    };

    CALL_VK(vkCreatePipelineCache(m_device.device_, &pipelineCacheInfo, nullptr,
                                  &m_gfxPipeline.cache));

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipelineCreateInfo {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportInfo,
            .pRasterizationState = &rasterInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = &dynamicStateInfo,
            .layout = m_gfxPipeline.layout,
            .renderPass = m_render.renderPass_,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };

    VkResult pipelineResult =
            vkCreateGraphicsPipelines(m_device.device_, m_gfxPipeline.cache, 1,
                                      &pipelineCreateInfo, nullptr, &m_gfxPipeline.pipeline);

    // We don't need the shaders anymore, we can release their memory
    vkDestroyShaderModule(m_device.device_, vertexShader, nullptr);
    vkDestroyShaderModule(m_device.device_, fragmentShader, nullptr);

    return pipelineResult;
}

void PointCloud::DeleteGraphicsPipeline(void) {
    if (m_gfxPipeline.pipeline == VK_NULL_HANDLE) { return; }
    vkDestroyPipeline(m_device.device_, m_gfxPipeline.pipeline, nullptr);
    vkDestroyPipelineCache(m_device.device_,m_gfxPipeline.cache, nullptr);
    vkDestroyPipelineLayout(m_device.device_, m_gfxPipeline.layout, nullptr);
}

bool PointCloud::InitVulkanPointCloud(android_app* app) {
    m_appCtx = app;

    if (!InitVulkan()) {
        LOGW("Vulkan is unavailable, install vulkan and re-start");
        return false;
    }

    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .apiVersion = VK_MAKE_VERSION(1, 0, 0),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .pApplicationName = "tutorial05_triangle_window",
            .pEngineName = "tutorial",
    };

    // create a device
    CreateVulkanDevice(app->window, &appInfo);

    CreateSwapChain();

    // -----------------------------------------------------------------
    // Create render pass
    VkAttachmentDescription attachmentDescriptions{
            .format = m_swapchain.displayFormat_,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colourReference = {
            .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .flags = 0,
            .inputAttachmentCount = 0,
            .pInputAttachments = nullptr,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colourReference,
            .pResolveAttachments = nullptr,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr,
    };
    VkRenderPassCreateInfo renderPassCreateInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .attachmentCount = 1,
            .pAttachments = &attachmentDescriptions,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 0,
            .pDependencies = nullptr,
    };
    CALL_VK(vkCreateRenderPass(m_device.device_, &renderPassCreateInfo,
                               nullptr, &m_render.renderPass_));

    // -----------------------------------------------------------------
    // Create 2 frame buffers.
    CreateFrameBuffers(m_render.renderPass_);

    CreateBuffers();    // create vertex buffers

    // Create graphics pipeline
    CreateGraphicsPipeline();

    // -----------------------------------------------
    // Create a pool of command buffers to allocate command buffer from
    VkCommandPoolCreateInfo cmdPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = 0,
    };
    CALL_VK(vkCreateCommandPool(m_device.device_, &cmdPoolCreateInfo,
                                nullptr, &m_render.cmdPool_));

    // Record a command buffer that just clear the screen
    // 1 command buffer draw in 1 framebuffer
    // In our case we need 2 command as we have 2 framebuffer
    m_render.cmdBufferLen_ = m_swapchain.swapchainLength_;
    m_render.cmdBuffer_ = new VkCommandBuffer[m_swapchain.swapchainLength_];
    VkCommandBufferAllocateInfo cmdBufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = m_render.cmdPool_,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = m_render.cmdBufferLen_,
    };
    CALL_VK(vkAllocateCommandBuffers(m_device.device_,
                                     &cmdBufferCreateInfo,
                                     m_render.cmdBuffer_));

    for (int bufferIndex = 0; bufferIndex < m_swapchain.swapchainLength_;
         bufferIndex++) {
        // We start by creating and declare the "beginning" our command buffer
        VkCommandBufferBeginInfo cmdBufferBeginInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = 0,
                .pInheritanceInfo = nullptr,
        };
        CALL_VK(vkBeginCommandBuffer(m_render.cmdBuffer_[bufferIndex],
                                     &cmdBufferBeginInfo));

        // Now we start a renderpass. Any draw command has to be recorded in a
        // renderpass
        VkClearValue clearVals {
                .color.float32[0] = 0.0f,
                .color.float32[1] = 0.34f,
                .color.float32[2] = 0.90f,
                .color.float32[3] = 1.0f,
        };
        VkRenderPassBeginInfo renderPassBeginInfo {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = nullptr,
                .renderPass = m_render.renderPass_,
                .framebuffer = m_swapchain.framebuffers_[bufferIndex],
                .renderArea = {
                        .offset = { .x = 0, .y = 0,},
                        .extent = m_swapchain.displaySize_
                },
                .clearValueCount = 1,
                .pClearValues = &clearVals
        };
        vkCmdBeginRenderPass(m_render.cmdBuffer_[bufferIndex], &renderPassBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        // Bind what is necessary to the command buffer
        vkCmdBindPipeline(m_render.cmdBuffer_[bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_gfxPipeline.pipeline);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_render.cmdBuffer_[bufferIndex], 0, 1, &m_buffers.vertexBuf,
                               &offset);

        // Draw Triangle
        vkCmdDraw(m_render.cmdBuffer_[bufferIndex], 3, 1, 0, 0);

        vkCmdEndRenderPass(m_render.cmdBuffer_[bufferIndex]);
        CALL_VK(vkEndCommandBuffer(m_render.cmdBuffer_[bufferIndex]));
    }

    // We need to create a fence to be able, in the main loop, to wait for our
    // draw command(s) to finish before swapping the framebuffers
    VkFenceCreateInfo fenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    CALL_VK(vkCreateFence(m_device.device_, &fenceCreateInfo,
                          nullptr, &m_render.fence_));

    // We need to create a semaphore to be able to wait, in the main loop, for our
    // framebuffer to be available for us before drawing.
    VkSemaphoreCreateInfo semaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
    };
    CALL_VK(vkCreateSemaphore(m_device.device_, &semaphoreCreateInfo,
                              nullptr, &m_render.semaphore_));

    m_device.initialized_ = true;
    return true;
}

// IsVulkanReady():
//    native app poll to see if we are ready to draw...
bool PointCloud::IsVulkanReady(void) {
    return m_device.initialized_;
}

void PointCloud::DeleteVulkan(void) {
    vkFreeCommandBuffers(m_device.device_, m_render.cmdPool_,
                         m_render.cmdBufferLen_, m_render.cmdBuffer_);
    delete[] m_render.cmdBuffer_;

    vkDestroyCommandPool(m_device.device_, m_render.cmdPool_, nullptr);
    vkDestroyRenderPass(m_device.device_, m_render.renderPass_, nullptr);
    DeleteSwapChain();
    DeleteGraphicsPipeline();
    DeleteBuffers();

    vkDestroyDevice(m_device.device_, nullptr);
    vkDestroyInstance(m_device.instance_, nullptr);

    m_device.initialized_ = false;
}

// Draw one frame
bool PointCloud::VulkanDrawFrame(void) {
    uint32_t nextIndex;
    // Get the framebuffer index we should draw in
    CALL_VK(vkAcquireNextImageKHR(m_device.device_, m_swapchain.swapchain_,
                                  UINT64_MAX, m_render.semaphore_,
                                  VK_NULL_HANDLE, &nextIndex));
    CALL_VK(vkResetFences(m_device.device_, 1, &m_render.fence_));

    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_render.semaphore_,
            .pWaitDstStageMask = &waitStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_render.cmdBuffer_[nextIndex],
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
    };
    CALL_VK(vkQueueSubmit(m_device.queue_, 1, &submit_info, m_render.fence_));
    CALL_VK(vkWaitForFences(m_device.device_, 1, &m_render.fence_, VK_TRUE, 100000000));

    LOGI("Drawing frames......");

    VkResult result;
    VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .swapchainCount = 1,
            .pSwapchains = &m_swapchain.swapchain_,
            .pImageIndices = &nextIndex,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pResults = &result,
    };
    vkQueuePresentKHR(m_device.queue_, &presentInfo);
    return true;
}