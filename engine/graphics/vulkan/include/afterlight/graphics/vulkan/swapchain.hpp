#pragma once

#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <afterlight/platform/platform.hpp>
#include <afterlight/rhi/frame_scheduler.hpp>
#include <afterlight/rhi/resource.hpp>
#include <afterlight/rhi/resource_state_tracker.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

class GpuMesh;
class MeshPipeline;

struct SwapchainInfo final
{
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t image_count{};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkPresentModeKHR present_mode{VK_PRESENT_MODE_FIFO_KHR};
};

struct GeometryInfo final
{
    std::uint32_t vertex_count{};
    std::uint32_t index_count{};
};

[[nodiscard]] VkSurfaceFormatKHR choose_surface_format(std::span<const VkSurfaceFormatKHR> formats);

[[nodiscard]] VkPresentModeKHR choose_present_mode(std::span<const VkPresentModeKHR> modes);

[[nodiscard]] VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilities,
                                                 platform::WindowSize pixel_size) noexcept;

[[nodiscard]] std::uint32_t
choose_swapchain_image_count(const VkSurfaceCapabilitiesKHR& capabilities) noexcept;

class SwapchainRenderer final
{
public:
    SwapchainRenderer(VulkanContext& context,
                      const platform::Window& window,
                      std::filesystem::path shader_directory);

    ~SwapchainRenderer();

    SwapchainRenderer(const SwapchainRenderer&) = delete;
    SwapchainRenderer& operator=(const SwapchainRenderer&) = delete;
    SwapchainRenderer(SwapchainRenderer&&) = delete;
    SwapchainRenderer& operator=(SwapchainRenderer&&) = delete;

    [[nodiscard]] bool render_frame(const platform::Window& window);

    void request_resize() noexcept;

    [[nodiscard]] const SwapchainInfo& info() const noexcept;

    [[nodiscard]] GeometryInfo geometry_info() const noexcept;

private:
    static constexpr std::size_t frames_in_flight = 2;

    struct FrameResources final
    {
        VkCommandBuffer command_buffer{VK_NULL_HANDLE};

        VkSemaphore image_available{VK_NULL_HANDLE};

        VkFence render_fence{VK_NULL_HANDLE};
    };

    void create_command_resources();
    void create_frame_resources();

    [[nodiscard]] bool recreate_swapchain(const platform::Window& window);

    [[nodiscard]] bool create_swapchain(const platform::Window& window);

    void create_image_views();
    void create_image_resources();
    void create_render_finished_semaphores();

    void destroy_swapchain() noexcept;
    void destroy_frame_resources() noexcept;
    void destroy_command_resources() noexcept;

    void wait_for_image(std::uint32_t image_index, VkFence frame_fence);

    void record_commands(VkCommandBuffer command_buffer, std::uint32_t image_index);

    void submit_frame(const FrameResources& frame, std::uint32_t image_index);

    [[nodiscard]] VkResult present_frame(std::uint32_t image_index);

    [[nodiscard]] static rhi::TextureFormat texture_format(VkFormat format) noexcept;

    VulkanContext& context_;
    std::filesystem::path shader_directory_;

    std::chrono::steady_clock::time_point animation_start_;

    VkCommandPool command_pool_{VK_NULL_HANDLE};
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};

    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    std::vector<VkSemaphore> render_finished_;
    std::vector<VkFence> image_fences_;
    std::vector<rhi::TextureHandle> image_resources_;

    std::unique_ptr<GpuMesh> gpu_mesh_;
    std::unique_ptr<MeshPipeline> mesh_pipeline_;

    std::array<FrameResources, frames_in_flight> frames_{};

    rhi::ResourceRegistry resource_registry_;
    rhi::ResourceStateTracker resource_states_;

    rhi::FrameScheduler frame_scheduler_{static_cast<std::uint32_t>(frames_in_flight)};

    bool resize_requested_{};

    SwapchainInfo info_;
};

} // namespace afterlight::graphics::vulkan
