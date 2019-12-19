#include <vulkan/vulkan.hpp>

class Vulkan {
public:
    Vulkan(const std::vector<const char*>& required_extensions);
    VkInstance get_instance() const { return instance_.get(); };
    void initialize(const VkSurfaceKHR surface, const VkExtent2D& surface_extent);

private:
    const vk::UniqueInstance instance_;
    VkSurfaceKHR surface_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    int graphics_queue_family_index_, present_queue_family_index_;
    vk::Queue graphics_queue_, present_queue_;
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchain_images_;

    vk::UniqueInstance create_instance(const std::vector<const char*>& required_extensions);
    void choose_physical_device();
    bool is_device_suitable(vk::PhysicalDevice device);
    std::pair<int, int> get_graphics_and_present_queue_families(vk::PhysicalDevice device);
    bool required_extensions_supported(vk::PhysicalDevice device);
    void create_logical_device();
    void create_swapchain(const VkExtent2D& surface_extent);
    std::vector<VkImageView> create_image_views(VkDevice device, const std::vector<VkImage>& swapchain_images);
};
