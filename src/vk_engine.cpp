//> includes
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>
#include <vk_images.h>

// boostrap library
#include "VkBootstrap.h"

#include <chrono>
#include <thread>

VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }

void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _windowExtent.width,
        _windowExtent.height,
        window_flags);

    init_vulkan();

    init_swapchain(_windowExtent.width, _windowExtent.height);

    init_commands();

    init_sync_structures();

    // everything went fine
    _isInitialized = true;
}

void VulkanEngine::init_vulkan() {
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("Example Vulkan Application")
    .request_validation_layers(true)
    .require_api_version(1, 3, 0)
    .use_default_debug_messenger()
    .build();

    vkb::Instance vkb_inst = inst_ret.value();
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    VkPhysicalDeviceVulkan13Features vkb_features_13 {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    vkb_features_13.dynamicRendering = VK_TRUE;
    vkb_features_13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vkb_features_12 {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    vkb_features_12.bufferDeviceAddress = VK_TRUE;
    vkb_features_12.descriptorIndexing = VK_TRUE;

    vkb::PhysicalDeviceSelector selector = vkb::PhysicalDeviceSelector(vkb_inst);

    vkb::PhysicalDevice physical_device = selector.set_minimum_version(1, 3).set_surface(_surface).set_required_features_13(vkb_features_13).set_required_features_12(vkb_features_12).select().value();
    _chosenGpu = physical_device.physical_device;

    vkb::DeviceBuilder device_builder{physical_device};
    vkb::Device vkbDevice = device_builder.build().value();
    _device = vkbDevice.device;

    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

}

void VulkanEngine::init_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchain_builder{_chosenGpu, _device, _surface};
    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    vkb::Swapchain vkbSwapchain = swapchain_builder.set_desired_format(VkSurfaceFormatKHR{.format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
    .set_desired_extent(width, height)
    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    .build().value();

    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
    _swapchainExtent = vkbSwapchain.extent;
    
    // Create semaphores for each swapchain image
    //_swapchainImageSemaphores.resize(_swapchainImages.size());
    //VkSemaphoreCreateInfo semaphore_info = vkinit::semaphore_create_info();
    _frames.resize(_swapchainImages.size());
    //for (size_t i = 0; i < _swapchainImages.size(); i++) {
        //VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_swapchainImageSemaphores[i]));
    //}
}

void VulkanEngine::init_commands() {
    VkCommandPoolCreateInfo pool_info = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    for (int i = 0; i < _frames.size(); i++) {
        VK_CHECK(vkCreateCommandPool(_device, &pool_info, nullptr, &_frames[i]._commandPool));
        VkCommandBufferAllocateInfo alloc_info = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(_device, &alloc_info, &_frames[i]._commandBuffer));
    }
}

void VulkanEngine::init_sync_structures() {
    VkFenceCreateInfo fence_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphore_info = vkinit::semaphore_create_info();
    for (int i = 0; i < _frames.size(); i++) {
        VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_frames[i]._render_fence));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_frames[i]._swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_frames[i]._render_semaphore));
    }
}

void VulkanEngine::cleanup()
{
    if (_isInitialized) {

        vkDeviceWaitIdle(_device);
        for (int i = 0; i < _frames.size(); i++) {
            vkDestroyFence(_device, _frames[i]._render_fence, nullptr);
            vkDestroySemaphore(_device, _frames[i]._render_semaphore, nullptr);
            vkDestroySemaphore(_device, _frames[i]._swapchain_semaphore, nullptr);
        }

        for (int i = 0; i < _frames.size(); i++) {
            vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
        }

        for (int i = 0; i < _swapchainImageViews.size(); i++) {
            vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
        }
        
        //for (int i = 0; i < _swapchainImageSemaphores.size(); i++) {
            //vkDestroySemaphore(_device, _swapchainImageSemaphores[i], nullptr);
        //}

        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger, nullptr);
        vkDestroyInstance(_instance, nullptr);
        SDL_DestroyWindow(_window);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._render_fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._render_fence));
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchain_semaphore, VK_NULL_HANDLE, &swapchainImageIndex));

    VkCommandBuffer commandBuffer = get_current_frame()._commandBuffer;
    VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));
    VkCommandBufferBeginInfo begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &begin_info));
    vkutil::transition_image_layout(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    float flash = std::abs(sinf(_frameNumber/120.0f));
    VkClearColorValue clearColor = {0.0f, 0.0f, flash, 1.0f};
    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &clearRange);
    vkutil::transition_image_layout(commandBuffer, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkCommandBufferSubmitInfo commandBufferSubmitInfo = vkinit::command_buffer_submit_info(commandBuffer);
    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, get_current_frame()._swapchain_semaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._render_semaphore);


    VkSubmitInfo2 submitInfo = vkinit::submit_info(&commandBufferSubmitInfo, &signalInfo, &waitInfo);
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo, get_current_frame()._render_fence));

    VkPresentInfoKHR presentInfo = vkinit::present_info();
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &get_current_frame()._render_semaphore;
    
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &swapchainImageIndex;
    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
    _frameNumber++;
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    stop_rendering = false;
                }
            }
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}