#include <afterlight/graphics/vulkan/depth_target.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace afterlight::graphics::vulkan
{

namespace
{

[[nodiscard]] std::runtime_error vulkan_failure(std::string_view operation, VkResult result)
{
    return std::runtime_error{std::string{operation} + " failed with VkResult " +
                              std::to_string(static_cast<int>(result))};
}

void require_success(VkResult result, std::string_view operation)
{
    if (result != VK_SUCCESS)
    {
        throw vulkan_failure(operation, result);
    }
}

} // namespace

VkFormat choose_depth_format(std::span<const VkFormat> supported_formats)
{
    constexpr std::array<VkFormat, 3> preferences{
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    for (const VkFormat preferred : preferences)
    {
        const auto match = std::find(supported_formats.begin(), supported_formats.end(), preferred);

        if (match != supported_formats.end())
        {
            return *match;
        }
    }

    throw std::invalid_argument{"no supported depth format was provided"};
}

DepthTarget::DepthTarget(VulkanContext& context, VkExtent2D extent) : context_{context}
{
    if (extent.width == 0 || extent.height == 0)
    {
        throw std::invalid_argument{"depth target extent cannot be zero"};
    }

    try
    {
        format_ = select_supported_format();

        VkImageCreateInfo image_info{};

        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

        image_info.imageType = VK_IMAGE_TYPE_2D;

        image_info.format = format_;

        image_info.extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1,
        };

        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;

        image_info.samples = VK_SAMPLE_COUNT_1_BIT;

        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        require_success(vkCreateImage(context_.device(), &image_info, nullptr, &image_),
                        "vkCreateImage(depth)");

        VkMemoryRequirements requirements{};

        vkGetImageMemoryRequirements(context_.device(), image_, &requirements);

        VkMemoryAllocateInfo allocation_info{};

        allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        allocation_info.allocationSize = requirements.size;

        allocation_info.memoryTypeIndex = select_memory_type(requirements.memoryTypeBits);

        require_success(vkAllocateMemory(context_.device(), &allocation_info, nullptr, &memory_),
                        "vkAllocateMemory(depth)");

        require_success(vkBindImageMemory(context_.device(), image_, memory_, 0),
                        "vkBindImageMemory(depth)");

        VkImageViewCreateInfo view_info{};

        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        view_info.image = image_;

        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        view_info.format = format_;

        view_info.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        require_success(vkCreateImageView(context_.device(), &view_info, nullptr, &image_view_),
                        "vkCreateImageView(depth)");
    }
    catch (...)
    {
        reset();
        throw;
    }
}

DepthTarget::~DepthTarget()
{
    reset();
}

void DepthTarget::prepare(VkCommandBuffer command_buffer)
{
    if (initialized_)
    {
        return;
    }

    if (command_buffer == VK_NULL_HANDLE)
    {
        throw std::invalid_argument{"depth transition requires a command buffer"};
    }

    VkImageMemoryBarrier2 barrier{};

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;

    barrier.srcAccessMask = VK_ACCESS_2_NONE;

    barrier.dstStageMask =
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

    barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image_;

    barrier.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkDependencyInfo dependency{};

    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

    dependency.imageMemoryBarrierCount = 1;

    dependency.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(command_buffer, &dependency);

    initialized_ = true;
}

VkImageView DepthTarget::image_view() const noexcept
{
    return image_view_;
}

VkFormat DepthTarget::format() const noexcept
{
    return format_;
}

VkFormat DepthTarget::select_supported_format() const
{
    constexpr std::array<VkFormat, 3> candidates{
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    std::vector<VkFormat> supported;
    supported.reserve(candidates.size());

    for (const VkFormat candidate : candidates)
    {
        VkFormatProperties properties{};

        vkGetPhysicalDeviceFormatProperties(context_.physical_device(), candidate, &properties);

        const bool depth_attachment = (properties.optimalTilingFeatures &
                                       VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;

        if (depth_attachment)
        {
            supported.push_back(candidate);
        }
    }

    return choose_depth_format(supported);
}

std::uint32_t DepthTarget::select_memory_type(std::uint32_t allowed_types) const
{
    VkPhysicalDeviceMemoryProperties properties{};

    vkGetPhysicalDeviceMemoryProperties(context_.physical_device(), &properties);

    for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
    {
        const bool allowed = (allowed_types & (std::uint32_t{1} << index)) != 0;

        const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;

        const bool device_local = (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;

        if (allowed && device_local)
        {
            return index;
        }
    }

    throw std::runtime_error{"Vulkan device exposes no device-local depth memory"};
}

void DepthTarget::reset() noexcept
{
    if (image_view_ != VK_NULL_HANDLE)
    {
        vkDestroyImageView(context_.device(), image_view_, nullptr);

        image_view_ = VK_NULL_HANDLE;
    }

    if (image_ != VK_NULL_HANDLE)
    {
        vkDestroyImage(context_.device(), image_, nullptr);

        image_ = VK_NULL_HANDLE;
    }

    if (memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(context_.device(), memory_, nullptr);

        memory_ = VK_NULL_HANDLE;
    }

    format_ = VK_FORMAT_UNDEFINED;
    initialized_ = false;
}

} // namespace afterlight::graphics::vulkan
