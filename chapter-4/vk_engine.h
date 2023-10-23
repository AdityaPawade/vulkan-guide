﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>
#include "vk_mem_alloc.h"
#include <deque>
#include <functional>
#include "vk_mesh.h"
#include <unordered_map>
#include <string>
#include <glm/mat4x4.hpp>
#include "vk_descriptors.h"
#include <chrono>
#include "fastgltf/types.hpp"
#include "vk_loader.h"

struct GLTFMesh;
namespace fastgltf { struct Mesh; }

class PipelineBuilder {
public:

	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo _depthStencil;
	VkPipelineRenderingCreateInfo _renderInfo;

	VkPipeline build_pipeline(VkDevice device);
};

enum class ImageTransitionMode {
	IntoAttachment,
	IntoGeneral,
	GeneralToPresent,
	AttachmentToPresent
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call functors
		}

		deletors.clear();
	}
};

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct RenderObject {
	Surface* mesh;
	Material* material;

	VkDescriptorSet bindSet;
	VkDescriptorSet matBind;
	glm::mat4 transform;
};

struct GPUSceneData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; //w for sun power
	glm::vec4 sunlightColor;
};

struct FrameData {
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;


	DescriptorAllocator _frameDescriptors;
	DeletionQueue _frameDeletionQueue;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	AllocatedBuffer cameraBuffer;
};

constexpr unsigned int FRAME_OVERLAP = 2;

struct GLTFScene {
	//stores the vertex and index buffers for the whole scene
	AllocatedBuffer meshBuffer;

	std::vector<AllocatedImage> allTextures;
};

struct DrawContext {
	std::vector<RenderObject> SurfacesToDraw;
	class VulkanEngine* engine;
};





class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};

	VkExtent2D _windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;

	//VkSemaphore _presentSemaphore, _renderSemaphore;
	//VkFence _renderFence;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	//VkCommandPool _commandPool;
	//VkCommandBuffer _mainCommandBuffer;
	
	FrameData _frames[FRAME_OVERLAP];

	VkRenderPass _renderPass;

	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swachainImageFormat;

	VkDescriptorPool _descriptorPool;

	DescriptorAllocator globalDescriptorAllocator;

	VkPipeline _gradientPipeline;
	VkPipelineLayout _gradientPipelineLayout;

	std::vector<VkFramebuffer> _framebuffers;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSet _defaultGLTFdescriptor;

	VkDescriptorSetLayout _swapchainImageDescriptorLayout;

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator; //vma lib allocator


	VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;
	VkDescriptorSetLayout _meshBufferDescriptorLayout;
	VkDescriptorSetLayout _gltfMatDescriptorLayout;

	VkPipelineLayout _trianglePipelineLayout;

	VkPipeline _trianglePipeline;

	Material _defaultMat;

	//draw resources
	AllocatedImage _drawImage;
	AllocatedImage _depthImage;

	//the format for the draw image
	VkFormat _drawFormat;

	//default image for fallback
	AllocatedImage _whiteImage;

	//immediate submit structures
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	void draw_geometry(VkCommandBuffer cmd);

	//run main loop
	void run();

	void uploadMesh(GLTFMesh* mesh);

	void uploadMesh(fastgltf::Mesh& mesh, fastgltf::Asset& asset);

	FrameData& get_current_frame();
	FrameData& get_last_frame();


	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage);

	AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage);

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
	std::unordered_map<std::string, Surface> loadedSurfaces;

	std::unordered_map<std::string,std::shared_ptr< LoadedGLTF>> loadedScenes;

	
private:

	//std::vector<RenderObject> renderables;


	void init_vulkan();

	void init_swapchain();

	void init_default_renderpass();

	void init_framebuffers();

	void init_commands();

	void init_pipelines();

	void init_descriptors();

	//void loadGltf(const std::filesystem::path& filePath);

	void init_sync_structures();

	void init_renderables();

	void transition_image(VkCommandBuffer cmd, VkImage image, ImageTransitionMode transitionMode);
	
	bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);
};
