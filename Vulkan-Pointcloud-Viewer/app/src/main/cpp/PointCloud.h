// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef VULKAN_POINTCLOUD_VIEWER_POINTCLOUD_H
#define VULKAN_POINTCLOUD_VIEWER_POINTCLOUD_H

// Android
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
// Vulkan
#include "vulkan_wrapper.h"
// App
#include "Util.h"
// C Libs
#include <unistd.h>
// STD Libs
#include <string>
#include <cstdlib>
#include <vector>
#include <cassert>

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, "PointCloud",             \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

class PointCloud {
    public:
    PointCloud();
    ~PointCloud();
    PointCloud(const PointCloud& other) = delete;
    PointCloud& operator=(const PointCloud& other) = delete;

    void CreateVulkanDevice(ANativeWindow* platformWindow, VkApplicationInfo* appInfo);

    void CreateSwapChain(void);
    void DeleteSwapChain(void);

        void CreateFrameBuffers(VkRenderPass& renderPass, VkImageView depthView = VK_NULL_HANDLE);

    // Helper functions
    bool MapMemoryTypeToIndex(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);

    bool CreateBuffers(void);
    void DeleteBuffers(void);

    VkResult CreateGraphicsPipeline(void);
    void DeleteGraphicsPipeline(void);

    VkResult loadShaderFromFile(const char* filePath, VkShaderModule* shaderOut, ShaderType type);

    // Initialize vulkan device context
    // after return, vulkan is ready to draw
    bool InitVulkanPointCloud(android_app* app);

    // delete vulkan device context when application goes away
    void DeleteVulkan(void);

    // Check if vulkan is ready to draw
    bool IsVulkanReady(void);

    // Ask Vulkan to Render a frame
    bool VulkanDrawFrame(void);

private:

    VulkanDeviceInfo  m_device;

    VulkanSwapchainInfo  m_swapchain;

    VulkanRenderInfo m_render;

    VulkanGfxPipelineInfo m_gfxPipeline;

    VulkanBufferInfo m_buffers;

    // Android Native App pointer
    android_app* m_appCtx = nullptr;

    // holds native window to write buffer too
    ANativeWindow* m_nativeWindow;

};

#endif // VULKAN_POINTCLOUD_VIEWER_POINTCLOUD_H


