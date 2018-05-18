/*
* Vulkan Example - Morph Target in a glTF 2.0 model (blend shape)
*
* Copyright (C) 2018 by Spencer Fricke - sjfricke
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

// glTF format: https://github.com/KhronosGroup/glTF
// tinyglTF loader: https://github.com/syoyo/tinygltf

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <chrono>
#include <ratio>

#include <vulkan/vulkan.h>
#include "VulkanExampleBase.h"
#include "VulkanTexture.hpp"
#include "VulkanglTFModel.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"

/*
	Utility functions
*/
VkPipelineShaderStageCreateInfo loadShader(VkDevice device, std::string filename, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage{};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;
	shaderStage.pName = "main";
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	std::string assetpath = "shaders/" + filename;
	AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, assetpath.c_str(), AASSET_MODE_STREAMING);
	assert(asset);
	size_t size = AAsset_getLength(asset);
	assert(size > 0);
	char *shaderCode = new char[size];
	AAsset_read(asset, shaderCode, size);
	AAsset_close(asset);
	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo moduleCreateInfo;
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = NULL;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = (uint32_t*)shaderCode;
	moduleCreateInfo.flags = 0;
	VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderStage.module));
	delete[] shaderCode;
#else
	std::ifstream is("./../data/shaders/" + filename, std::ios::binary | std::ios::in | std::ios::ate);

	if (is.is_open()) {
		size_t size = is.tellg();
		is.seekg(0, std::ios::beg);
		char* shaderCode = new char[size];
		is.read(shaderCode, size);
		is.close();
		assert(size > 0);
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;
		vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderStage.module);
		delete[] shaderCode;
	}
	else {
		std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
		shaderStage.module = VK_NULL_HANDLE;
	}

#endif
	assert(shaderStage.module != VK_NULL_HANDLE);
	return shaderStage;
}

/*
	main class
*/
class VulkanExample : public VulkanExampleBase
{
public:
	int test = 0;
	struct Models {
		vkglTF::Model cube;
	} models;

	struct Buffer {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
		void *mapped;
	};

	struct UniformBuffers {
		Buffer morphTaret; // SSBO block
		Buffer cube;
	} uniformBuffers;

	struct UBOMatrices {
		glm::mat4 MVP;
		glm::mat4 model;
		glm::vec4 camera;
	} uboMatrices;

	struct PipelineLayouts {
		VkPipelineLayout morph;
		VkPipelineLayout normal;
	} pipelineLayouts;

	struct Pipelines {
		VkPipeline morph;
		VkPipeline normal;
	} pipelines;

	struct DescriptorSetLayouts {
		VkDescriptorSetLayout loader;
	} descriptorSetLayouts;

	struct DescriptorSets {
		VkDescriptorSet loader;
	} descriptorSets;

	glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);

	VulkanExample() : VulkanExampleBase()
	{
		title = "Vulkan glTf 2.0 Morph Target";
		camera.type = Camera::CameraType::firstperson;
		camera.movementSpeed = 2.0f;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.rotationSpeed = 0.25f;
		camera.setRotation({ 0.0f, 0.0f, 0.0f });
		camera.setPosition({ 0.0f, 0.0f, -3.5f });
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipelines.morph, nullptr);
		vkDestroyPipeline(device, pipelines.normal, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayouts.morph, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayouts.normal, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.loader, nullptr);

		models.cube.destroy(device);

		vkDestroyBuffer(device, uniformBuffers.cube.buffer, nullptr);
		vkFreeMemory(device, uniformBuffers.cube.memory, nullptr);
		vkDestroyBuffer(device, uniformBuffers.morphTaret.buffer, nullptr);
		vkFreeMemory(device, uniformBuffers.morphTaret.memory, nullptr);
	}

	void reBuildCommandBuffers()
	{
		if (!checkCommandBuffers())
		{
			destroyCommandBuffers();
			createCommandBuffers();
		}
		buildCommandBuffers();
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkClearValue clearValues[3];
		if (settings.multiSampling) {
			clearValues[0].color = { { 1.0f, 1.0f, 1.0f, 1.0f } };
			clearValues[1].color = { { 1.0f, 1.0f, 1.0f, 1.0f } };
			clearValues[2].depthStencil = { 1.0f, 0 };
		}
		else {
			clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
			clearValues[1].depthStencil = { 1.0f, 0 };
		}

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = settings.multiSampling ? 3 : 2;
		renderPassBeginInfo.pClearValues = clearValues;

		for (size_t i = 0; i < drawCmdBuffers.size(); ++i) {
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufferBeginInfo));
			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport{};
			viewport.width = (float)width;
			viewport.height = (float)height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.extent = { width, height };
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.morph, 0, 1, &descriptorSets.loader, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.morph);
			models.cube.draw(drawCmdBuffers[i], pipelineLayouts.morph);

			vkCmdEndRenderPass(drawCmdBuffers[i]);
			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
		const std::string assetpath = "";
