// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

struct FrameData {
    VkCommandPool _commandPool;
    VkCommandBuffer _commandBuffer;
	VkFence _render_fence;
	VkSemaphore _render_semaphore;
	VkSemaphore _swapchain_semaphore;
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
	
	// Per-swapchain-image semaphores to fix validation errors
	//std::vector<VkSemaphore> _swapchainImageSemaphores;

	void init_vulkan();
	void init_swapchain(uint32_t width, uint32_t height);
	void init_commands();
	void init_sync_structures();

};
