#include "vulkan.hh"

#include <iostream>

const auto APPLICATION_NAME = "Vulkan demo";
const auto SWAPCHAIN_EXTENSION = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

namespace {
void print_extensions() {
    std::cout << "Available extensions:\n";
    for (const auto& extension : vk::enumerateInstanceExtensionProperties())
        std::cout << "\t" << extension.extensionName << "\n";
}

bool required_extensions_supported(const vk::PhysicalDevice device) {
    const auto extensions = device.enumerateDeviceExtensionProperties();
    // For now, check for VK_KHR_SWAPCHAIN_EXTENSION_NAME only
    return std::any_of(std::begin(extensions), std::end(extensions), [](const vk::ExtensionProperties& extension) { return !std::strcmp(extension.extensionName, SWAPCHAIN_EXTENSION); });
}
}

Vulkan::Vulkan(const std::vector<const char*>& required_extensions, const std::function<std::pair<int, int>()>& get_extent, const std::function<void()>& wait_window_show_event) :
    instance_(create_instance(required_extensions)), get_extent_(get_extent), wait_window_show_event_(wait_window_show_event) {}

vk::UniqueInstance Vulkan::create_instance(const std::vector<const char*>& required_extensions) {
    print_extensions();
    const vk::ApplicationInfo application_info(APPLICATION_NAME, VK_MAKE_VERSION(1, 0, 0), nullptr, 0, VK_API_VERSION_1_1);
    const vk::InstanceCreateInfo create_info(vk::InstanceCreateFlags(), &application_info, 0, nullptr, static_cast<uint32_t>(required_extensions.size()), required_extensions.data());
    return vk::createInstanceUnique(create_info);
}

void Vulkan::initialize(const VkSurfaceKHR surface) {
    surface_ = vk::UniqueSurfaceKHR(surface, *instance_);
    choose_physical_device();
    create_logical_device();
    create_command_pool();
    create_semaphores();

    recreate_swapchain();
}

void Vulkan::choose_physical_device() {
    const auto devices = instance_->enumeratePhysicalDevices();
    const auto device = std::find_if(std::begin(devices), std::end(devices), [this](VkPhysicalDevice device){ return is_device_suitable(device); });
    if (device == std::end(devices))
        throw std::runtime_error("No GPU with required Vulkan features found");
    physical_device_ = *device;
}

bool Vulkan::is_device_suitable(const vk::PhysicalDevice device) {
    const auto properties = device.getProperties();
    std::cout << "Trying device " << properties.deviceName << "\n";
    // auto features = device.getFeatures(); can be used to check for extra features
    std::tie(graphics_queue_family_index_, present_queue_family_index_) = get_graphics_and_present_queue_families(device);
    return graphics_queue_family_index_ != -1 && present_queue_family_index_ != -1 && required_extensions_supported(device);
}

std::pair<int, int> Vulkan::get_graphics_and_present_queue_families(const vk::PhysicalDevice device) const {
    const auto supports_graphics = [](const vk::QueueFamilyProperties& properties){ return properties.queueFlags & vk::QueueFlagBits::eGraphics; };
    const auto queue_family_properties = device.getQueueFamilyProperties();
    uint32_t present_index = -1;

    for (unsigned i = 0; i < queue_family_properties.size(); i++) {
        if (device.getSurfaceSupportKHR(i, *surface_)) {
            if (supports_graphics(queue_family_properties[i])) {
                return {i, i};
            }
            present_index = i;
        }
    }
    const auto graphics = std::find_if(std::begin(queue_family_properties), std::end(queue_family_properties), supports_graphics);
    uint32_t graphics_index = graphics == std::end(queue_family_properties) ? -1 : graphics - std::begin(queue_family_properties);
    return {graphics_index, present_index};
}

void Vulkan::create_logical_device() {
    const float queue_priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_create_info({}, 0, 1, &queue_priority);
    const std::array<vk::DeviceQueueCreateInfo, 2> queue_create_infos {
        vk::DeviceQueueCreateInfo({}, graphics_queue_family_index_, 1, &queue_priority),
        vk::DeviceQueueCreateInfo({}, present_queue_family_index_, 1, &queue_priority)
    };
    const vk::DeviceCreateInfo create_info({}, graphics_queue_family_index_ == present_queue_family_index_ ? 1 : 2, queue_create_infos.data(), 0, nullptr, 1, &SWAPCHAIN_EXTENSION);
    device_ = physical_device_.createDeviceUnique(create_info);
    graphics_queue_ = device_->getQueue(graphics_queue_family_index_, 0);
    present_queue_ = device_->getQueue(present_queue_family_index_, 0);
}

