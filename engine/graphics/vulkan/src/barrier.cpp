#include <afterlight/graphics/vulkan/barrier.hpp>

namespace afterlight::graphics::vulkan
{

VulkanResourceState translate_resource_state(rhi::ResourceState state) noexcept
{
    switch (state)
    {
        case rhi::ResourceState::undefined:
            return {
                .stage_mask = VK_PIPELINE_STAGE_2_NONE,
                .access_mask = VK_ACCESS_2_NONE,
                .layout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

        case rhi::ResourceState::color_attachment:
            return {
                .stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };

        case rhi::ResourceState::present:
            return {
                .stage_mask = VK_PIPELINE_STAGE_2_NONE,
                .access_mask = VK_ACCESS_2_NONE,
                .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            };
    }

    return {};
}

VkImageMemoryBarrier2 make_image_barrier(VkImage image,
                                         const rhi::TextureTransition& transition) noexcept
{
    const VulkanResourceState before = translate_resource_state(transition.before);

    const VulkanResourceState after = translate_resource_state(transition.after);

    VkImageMemoryBarrier2 barrier{};

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    barrier.srcStageMask = before.stage_mask;
    barrier.srcAccessMask = before.access_mask;
    barrier.dstStageMask = after.stage_mask;
    barrier.dstAccessMask = after.access_mask;
    barrier.oldLayout = before.layout;
    barrier.newLayout = after.layout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;

    barrier.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    return barrier;
}

} // namespace afterlight::graphics::vulkan
