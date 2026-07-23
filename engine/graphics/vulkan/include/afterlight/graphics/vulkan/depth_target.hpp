#pragma once

#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

[[nodiscard]] VkFormat choose_depth_format(std::span<const VkFormat> supported_formats);

[[nodiscard]] std::uint32_t depth_target_count_for_swapchain(std::size_t swapchain_image_count);

class DepthTarget final
{
public:
    DepthTarget(VulkanContext& context, VkExtent2D extent);

    ~DepthTarget();

    DepthTarget(const DepthTarget&) = delete;
    DepthTarget& operator=(const DepthTarget&) = delete;
    DepthTarget(DepthTarget&&) = delete;
    DepthTarget& operator=(DepthTarget&&) = delete;

    void prepare(VkCommandBuffer command_buffer);

    [[nodiscard]] VkImageView image_view() const noexcept;

    [[nodiscard]] VkFormat format() const noexcept;

private:
    [[nodiscard]] VkFormat select_supported_format() const;

    [[nodiscard]] std::uint32_t select_memory_type(std::uint32_t allowed_types) const;

    void reset() noexcept;

    VulkanContext& context_;

    VkImage image_{VK_NULL_HANDLE};
    VkDeviceMemory memory_{VK_NULL_HANDLE};
    VkImageView image_view_{VK_NULL_HANDLE};
    VkFormat format_{VK_FORMAT_UNDEFINED};

    bool initialized_{};
};

} // namespace afterlight::graphics::vulkan
