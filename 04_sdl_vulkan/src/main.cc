#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "sdl_window.hh"
#include "vulkan.hh"

class App {
public:
    void run() {
        SDLWindow window;
        Vulkan vulkan(window.get_vulkan_extensions());
        const auto surface = window.create_vulkan_surface(vulkan.get_instance());
        const auto extent = window.get_drawable_size();
        vulkan.initialize(surface, extent.first, extent.second);
        window.main_loop();
    }
};

int main(int, char **) {
    try {
        App app;
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        throw;
    } catch (...) {
        throw;
    }

    return 0;
}


// class OldVulkan {
// public:
//     void run() {
//         createGraphicsPipeline();
//         createFramebuffers();
//         createCommandPool();
//         createCommandBuffers();
//         createSemaphores();
//         mainLoop();
//         cleanup();
//     }

// private:
//     SDL_Window* window;
//     VkInstance instance;
//     VkDebugReportCallbackEXT callback;
//     VkPhysicalDevice physical_device = VK_NULL_HANDLE;
//     VkDevice device;
//     VkQueue graphicsQueue;
//     VkSurfaceKHR surface;
//     VkQueue presentQueue;
//     VkSwapchainKHR swapChain;
//     std::vector<VkImage> swapChainImages;
//     VkFormat swapChainImageFormat;
//     VkExtent2D swapChainExtent;
//     std::vector<VkImageView> swapChainImageViews;
//     VkRenderPass renderPass;
//     VkPipelineLayout pipelineLayout;
//     VkPipeline graphicsPipeline;
//     std::vector<VkFramebuffer> swapChainFramebuffers;
//     VkCommandPool commandPool;
//     std::vector<VkCommandBuffer> commandBuffers;
//     VkSemaphore imageAvailableSemaphore;
//     VkSemaphore renderFinishedSemaphore;
//     const std::vector<const char*> deviceExtensions = {
//         VK_KHR_SWAPCHAIN_EXTENSION_NAME
//     };

//     void createSemaphores() {
//         VkSemaphoreCreateInfo semaphoreInfo = {};
//         semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
//         if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
//             vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create semaphores!");
//         }
//     }

//     void createCommandBuffers() {
//         commandBuffers.resize(swapChainFramebuffers.size());
//         VkCommandBufferAllocateInfo allocInfo = {};
//         allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//         allocInfo.commandPool = commandPool;
//         allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//         allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

//         if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
//             throw std::runtime_error("failed to allocate command buffers!");
//         }
//         for (size_t i = 0; i < commandBuffers.size(); i++) {
//             VkCommandBufferBeginInfo beginInfo = {};
//             beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//             beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
//             beginInfo.pInheritanceInfo = nullptr; // Optional

//             if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
//                 throw std::runtime_error("failed to begin recording command buffer!");
//             }
//             VkRenderPassBeginInfo renderPassInfo = {};
//             renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//             renderPassInfo.renderPass = renderPass;
//             renderPassInfo.framebuffer = swapChainFramebuffers[i];
//             renderPassInfo.renderArea.offset = {0, 0};
//             renderPassInfo.renderArea.extent = swapChainExtent;
//             VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
//             renderPassInfo.clearValueCount = 1;
//             renderPassInfo.pClearValues = &clearColor;
//             vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//             vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
//             vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
//             vkCmdEndRenderPass(commandBuffers[i]);
//             if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
//                 throw std::runtime_error("failed to record command buffer!");
//             }
//         }
//     }

//     void createCommandPool() {
//         QueueFamilyIndices queue_family_indices = find_queue_families(physical_device);

//         VkCommandPoolCreateInfo poolInfo = {};
//         poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//         poolInfo.queueFamilyIndex = queue_family_indices.graphicsFamily;
//         poolInfo.flags = 0; // Optional
//         if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create command pool!");
//         }
//     }

//     void createFramebuffers() {
//         swapChainFramebuffers.resize(swapChainImageViews.size());
//         for (size_t i = 0; i < swapChainImageViews.size(); i++) {
//             VkImageView attachments[] = {
//                 swapChainImageViews[i]
//             };

//             VkFramebufferCreateInfo framebufferInfo = {};
//             framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//             framebufferInfo.renderPass = renderPass;
//             framebufferInfo.attachmentCount = 1;
//             framebufferInfo.pAttachments = attachments;
//             framebufferInfo.width = swapChainExtent.width;
//             framebufferInfo.height = swapChainExtent.height;
//             framebufferInfo.layers = 1;

//             if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
//                 throw std::runtime_error("failed to create framebuffer!");
//             }
//         }
//     }

//     VkShaderModule createShaderModule(const std::vector<char>& code) {
//         VkShaderModuleCreateInfo create_info = {};
//         create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//         create_info.codeSize = code.size();
//         create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());  // std::vector enforces worst case alignment
//         VkShaderModule shaderModule;
//         if (vkCreateShaderModule(device, &create_info, nullptr, &shaderModule) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create shader module!");
//         }
//         return shaderModule;
//     }

//     void drawFrame() {
//         uint32_t imageIndex;
//         vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
//         VkSubmitInfo submitInfo = {};
//         submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

//         VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
//         // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT ensures the render passes don't begin until the image is available,
//         // or a VkSubpassDependency can be used
//         VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
//         submitInfo.waitSemaphoreCount = 1;
//         submitInfo.pWaitSemaphores = waitSemaphores;
//         submitInfo.pWaitDstStageMask = waitStages;
//         submitInfo.commandBufferCount = 1;
//         submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
//         submitInfo.signalSemaphoreCount = 1;
//         VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
//         submitInfo.pSignalSemaphores = signalSemaphores;
//         if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
//             throw std::runtime_error("failed to submit draw command buffer!");
//         }

//         VkPresentInfoKHR presentInfo = {};
//         presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

//         presentInfo.waitSemaphoreCount = 1;
//         presentInfo.pWaitSemaphores = signalSemaphores;
//         VkSwapchainKHR swapChains[] = {swapChain};
//         presentInfo.swapchainCount = 1;
//         presentInfo.pSwapchains = swapChains;
//         presentInfo.pImageIndices = &imageIndex;
//         presentInfo.pResults = nullptr; // Optional
//         vkQueuePresentKHR(presentQueue, &presentInfo);
//         vkQueueWaitIdle(presentQueue);
//     }
// };
