#include <vulkan/vulkan.hpp>

class Vulkan {
public:
    Vulkan(const std::vector<const char*>& required_extensions);
    VkInstance get_instance() const { return instance_.get(); };
    void initialize(const VkSurfaceKHR surface, int surface_width, int surface_height);

private:
    const vk::UniqueInstance instance_;
    vk::UniqueSurfaceKHR surface_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    int graphics_queue_family_index_, present_queue_family_index_;
    vk::Queue graphics_queue_, present_queue_;
    vk::Extent2D surface_extent_;
    vk::UniqueSwapchainKHR swapchain_;
    vk::Format swapchain_format_;
    std::vector<vk::Image> swapchain_images_;
    std::vector<vk::ImageView> swapchain_image_views_;

    vk::UniqueInstance create_instance(const std::vector<const char*>& required_extensions);
    void choose_physical_device();
    bool is_device_suitable(const vk::PhysicalDevice device);
    std::pair<int, int> get_graphics_and_present_queue_families(const vk::PhysicalDevice device);
    bool required_extensions_supported(const vk::PhysicalDevice device);
    void create_logical_device();
    void create_swapchain(int surface_width, int surface_height);
    void create_image_views();
};
