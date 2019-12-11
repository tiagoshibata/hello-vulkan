#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "sdl_window.hh"

const auto APPLICATION_NAME = "Vulkan demo";
const auto SWAPCHAIN_EXTENSION = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

class Vulkan {
public:
    VkInstance create_instance(const std::vector<const char*>& required_extensions) {
        print_extensions();

        VkApplicationInfo app_info {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = APPLICATION_NAME,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = nullptr,
            .engineVersion = 0,
            .apiVersion = VK_API_VERSION_1_1,
        };
        VkInstanceCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
            .ppEnabledExtensionNames = required_extensions.data(),
        };
        VkInstance instance;
        if (auto status = vkCreateInstance(&create_info, nullptr, &instance); status != VK_SUCCESS) {
            throw std::runtime_error("vkCreateInstance: " + status);
        }
        return instance;
    }

    VkPhysicalDevice pick_physical_device(const VkInstance instance, VkSurfaceKHR surface) {
        uint32_t count;
        if (auto status = vkEnumeratePhysicalDevices(instance, &count, nullptr); status != VK_SUCCESS)
            throw std::runtime_error("vkEnumeratePhysicalDevices: " + status);
        std::vector<VkPhysicalDevice> devices(count);
        if (auto status = vkEnumeratePhysicalDevices(instance, &count, devices.data()); status != VK_SUCCESS)
            throw std::runtime_error("vkEnumeratePhysicalDevices: " + status);
        auto device = std::find_if(std::begin(devices), std::end(devices), [surface](VkPhysicalDevice device){ return is_device_suitable(device, surface); });
        if (device == std::end(devices))
            throw std::runtime_error("No GPU with required Vulkan features found");
        return *device;
    }

    std::tuple<VkDevice, VkQueue, VkQueue> create_logical_device(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
        float queue_priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = 0,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
        std::array<VkDeviceQueueCreateInfo, 2> queue_create_infos{queue_create_info, queue_create_info};
        std::tie(queue_create_infos[0].queueFamilyIndex, queue_create_infos[1].queueFamilyIndex) = get_graphics_and_present_queue_families(physical_device, surface);

        VkDeviceCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
            .pQueueCreateInfos = queue_create_infos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = &SWAPCHAIN_EXTENSION,
            .pEnabledFeatures = nullptr,
        };
        VkDevice device;
        if (auto status = vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS; status != VK_SUCCESS)
            throw std::runtime_error("vkCreateDevice: " + status);
        VkQueue graphics_queue, present_queue;
        vkGetDeviceQueue(device, queue_create_infos[0].queueFamilyIndex, 0, &graphics_queue);
        vkGetDeviceQueue(device, queue_create_infos[1].queueFamilyIndex, 0, &present_queue);
        return {device, graphics_queue, present_queue};
    }

private:
    int graphics_queue_family_index, present_queue_family_index;

    void print_extensions() {
        uint32_t extension_count;
        if (auto status = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr); status != VK_SUCCESS)
            throw std::runtime_error("vkEnumerateInstanceExtensionProperties: " + status);
        std::vector<VkExtensionProperties> extensions(extension_count);
        if (auto status = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data()); status != VK_SUCCESS)
            throw std::runtime_error("vkEnumerateInstanceExtensionProperties: " + status);
        std::cout << "Available extensions:\n";
        for (const auto& extension : extensions)
            std::cout << "\t" << extension.extensionName << "\n";
    }

    static bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << "Trying device " << deviceProperties.deviceName << "\n";

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        auto indices = get_graphics_and_present_queue_families(device, surface);
        return indices.first != -1 && indices.second != -1 && required_extensions_supported(device) && swapchain_supported(device, surface);
    }

    static std::pair<int, int> get_graphics_and_present_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        if (!queue_family_count)
            return {-1, -1};
        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties.data());

        auto graphics = std::find_if(std::begin(queue_family_properties), std::end(queue_family_properties), [](const VkQueueFamilyProperties& properties){ return properties.queueFlags & VK_QUEUE_GRAPHICS_BIT; });
        uint32_t graphics_index = graphics == std::end(queue_family_properties) ? -1 : graphics - std::begin(queue_family_properties);

        for (unsigned i = 0; i < queue_family_count; i++) {
            VkBool32 presentSupport;
            if (auto status = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport); status != VK_SUCCESS)
                throw std::runtime_error("vkGetPhysicalDeviceSurfaceSupportKHR: " + status);
            if (presentSupport)
                return {graphics_index, i};
        }
        return {graphics_index, -1};
    }

    static bool required_extensions_supported(VkPhysicalDevice device) {
        uint32_t extension_count;
        if (auto status = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr); status != VK_SUCCESS)
            throw std::runtime_error("vkEnumerateDeviceExtensionProperties: " + status);
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        if (auto status = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data()); status != VK_SUCCESS)
            throw std::runtime_error("vkEnumerateDeviceExtensionProperties: " + status);
        // For now, check for VK_KHR_SWAPCHAIN_EXTENSION_NAME only
        return std::any_of(std::begin(available_extensions), std::end(available_extensions), [](const VkExtensionProperties& extension) { return !std::strcmp(extension.extensionName, SWAPCHAIN_EXTENSION); });
    }

    static bool swapchain_supported(VkPhysicalDevice device, VkSurfaceKHR surface) {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        return formatCount && presentModeCount;
    }
};

