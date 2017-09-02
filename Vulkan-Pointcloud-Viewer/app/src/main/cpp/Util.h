#ifndef VULKAN_POINTCLOUD_VIEWER_UTIL_H
#define VULKAN_POINTCLOUD_VIEWER_UTIL_H

#include "vulkan_wrapper.h"

#define LOG_TAG "PointCloud Util"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ASSERT(cond, fmt, ...)                                \
  if (!(cond)) {                                              \
    __android_log_assert(#cond, LOG_TAG, fmt, ##__VA_ARGS__); \
  }

// Collection of grouped values
struct VulkanDeviceInfo {
    bool                initialized_;

    VkInstance          instance_;
    VkPhysicalDevice    gpuDevice_;
    VkDevice            device_;

    VkSurfaceKHR        surface_;
    VkQueue             queue_;
};

struct VulkanSwapchainInfo {
    VkSwapchainKHR swapchain_;
    uint32_t       swapchainLength_;

    VkExtent2D     displaySize_;
    VkFormat       displayFormat_;

    // array of frame buffers and views
    VkFramebuffer* framebuffers_;
    VkImageView*   displayViews_;
};

struct VulkanBufferInfo {
    VkBuffer vertexBuf;
};

struct VulkanGfxPipelineInfo {
    VkPipelineLayout  layout;
    VkPipelineCache   cache;
    VkPipeline        pipeline;
};
struct VulkanRenderInfo {
    VkRenderPass     renderPass_;
    VkCommandPool    cmdPool_;
    VkCommandBuffer* cmdBuffer_;
    uint32_t         cmdBufferLen_;
    VkSemaphore      semaphore_;
    VkFence          fence_;
};

enum ShaderType { VERTEX_SHADER, FRAGMENT_SHADER };

#endif //VULKAN_POINTCLOUD_VIEWER_UTIL_H