#else
		const std::string assetpath = "./../data/";
		struct stat info;
		if (stat(assetpath.c_str(), &info) != 0) {
			std::string msg = "Could not locate asset path in \"" + assetpath + "\".\nMake sure binary is run from correct relative directory!";
			std::cerr << msg << std::endl;
#if defined(_WIN32)
			MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
#endif
			exit(-1);
		}
#endif
		models.cube.loadFromFile(assetpath + "models/AnimatedMorphCube/glTF/AnimatedMorphCube.gltf", vulkanDevice, queue);
//		models.cube.loadFromFile(assetpath + "models/AnimatedMorphSphere/glTF/AnimatedMorphSphere.gltf", vulkanDevice, queue);
//		models.cube.loadFromFile(assetpath + "models/twoCube/twoCubeLinear.gltf", vulkanDevice, queue);
//		models.cube.loadFromFile(assetpath + "models/twoCubeMorph/twoCubeMorph.gltf", vulkanDevice, queue);
//		models.cube.loadFromFile(assetpath + "models/threeCube/threeCube.gltf", vulkanDevice, queue);

		// Need to wait until we get morph target data to build storage buffer for it
		prepareStorageBuffers();
    }

	void setupDescriptors()
	{
		/*
			Descriptor Pool
		*/
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
		};
		VkDescriptorPoolCreateInfo descriptorPoolCI{};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolCI.pPoolSizes = poolSizes.data();
		descriptorPoolCI.maxSets = 1;
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

		/*
			Descriptor sets
		*/
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT , nullptr },
				{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT , nullptr },
			};

			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
			descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
			descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.loader));

			VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
			descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocInfo.descriptorPool = descriptorPool;
			descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayouts.loader;
			descriptorSetAllocInfo.descriptorSetCount = 1;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSets.loader));

			std::vector<VkWriteDescriptorSet> writeDescriptorSets(2);

			writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeDescriptorSets[0].descriptorCount = 1;
			writeDescriptorSets[0].dstSet = descriptorSets.loader;
			writeDescriptorSets[0].dstBinding = 0;
			writeDescriptorSets[0].pBufferInfo = &uniformBuffers.cube.descriptor;

			writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[1].descriptorCount = 1;
			writeDescriptorSets[1].dstSet = descriptorSets.loader;
			writeDescriptorSets[1].dstBinding = 1;
			writeDescriptorSets[1].pBufferInfo = &uniformBuffers.morphTaret.descriptor;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
		}
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationStateCI.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;
		
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_TRUE;
		depthStencilStateCI.depthWriteEnable = VK_TRUE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.front = depthStencilStateCI.back;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		if (settings.multiSampling) {
			multisampleStateCI.rasterizationSamples = settings.sampleCount;
		} else {
			multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		}

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		// Pipeline layout
		std::array<VkDescriptorSetLayout, 1> setLayouts = { descriptorSetLayouts.loader };

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.size = sizeof(vkglTF::Mesh::morphPushConst);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.pSetLayouts = setLayouts.data();
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pushConstantRangeCount = 1;
		pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayouts.morph));

		pipelineLayoutCI.pushConstantRangeCount = 0;
		pipelineLayoutCI.pPushConstantRanges = nullptr;

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayouts.normal));

		// Vertex bindings an attributes
		VkVertexInputBindingDescription vertexInputBinding = { 0, sizeof(vkglTF::Model::Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, pos) }, // inPos
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, normal) }, // inNormal
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Model::Vertex, tangent) }, // inTangent
		};

		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

		// Pipelines
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelineLayouts.morph;
		pipelineCI.renderPass = renderPass;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		// Morph Mesh pipeline
		rasterizationStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;
		shaderStages = {
			loadShader(device, "morph.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(device, "morph.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.morph));
		for (auto shaderStage : shaderStages) {
			vkDestroyShaderModule(device, shaderStage.module, nullptr);
		}

		// Normal Mesh pipeline
		pipelineCI.layout = pipelineLayouts.normal;
		shaderStages = {
			loadShader(device, "normal.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(device, "morph.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.normal));
		for (auto shaderStage : shaderStages) {
			vkDestroyShaderModule(device, shaderStage.module, nullptr);
		}
	}

	/*
		Prepare and initialize uniform buffer containing shader uniforms
	*/
	void prepareUniformBuffers()
	{
		// Cube vertex shader uniform buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboMatrices),
			&uniformBuffers.cube.buffer,
			&uniformBuffers.cube.memory));

		// Descriptors
		uniformBuffers.cube.descriptor = { uniformBuffers.cube.buffer, 0, sizeof(uboMatrices) };

		// Map persistent
		VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers.cube.memory, 0, sizeof(uboMatrices), 0, &uniformBuffers.cube.mapped));

		updateUniformBuffers();
	}

	/*
		Prepare Storage Buffers used for dynamic amount of buffers in shaders
	 */
	void prepareStorageBuffers()
	{
		VkBuffer stageBuffer;
		VkDeviceMemory stageMemory;
		uint32_t stagingSize = static_cast<uint32_t>(models.cube.morphVertexData.size() * sizeof(float));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			stagingSize,
			&stageBuffer,
			&stageMemory,
		    models.cube.morphVertexData.data() ));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			stagingSize,
			&uniformBuffers.morphTaret.buffer,
			&uniformBuffers.morphTaret.memory));

		// Copy to staging buffer
		VkCommandPool commandPool;
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex; // TODO get a better way
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.commandPool = commandPool;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer copyCmd;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));
		VkCommandBufferBeginInfo cmdBufInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

		VkBufferCopy copyRegion = {};
		copyRegion.size = stagingSize;
		vkCmdCopyBuffer(copyCmd, stageBuffer, uniformBuffers.morphTaret.buffer, 1, &copyRegion);
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &copyCmd;
		VkFenceCreateInfo fenceCreateInfo {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));

		// Submit to the queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);
		vkDestroyBuffer(device, stageBuffer, nullptr);
		vkFreeMemory(device, stageMemory, nullptr);
		vkDestroyCommandPool(device, commandPool, nullptr);

		uniformBuffers.morphTaret.descriptor = { uniformBuffers.morphTaret.buffer, 0, VK_WHOLE_SIZE };
	}

	void updateUniformBuffers()
	{
		// 3D object
		uboMatrices.model = glm::mat4(1.0f);
		uboMatrices.model = glm::rotate(uboMatrices.model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		uboMatrices.MVP = camera.matrices.perspective * camera.matrices.view * uboMatrices.model;
		uboMatrices.camera = glm::vec4(camera.position * -1.0f, 1.0f);
		memcpy(uniformBuffers.cube.mapped, &uboMatrices, sizeof(uboMatrices));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		buildCommandBuffers();

		prepared = true;

		// start timer for animation
		tAnimation = std::chrono::high_resolution_clock::now();
	}

	virtual void render()
	{
		if (!prepared) {
			return;
		}
		VulkanExampleBase::prepareFrame();
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentBuffer]));
		const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]));
		VulkanExampleBase::submitFrame();
		VK_CHECK_RESULT(vkQueueWaitIdle(queue));
		if (!paused) {
			// This is my implemenation of doing the animation loop
			// Very naive approuch, but gets the job done, would like to clean up in future TODO

//			test++; if (test % 500 == 0) { test = 0; std::cout << getWindowTitle() << std::endl; } // print out FPS

			// Update all the models animation timers
			auto tDiff = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tAnimation).count() / 1000.0f;
			tAnimation = std::chrono::high_resolution_clock::now();
			models.cube.currentTime += tDiff;

			// need shared reset since curretTime is per model
			bool reset = false;

			for (auto& mesh: models.cube.meshesMorph) {

				// check to reset loop
				if (models.cube.currentTime > models.cube.animationMaxTime) {
					mesh.currentIndex = 0;
					reset = true;

					// reset all weight data
					for (size_t i = 0; i < mesh.weightsInit.size(); i++) {
						mesh.morphPushConst.weights[i] = mesh.weightsInit[i];
					}

				} else {

					// check where currentIndex is at
					while (true) {
						if (mesh.currentIndex == mesh.weightsTime.size() - 1) {
							break; // at end
						}

						if (models.cube.currentTime > mesh.weightsTime[mesh.currentIndex + 1]) {
							mesh.currentIndex++;
						} else {
							break;
						}
					}

					// TODO all glTF sampler inputs are linear, don't need to compute for non-linear methods
					switch (mesh.interpolation) {
						// TODO clean up LINEAR math style to be readable
						case vkglTF::Mesh::LINEAR:
							if (mesh.currentIndex < mesh.weightsTime.size() - 1) {

								float mixRate = (models.cube.currentTime - mesh.weightsTime[mesh.currentIndex]) /
									(mesh.weightsTime[mesh.currentIndex + 1] - mesh.weightsTime[mesh.currentIndex]);

								for (size_t i = 0; i < mesh.weightsInit.size(); i++) {
									float weightDiff = mesh.weightsData[(mesh.currentIndex + 1) * mesh.weightsInit.size() + i] - mesh.weightsData[mesh.currentIndex * mesh.weightsInit.size() + i];
									mesh.morphPushConst.weights[i] = (mixRate * weightDiff) + mesh.weightsData[mesh.currentIndex * mesh.weightsInit.size() + i];
								}
//								std::cout << "weights[0]: " << mesh.morphPushConst.weights[0] << std::endl;
								break;
							}
							// if at end then drop to STEP and set last index weights
						case vkglTF::Mesh::STEP:
							// sets weight to currentIndex only when step is reached
							for (size_t i = 0; i < mesh.weightsInit.size(); i++) {
								mesh.morphPushConst.weights[i] =
									mesh.weightsData[mesh.currentIndex * mesh.weightsInit.size() + i];
							}
							break;
						case vkglTF::Mesh::CUBICSPLINE:
							// TODO correctly implement... putting linear until then
							if (mesh.currentIndex < mesh.weightsTime.size() - 1) {

								float mixRate = (models.cube.currentTime - mesh.weightsTime[mesh.currentIndex]) /
									(mesh.weightsTime[mesh.currentIndex + 1] - mesh.weightsTime[mesh.currentIndex]);

								for (size_t i = 0; i < mesh.weightsInit.size(); i++) {
									float weightDiff = mesh.weightsData[(mesh.currentIndex+1) * mesh.weightsInit.size() * 3 + mesh.weightsInit.size() + i] - mesh.weightsData[mesh.currentIndex * mesh.weightsInit.size() * 3 + mesh.weightsInit.size() + i];
									mesh.morphPushConst.weights[i] = (mixRate * weightDiff) + mesh.weightsData[mesh.currentIndex * mesh.weightsInit.size() * 3 + mesh.weightsInit.size() + i];
								}
								break;
							}
							break;
						default: std::cout << "Non supported interpolation" << std::endl;
					}
				}
			} // for(mesh)

			if (reset) {
				models.cube.currentTime = 0.0f;
			}
			reBuildCommandBuffers();
		} // if(!paused)
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}
};

VulkanExample *vulkanExample;

// OS specific macros for the example main entry points
#if defined(_WIN32)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	for (int32_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
// Android entry point
// A note on app_dummy(): This is required as the compiler may otherwise remove the main entry point of the application
void android_main(android_app* state)
{
	vulkanExample = new VulkanExample();
	state->userData = vulkanExample;
	state->onAppCmd = VulkanExample::handleAppCommand;
	state->onInputEvent = VulkanExample::handleAppInput;
	androidApp = state;
	vks::android::getDeviceConfig();
	vulkanExample->renderLoop();
	delete(vulkanExample);
}
#elif defined(_DIRECT2DISPLAY)
// Linux entry point with direct to display wsi
static void handleEvent()
{
}
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_XCB_KHR)
static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#endif