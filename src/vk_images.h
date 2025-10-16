
#pragma once 

namespace vkutil {


    void transition_image_layout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
};