class App {
public:
    void run() {
        SDLWindow window;
        Vulkan vulkan;
        const auto instance = vulkan.create_instance(window.get_vulkan_extensions());
        const auto surface = window.create_vulkan_surface(instance);
        const auto physical_device = vulkan.pick_physical_device(instance, surface);
        VkDevice logical_device;
        VkQueue graphics_queue, present_queue;
        std::tie(logical_device, graphics_queue, present_queue) = vulkan.create_logical_device(physical_device, surface);
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




// struct QueueFamilyIndices {
//     int graphicsFamily = -1;
//     int presentFamily = -1;

//     bool isComplete() {
//         return graphicsFamily >= 0 && presentFamily >= 0;
//     }
// };

// struct SwapChainSupportDetails {
//     VkSurfaceCapabilitiesKHR capabilities;
//     std::vector<VkSurfaceFormatKHR> formats;
//     std::vector<VkPresentModeKHR> presentModes;
// };

// class OldVulkan {
// public:
//     void run() {
//         createSwapChain();
//         createImageViews();
//         createRenderPass();
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
//     VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
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
//         QueueFamilyIndices queueFamilyIndices = find_queue_families(physicalDevice);

//         VkCommandPoolCreateInfo poolInfo = {};
//         poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//         poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
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

//     void createGraphicsPipeline() {
//         auto vertShaderCode = readFile("../src/vulkan_demo/shader.vert.spv");
//         auto fragShaderCode = readFile("../src/vulkan_demo/shader.frag.spv");
//         VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
//         VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

//         VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
//         vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//         vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//         vertShaderStageInfo.module = vertShaderModule;
//         vertShaderStageInfo.pName = "main";

//         VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
//         fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//         fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//         fragShaderStageInfo.module = fragShaderModule;
//         fragShaderStageInfo.pName = "main";

//         VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

//         VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
//         vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//         vertexInputInfo.vertexBindingDescriptionCount = 0;
//         vertexInputInfo.vertexAttributeDescriptionCount = 0;

//         VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
//         inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//         inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//         inputAssembly.primitiveRestartEnable = VK_FALSE;

//         VkViewport viewport = {};
//         viewport.x = 0.0f;
//         viewport.y = 0.0f;
//         viewport.width = (float) swapChainExtent.width;
//         viewport.height = (float) swapChainExtent.height;
//         viewport.minDepth = 0.0f;
//         viewport.maxDepth = 1.0f;
//         VkRect2D scissor = {};
//         scissor.offset = {0, 0};
//         scissor.extent = swapChainExtent;

//         VkPipelineViewportStateCreateInfo viewportState = {};
//         viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//         viewportState.viewportCount = 1;
//         viewportState.pViewports = &viewport;
//         viewportState.scissorCount = 1;
//         viewportState.pScissors = &scissor;

//         VkPipelineRasterizationStateCreateInfo rasterizer = {};
//         rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//         rasterizer.depthClampEnable = VK_FALSE;
//         rasterizer.rasterizerDiscardEnable = VK_FALSE;
//         rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//         rasterizer.lineWidth = 1.0f;
//         rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//         rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
//         rasterizer.depthBiasEnable = VK_FALSE;

//         VkPipelineMultisampleStateCreateInfo multisampling = {};
//         multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//         multisampling.sampleShadingEnable = VK_FALSE;
//         multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//         multisampling.minSampleShading = 1.0f; // Optional
//         multisampling.pSampleMask = nullptr; // Optional
//         multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
//         multisampling.alphaToOneEnable = VK_FALSE; // Optional

//         VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
//         colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//         // if (blendEnable) {
//         //     finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
//         //     finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
//         // } else {
//         //     finalColor = newColor;
//         // }

//         // finalColor = finalColor & colorWriteMask;
//         colorBlendAttachment.blendEnable = VK_FALSE;
//         colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//         colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//         colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
//         colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//         colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//         colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

//         VkPipelineColorBlendStateCreateInfo colorBlending = {};
//         colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//         colorBlending.logicOpEnable = VK_FALSE;
//         colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
//         colorBlending.attachmentCount = 1;
//         colorBlending.pAttachments = &colorBlendAttachment;
//         colorBlending.blendConstants[0] = 0.0f; // Optional
//         colorBlending.blendConstants[1] = 0.0f; // Optional
//         colorBlending.blendConstants[2] = 0.0f; // Optional
//         colorBlending.blendConstants[3] = 0.0f; // Optional

//         VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
//         pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//         pipelineLayoutInfo.setLayoutCount = 0; // Optional
//         pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
//         pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
//         pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

//         if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create pipeline layout!");
//         }

//         VkGraphicsPipelineCreateInfo pipelineInfo = {};
//         pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//         pipelineInfo.stageCount = 2;
//         pipelineInfo.pStages = shaderStages;
//         pipelineInfo.pVertexInputState = &vertexInputInfo;
//         pipelineInfo.pInputAssemblyState = &inputAssembly;
//         pipelineInfo.pViewportState = &viewportState;
//         pipelineInfo.pRasterizationState = &rasterizer;
//         pipelineInfo.pMultisampleState = &multisampling;
//         pipelineInfo.pDepthStencilState = nullptr; // Optional
//         pipelineInfo.pColorBlendState = &colorBlending;
//         pipelineInfo.pDynamicState = nullptr; // Optional
//         pipelineInfo.layout = pipelineLayout;
//         pipelineInfo.renderPass = renderPass;
//         pipelineInfo.subpass = 0;
//         pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
//         pipelineInfo.basePipelineIndex = -1; // Optional

//         if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create graphics pipeline!");
//         }

//         vkDestroyShaderModule(device, fragShaderModule, nullptr);
//         vkDestroyShaderModule(device, vertShaderModule, nullptr);
//     }

//     void createRenderPass() {
//         VkAttachmentDescription colorAttachment = {};
//         colorAttachment.format = swapChainImageFormat;
//         colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
//         colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//         colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//         colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//         colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//         colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//         colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//         VkAttachmentReference colorAttachmentRef = {};
//         colorAttachmentRef.attachment = 0;
//         colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//         VkSubpassDescription subpass = {};
//         subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//         subpass.colorAttachmentCount = 1;
//         subpass.pColorAttachments = &colorAttachmentRef;
//         VkRenderPassCreateInfo renderPassInfo = {};
//         renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//         renderPassInfo.attachmentCount = 1;
//         renderPassInfo.pAttachments = &colorAttachment;
//         renderPassInfo.subpassCount = 1;
//         renderPassInfo.pSubpasses = &subpass;

//         VkSubpassDependency dependency = {};
//         dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//         dependency.dstSubpass = 0;
//         dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//         dependency.srcAccessMask = 0;
//         dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//         dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//         renderPassInfo.dependencyCount = 1;
//         renderPassInfo.pDependencies = &dependency;

//         if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create render pass!");
//         }

//     }

//     void createImageViews() {
//         swapChainImageViews.resize(swapChainImages.size());
//         for (size_t i = 0; i < swapChainImages.size(); i++) {
//             VkImageViewCreateInfo create_info = {};
//             create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//             create_info.image = swapChainImages[i];
//             create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
//             create_info.format = swapChainImageFormat;
//             create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//             create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//             create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//             create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//             create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//             create_info.subresourceRange.baseMipLevel = 0;
//             create_info.subresourceRange.levelCount = 1;
//             create_info.subresourceRange.baseArrayLayer = 0;
//             create_info.subresourceRange.layerCount = 1;
//             if (vkCreateImageView(device, &create_info, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
//                 throw std::runtime_error("failed to create image views!");
//             }
//         }
//     }

//     void createSwapChain() {
//         SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

//         VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
//         VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
//         VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

//         uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
//         if (swapChainSupport.capabilities.maxImageCount && imageCount > swapChainSupport.capabilities.maxImageCount) {
//             imageCount = swapChainSupport.capabilities.maxImageCount;
//         }

//         VkSwapchainCreateInfoKHR create_info = {};
//         create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//         create_info.surface = surface;
//         create_info.minImageCount = imageCount;
//         create_info.imageFormat = surfaceFormat.format;
//         create_info.imageColorSpace = surfaceFormat.colorSpace;
//         create_info.imageExtent = extent;
//         create_info.imageArrayLayers = 1;
//         create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

//         QueueFamilyIndices indices = find_queue_families(physicalDevice);
//         uint32_t queueFamilyIndices[2];

//         if (indices.graphicsFamily != indices.presentFamily) {
//             queueFamilyIndices[0] = (uint32_t) indices.graphicsFamily;
//             queueFamilyIndices[1] = (uint32_t) indices.presentFamily;
//             create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
//             create_info.queueFamilyIndexCount = 2;
//             create_info.pQueueFamilyIndices = queueFamilyIndices;
//         } else {
//             create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//             create_info.queueFamilyIndexCount = 0; // Optional
//             create_info.pQueueFamilyIndices = nullptr; // Optional
//         }
//         create_info.preTransform = swapChainSupport.capabilities.currentTransform;
//         create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
//         create_info.presentMode = presentMode;
//         create_info.clipped = VK_TRUE;
//         create_info.oldSwapchain = VK_NULL_HANDLE;
//         if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapChain) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create swap chain!");
//         }
//         swapChainImageFormat = surfaceFormat.format;
//         swapChainExtent = extent;
//         if (vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr) != VK_SUCCESS) {
//             abort();
//         }
//         swapChainImages.resize(imageCount);
//         vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
//     }

//     void createLogicalDevice() {
//         QueueFamilyIndices indices = find_queue_families(physicalDevice);

//         std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
//         std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

//         float queuePriority = 1.0f;
//         for (int queueFamily : uniqueQueueFamilies) {
//             VkDeviceQueueCreateInfo queueCreateInfo = {};
//             queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//             queueCreateInfo.queueFamilyIndex = queueFamily;
//             queueCreateInfo.queueCount = 1;
//             queueCreateInfo.pQueuePriorities = &queuePriority;
//             queueCreateInfos.push_back(queueCreateInfo);
//         }

//         VkPhysicalDeviceFeatures deviceFeatures = {};
//         VkDeviceCreateInfo create_info = {};
//         create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//         create_info.pEnabledFeatures = &deviceFeatures;
//         create_info.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
//         create_info.ppEnabledExtensionNames = deviceExtensions.data();
//         create_info.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
//         create_info.pQueueCreateInfos = queueCreateInfos.data();
//         create_info.enabledLayerCount = 0;
//         if (vkCreateDevice(physicalDevice, &create_info, nullptr, &device) != VK_SUCCESS) {
//             throw std::runtime_error("failed to create logical device!");
//         }
//         vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
//         vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
//     }

//     VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
//         if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
//             return capabilities.currentExtent;
//         } else {
//             VkExtent2D actualExtent = {WIDTH, HEIGHT};
//             actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
//             actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
//             return actualExtent;
//         }
//     }

//     VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
//         // for (const auto& availablePresentMode : availablePresentModes) {
//         //     if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
//         //         return availablePresentMode;
//         //     }
//         // }

//         return VK_PRESENT_MODE_FIFO_KHR;
//     }

//     VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
//         if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
//             return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
//         }
//         for (const auto& availableFormat : availableFormats) {
//             if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//                 return availableFormat;
//             }
//         }
//         return availableFormats[0];
//     }

//     void mainLoop() {
//         // while (!glfwWindowShouldClose(window)) {
//         //     glfwPollEvents();
//         //     drawFrame();
//         // }
//         vkDeviceWaitIdle(device);
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

//     void cleanup() {
//         vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
//         vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
//         vkDestroyCommandPool(device, commandPool, nullptr);
//         for (auto framebuffer : swapChainFramebuffers) {
//             vkDestroyFramebuffer(device, framebuffer, nullptr);
//         }
//         vkDestroyPipeline(device, graphicsPipeline, nullptr);
//         vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
//         vkDestroyRenderPass(device, renderPass, nullptr);
//         for (auto imageView : swapChainImageViews) {
//             vkDestroyImageView(device, imageView, nullptr);
//         }
//         vkDestroySwapchainKHR(device, swapChain, nullptr);
//         vkDestroyDevice(device, nullptr);
//         vkDestroySurfaceKHR(instance, surface, nullptr);
//         vkDestroyInstance(instance, nullptr);
//     }
// };
