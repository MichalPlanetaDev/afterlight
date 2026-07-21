#pragma once

#include <afterlight/rhi/resource_state_tracker.hpp>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

struct VulkanResourceState final
{
    VkPipelineStageFlags2 stage_mask{VK_PIPELINE_STAGE_2_NONE};

    VkAccessFlags2 access_mask{VK_ACCESS_2_NONE};

    VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
};

[[nodiscard]] VulkanResourceState translate_resource_state(rhi::ResourceState state) noexcept;

[[nodiscard]] VkImageMemoryBarrier2
make_image_barrier(VkImage image, const rhi::TextureTransition& transition) noexcept;

} // namespace afterlight::graphics::vulkan
