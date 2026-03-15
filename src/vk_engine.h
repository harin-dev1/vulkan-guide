// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vk_descriptors.h>

struct DeletionQueue {
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()> &&function) {
		deletors.push_back(function);
	}

	void flush() {
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)();
		}
		deletors.clear();
	}
};

struct FrameData {
    VkCommandPool _commandPool;
    VkCommandBuffer _commandBuffer;
	VkFence _render_fence;
	VkSemaphore _render_semaphore;
	VkSemaphore _swapchain_semaphore;
	DeletionQueue _deletionQueue;
};

struct AllocatedImage {
	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
	VkExtent3D imageExtent;
	VkFormat imageFormat;
};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
	char const* name;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	ComputePushConstants data;
};

class VulkanEngine {
public:
	std::vector<FrameData> _frames;
	FrameData& get_current_frame() { return _frames[_frameNumber % _frames.size()]; }
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	
	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };
	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet _drawImageDescriptors;
	VkDescriptorSetLayout _drawImageDescriptorLayout;

	struct SDL_Window* _window{ nullptr };

	static VulkanEngine& Get();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private:
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGpu;
	VkDevice _device;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkExtent2D _swapchainExtent;
	DeletionQueue _mainDeletionQueue;
	VmaAllocator _allocator;
	AllocatedImage _drawImage;
	VkExtent2D _drawExtent;
	VkPipelineLayout _backgroundPipelineLayout;
	std::vector<ComputeEffect> backgroundEffects;
	int currentBackgroundEffect{0};
	uint32_t _lastSubmittedFrameIndex{0};

	void init_vulkan();
	void init_swapchain(uint32_t width, uint32_t height);
	void init_commands();
	void init_sync_structures();
	void init_descriptors();
	void draw_background(VkCommandBuffer cmd);
	void init_pipelines();
	void init_imgui();
	void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
};
