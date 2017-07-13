VULKAN_SDK = /home/fricke/VulkanSDK/1.0.51.0/x86_64

CFLAGS = -std=c++11 -I$(VULKAN_SDK)/include
LDFLAGS = -L$(VULKAN_SDK)/lib `pkg-config --static --libs glfw3` -lvulkan

VulkanTest: src/VPCViewer.cpp
	g++ $(CFLAGS) -o VulkanTest src/VPCViewer.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	LD_LIBRARY_PATH=$(VULKAN_SDK)/lib VK_LAYER_PATH=$(VULKAN_SDK)/etc/explicit_layer.d ./VulkanTest

clean:
	rm -f VulkanTest