void Vulkan::recreate_swapchain() {
    device_->waitIdle();
    create_swapchain();
    create_image_views();
    create_render_pass();
    create_pipeline();
    create_framebuffers();
    create_command_buffers();
}

vk::SurfaceCapabilitiesKHR Vulkan::update_surface_capabilities() {
    for (;;) {
        auto capabilities = physical_device_.getSurfaceCapabilitiesKHR(*surface_);
        // "currentExtent is the current width and height of the surface, or the special value (0xFFFFFFFF, 0xFFFFFFFF) indicating
        // that the surface size will be determined by the extent of a swapchain targeting the surface"
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() || capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()) {
            surface_extent_ = capabilities.currentExtent;
        } else {
            std::tie(surface_extent_.width, surface_extent_.height) = get_extent_();
            surface_extent_.width = std::clamp(surface_extent_.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            surface_extent_.height = std::clamp(surface_extent_.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
        // "On some platforms, it is normal that maxImageExtent may become (0, 0), for example when the window is minimized.
        // In such a case, it is not possible to create a swapchain due to the Valid Usage requirements."
        if (surface_extent_.width && surface_extent_.height)
            return capabilities;
        wait_window_show_event_();
    }
}

void Vulkan::create_swapchain() {
    const auto capabilities = update_surface_capabilities();
    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    const auto formats = physical_device_.getSurfaceFormatsKHR(*surface_);
    vk::SurfaceFormatKHR chosen_format = formats[0];
    for (const auto& format : formats) {
        // TODO check for other sRGB colorspaces? One of them must be available:
        // "Vulkan requires that all implementations support the sRGB transfer function by use of an SRGB pixel format"
        if (format.format == vk::Format::eB8G8R8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            chosen_format = format;
            break;
        }
    }
    swapchain_format_ = chosen_format.format;

    vk::SwapchainCreateInfoKHR create_info(vk::SwapchainCreateFlagsKHR(), *surface_, image_count, chosen_format.format, chosen_format.colorSpace, surface_extent_, 1,
        vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, capabilities.currentTransform, vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::PresentModeKHR::eFifo, true);
    std::array<uint32_t, 2> queue_family_indices;
    if (graphics_queue_family_index_ != present_queue_family_index_) {
        queue_family_indices = {static_cast<uint32_t>(graphics_queue_family_index_), static_cast<uint32_t>(present_queue_family_index_)};
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    swapchain_ = device_->createSwapchainKHRUnique(create_info);

    swapchain_images_ = device_->getSwapchainImagesKHR(*swapchain_);
}

void Vulkan::create_image_views() {
    swapchain_image_views_.resize(swapchain_images_.size());
    for (unsigned i = 0; i < swapchain_images_.size(); i++) {
        const vk::ImageViewCreateInfo create_info(vk::ImageViewCreateFlags(), swapchain_images_[i], vk::ImageViewType::e2D, swapchain_format_,
            vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        swapchain_image_views_[i] = device_->createImageViewUnique(create_info);
    }
}

void Vulkan::create_render_pass() {
    const vk::AttachmentDescription color_attachment_description(vk::AttachmentDescriptionFlags(), swapchain_format_, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

    const vk::AttachmentReference color_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
    const vk::SubpassDescription subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &color_attachment_reference);

    const vk::SubpassDependency dependency(VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlags(), vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

    const vk::RenderPassCreateInfo create_info(vk::RenderPassCreateFlags(), 1, &color_attachment_description, 1, &subpass, 1, &dependency);
    render_pass_ = device_->createRenderPassUnique(create_info);
}

vk::UniqueShaderModule Vulkan::create_shader_module(const uint32_t *spirv, size_t code_size) {
    const vk::ShaderModuleCreateInfo create_info(vk::ShaderModuleCreateFlags(), code_size, spirv);
    return device_->createShaderModuleUnique(create_info);
}

// TODO remove hardcoded shaders
#include <cstdint>
#include "shader.frag.h"
#include "shader.vert.h"

void Vulkan::create_pipeline() {
    const auto vertex_shader = create_shader_module(shader_vert_spirv, sizeof(shader_vert_spirv));
    const auto fragment_shader = create_shader_module(shader_frag_spirv, sizeof(shader_frag_spirv));

    vk::PipelineShaderStageCreateInfo vertex_create_info({}, vk::ShaderStageFlagBits::eVertex, *vertex_shader, "main");
    vk::PipelineShaderStageCreateInfo fragment_create_info({}, vk::ShaderStageFlagBits::eFragment, *fragment_shader, "main");
    vk::PipelineShaderStageCreateInfo stages_create_info[] = {vertex_create_info, fragment_create_info};
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;

    vk::PipelineInputAssemblyStateCreateInfo input_assembly({}, vk::PrimitiveTopology::eTriangleList);

    vk::Viewport viewport(0., 0., surface_extent_.width, surface_extent_.height, 0., 1.);
    vk::Rect2D scissor({}, surface_extent_);
    vk::PipelineViewportStateCreateInfo viewport_state({}, 1, &viewport, 1, &scissor);
    vk::PipelineRasterizationStateCreateInfo rasterizer({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, {}, {}, {}, 1.);
    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, false);
    vk::PipelineColorBlendAttachmentState color_blend_attachment(false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    vk::PipelineColorBlendStateCreateInfo color_blend_create_info({}, {}, vk::LogicOp::eClear, 1, &color_blend_attachment);

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_ = device_->createPipelineLayoutUnique(pipeline_layout_info);

    vk::GraphicsPipelineCreateInfo pipeline_create_info({}, 2, stages_create_info, &vertex_input_info, &input_assembly, {}, &viewport_state, &rasterizer, &multisampling, {}, &color_blend_create_info, {}, *pipeline_layout_, *render_pass_);
    auto pipeline_result_value = device_->createGraphicsPipelinesUnique(nullptr, pipeline_create_info);
    // if (pipeline_result_value.result != vk::Result::eSuccess) {
    //     // TODO proper error handling
    //     throw std::runtime_error("Failed to create pipeline");
    // }
    graphics_pipeline_ = std::move(pipeline_result_value[0]);
}

void Vulkan::create_framebuffers() {
    swapchain_frame_buffers_.resize(swapchain_image_views_.size());
    for (size_t i = 0; i < swapchain_frame_buffers_.size(); i++) {
        const vk::FramebufferCreateInfo framebuffer_create_info({}, *render_pass_, 1, &*swapchain_image_views_[i], surface_extent_.width, surface_extent_.height, 1);
        swapchain_frame_buffers_[i] = device_->createFramebufferUnique(framebuffer_create_info);
    }
}

void Vulkan::create_command_pool() {
    const vk::CommandPoolCreateInfo command_pool_create_info({}, graphics_queue_family_index_);
    command_pool_ = device_->createCommandPoolUnique(command_pool_create_info);
}

void Vulkan::create_command_buffers() {
    const vk::CommandBufferAllocateInfo allocate_info(*command_pool_, vk::CommandBufferLevel::ePrimary, swapchain_frame_buffers_.size());
    command_buffers_ = device_->allocateCommandBuffersUnique(allocate_info);
    const vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    for (size_t i = 0; i < command_buffers_.size(); i++) {
        command_buffers_[i]->begin(begin_info);

        vk::ClearValue clear_color(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 1.f}));
        vk::RenderPassBeginInfo render_pass_begin_info(*render_pass_, *swapchain_frame_buffers_[i], vk::Rect2D({0, 0}, surface_extent_), 1, &clear_color);

        command_buffers_[i]->beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
        command_buffers_[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *graphics_pipeline_);
        command_buffers_[i]->draw(3, 1, 0, 0);
        command_buffers_[i]->endRenderPass();
        command_buffers_[i]->end();
    }
}

void Vulkan::create_semaphores() {
    vk::SemaphoreCreateInfo create_info;
    image_available_semaphore_ = device_->createSemaphoreUnique(create_info);
    render_finished_semaphore_ = device_->createSemaphoreUnique(create_info);
}

void Vulkan::draw_frame() {
    bool swapchain_outdated = false;
    try {
        const auto image_index = device_->acquireNextImageKHR(*swapchain_, std::numeric_limits<uint64_t>::max(), *image_available_semaphore_, nullptr);

        vk::PipelineStageFlags wait_dst_stage_mask[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::SubmitInfo submit_info(1, &*image_available_semaphore_, wait_dst_stage_mask, 1, &*command_buffers_[image_index.value], 1, &*render_finished_semaphore_);
        graphics_queue_.submit({submit_info}, nullptr);

        vk::PresentInfoKHR present_info(1, &*render_finished_semaphore_, 1, &*swapchain_, &image_index.value);
        present_queue_.waitIdle();
        const auto present_result = present_queue_.presentKHR(present_info);
        swapchain_outdated = image_index.result == vk::Result::eSuboptimalKHR || present_result == vk::Result::eSuboptimalKHR;
    } catch (const vk::OutOfDateKHRError& e) {
        swapchain_outdated = true;
    }
    if (swapchain_outdated)
        recreate_swapchain();
}
