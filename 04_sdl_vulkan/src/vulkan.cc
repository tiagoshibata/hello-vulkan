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
}

Vulkan::Vulkan(const std::vector<const char*>& required_extensions) : instance_(create_instance(required_extensions)) {}

vk::UniqueInstance Vulkan::create_instance(const std::vector<const char*>& required_extensions) {
    print_extensions();
    const vk::ApplicationInfo application_info(APPLICATION_NAME, VK_MAKE_VERSION(1, 0, 0), nullptr, 0, VK_API_VERSION_1_1);
    const vk::InstanceCreateInfo create_info(vk::InstanceCreateFlags(), &application_info, 0, nullptr, static_cast<uint32_t>(required_extensions.size()), required_extensions.data());
    return vk::createInstanceUnique(create_info);
}

void Vulkan::initialize(const VkSurfaceKHR surface, int surface_width, int surface_height) {
    surface_ = vk::UniqueSurfaceKHR(surface, *instance_);
    choose_physical_device();
    create_logical_device();
    create_swapchain(surface_width, surface_height);
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

std::pair<int, int> Vulkan::get_graphics_and_present_queue_families(const vk::PhysicalDevice device) {
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

bool Vulkan::required_extensions_supported(const vk::PhysicalDevice device) {
    const auto extensions = device.enumerateDeviceExtensionProperties();
    // For now, check for VK_KHR_SWAPCHAIN_EXTENSION_NAME only
    return std::any_of(std::begin(extensions), std::end(extensions), [](const vk::ExtensionProperties& extension) { return !std::strcmp(extension.extensionName, SWAPCHAIN_EXTENSION); });
}

void Vulkan::create_logical_device() {
    const float queue_priority = 1.0f;
    vk::DeviceQueueCreateInfo queue_create_info(vk::DeviceQueueCreateFlags(), 0, 1, &queue_priority);
    const std::array<vk::DeviceQueueCreateInfo, 2> queue_create_infos {
        queue_create_info.setQueueFamilyIndex(graphics_queue_family_index_),
        queue_create_info.setQueueFamilyIndex(present_queue_family_index_),
    };
    const vk::DeviceCreateInfo create_info(vk::DeviceCreateFlags(), static_cast<uint32_t>(queue_create_infos.size()), queue_create_infos.data(), 0, nullptr, 1, &SWAPCHAIN_EXTENSION);
    device_ = physical_device_.createDeviceUnique(create_info);
    graphics_queue_ = device_->getQueue(graphics_queue_family_index_, 0);
    present_queue_ = device_->getQueue(present_queue_family_index_, 0);
}

void Vulkan::create_swapchain(int width, int height) {
    surface_extent_.width = width;
    surface_extent_.height = height;
    const auto capabilities = physical_device_.getSurfaceCapabilitiesKHR(*surface_);
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() || capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()) {
        // FIXME check for zero capabilities.maxImageExtent.width / height:
        // "On some platforms, it is normal that maxImageExtent may become (0, 0), for example when the window is minimized.
        // In such a case, it is not possible to create a swapchain due to the Valid Usage requirements."
        surface_extent_.width = std::clamp(surface_extent_.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        surface_extent_.height = std::clamp(surface_extent_.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
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
    swapchain_image_views_.reserve(swapchain_images_.size());
    for (unsigned i = 0; i < swapchain_images_.size(); i++) {
        const vk::ImageViewCreateInfo create_info(vk::ImageViewCreateFlags(), swapchain_images_[i], vk::ImageViewType::e2D, swapchain_format_,
            vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        swapchain_image_views_.push_back(device_->createImageView(create_info));
    }
}
