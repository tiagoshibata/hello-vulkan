#include <functional>
#include <utility>

#include <vulkan/vulkan.hpp>

class Vulkan {
public:
    Vulkan(const std::vector<const char*>& required_extensions, std::function<std::pair<int, int>()>  get_extent, std::function<void()>  wait_window_show_event);
    [[nodiscard]] VkInstance get_instance() const { return instance_.get(); };
    void initialize(const VkSurfaceKHR surface);
    void draw_frame();

private:
    const vk::UniqueInstance instance_;
    const std::function<std::pair<int, int>()> get_extent_;
    const std::function<void()> wait_window_show_event_;
    vk::UniqueSurfaceKHR surface_;
    vk::PhysicalDevice physical_device_;
    vk::UniqueDevice device_;
    int graphics_queue_family_index_, present_queue_family_index_;
    vk::Queue graphics_queue_, present_queue_;
    vk::Extent2D surface_extent_;
    vk::UniqueSwapchainKHR swapchain_;
    vk::Format swapchain_format_;
    std::vector<vk::Image> swapchain_images_;
    std::vector<vk::UniqueImageView> swapchain_image_views_;
    vk::UniqueRenderPass render_pass_;
    vk::UniquePipelineLayout pipeline_layout_;
    vk::UniquePipeline graphics_pipeline_;
    std::vector<vk::UniqueFramebuffer> swapchain_frame_buffers_;
    vk::UniqueCommandPool command_pool_;
    std::vector<vk::UniqueCommandBuffer> command_buffers_;
    vk::UniqueSemaphore image_available_semaphore_;
    vk::UniqueSemaphore render_finished_semaphore_;

    vk::UniqueInstance create_instance(const std::vector<const char*>& required_extensions);
    void choose_physical_device();
    bool is_device_suitable(const vk::PhysicalDevice device);
    [[nodiscard]] std::pair<int, int> get_graphics_and_present_queue_families(const vk::PhysicalDevice device) const;
    void create_logical_device();
    void recreate_swapchain();
    vk::SurfaceCapabilitiesKHR update_surface_capabilities();
    void create_swapchain();
    void create_image_views();
    void create_render_pass();
    vk::UniqueShaderModule create_shader_module(const uint32_t *spirv, size_t code_size);
    void create_pipeline();
    void create_framebuffers();
    void create_command_pool();
    void create_command_buffers();
    void create_semaphores();
};
