#include <afterlight/graphics/vulkan/swapchain.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
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

[[nodiscard]] VkCompositeAlphaFlagBitsKHR
choose_composite_alpha(VkCompositeAlphaFlagsKHR supported) noexcept
{
    constexpr std::array<VkCompositeAlphaFlagBitsKHR, 4> preferences{
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (const VkCompositeAlphaFlagBitsKHR mode : preferences)
    {
        if ((supported & static_cast<VkCompositeAlphaFlagsKHR>(mode)) != 0)
        {
            return mode;
        }
    }

    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

[[nodiscard]] std::vector<VkSurfaceFormatKHR>
query_surface_formats(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    std::uint32_t count = 0;

    require_success(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr),
                    "vkGetPhysicalDeviceSurfaceFormatsKHR(count)");

    if (count == 0)
    {
        throw std::runtime_error{"Vulkan surface exposes no formats"};
    }

    std::vector<VkSurfaceFormatKHR> formats(count);

    require_success(
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, formats.data()),
        "vkGetPhysicalDeviceSurfaceFormatsKHR(data)");

    return formats;
}

[[nodiscard]] std::vector<VkPresentModeKHR> query_present_modes(VkPhysicalDevice physical_device,
                                                                VkSurfaceKHR surface)
{
    std::uint32_t count = 0;

    require_success(
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr),
        "vkGetPhysicalDeviceSurfacePresentModesKHR(count)");

    if (count == 0)
    {
        throw std::runtime_error{"Vulkan surface exposes no presentation modes"};
    }

    std::vector<VkPresentModeKHR> modes(count);

    require_success(
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, modes.data()),
        "vkGetPhysicalDeviceSurfacePresentModesKHR(data)");

    return modes;
}

[[nodiscard]] VkImageMemoryBarrier2 begin_rendering_barrier(VkImage image) noexcept
{
    VkImageMemoryBarrier2 barrier{};

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;

    barrier.srcAccessMask = VK_ACCESS_2_NONE;

    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

[[nodiscard]] VkImageMemoryBarrier2 present_barrier(VkImage image) noexcept
{
    VkImageMemoryBarrier2 barrier{};

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;

    barrier.dstAccessMask = VK_ACCESS_2_NONE;

    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

void apply_image_barrier(VkCommandBuffer command_buffer, const VkImageMemoryBarrier2& image_barrier)
{
    VkDependencyInfo dependency{};

    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &image_barrier;

    vkCmdPipelineBarrier2(command_buffer, &dependency);
}

} // namespace

VkSurfaceFormatKHR choose_surface_format(std::span<const VkSurfaceFormatKHR> formats)
{
    if (formats.empty())
    {
        throw std::invalid_argument{"surface format list cannot be empty"};
    }

    if (formats.size() == 1 && formats.front().format == VK_FORMAT_UNDEFINED)
    {
        return {
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        };
    }

    constexpr std::array<VkFormat, 2> preferred_formats{
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_SRGB,
    };

    for (const VkFormat preferred : preferred_formats)
    {
        const auto match =
            std::find_if(formats.begin(),
                         formats.end(),
                         [preferred](const VkSurfaceFormatKHR& candidate)
                         {
                             return candidate.format == preferred &&
                                    candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                         });

        if (match != formats.end())
        {
            return *match;
        }
    }

    return formats.front();
}

VkPresentModeKHR choose_present_mode(std::span<const VkPresentModeKHR> modes)
{
    if (modes.empty())
    {
        throw std::invalid_argument{"presentation mode list cannot be empty"};
    }

    const auto mailbox = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR);

    if (mailbox != modes.end())
    {
        return *mailbox;
    }

    const auto fifo = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_FIFO_KHR);

    if (fifo != modes.end())
    {
        return *fifo;
    }

    return modes.front();
}

VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilities,
                                   platform::WindowSize pixel_size) noexcept
{
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    const auto requested_width = static_cast<std::uint32_t>(std::max(pixel_size.width, 0));

    const auto requested_height = static_cast<std::uint32_t>(std::max(pixel_size.height, 0));

    return {
        .width = std::clamp(
            requested_width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = std::clamp(requested_height,
                             capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height),
    };
}

std::uint32_t choose_swapchain_image_count(const VkSurfaceCapabilitiesKHR& capabilities) noexcept
{
    std::uint32_t count = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount != 0 && count > capabilities.maxImageCount)
    {
        count = capabilities.maxImageCount;
    }

    return count;
}

SwapchainRenderer::SwapchainRenderer(VulkanContext& context, const platform::Window& window)
    : context_{context}
{
    try
    {
        create_command_resources();
        create_frame_resources();

        if (!recreate_swapchain(window))
        {
            throw std::runtime_error{"initial Vulkan swapchain extent is zero"};
        }
    }
    catch (...)
    {
        destroy_swapchain();
        destroy_frame_resources();
        destroy_command_resources();
        throw;
    }
}

SwapchainRenderer::~SwapchainRenderer()
{
    if (context_.device() != VK_NULL_HANDLE)
    {
        static_cast<void>(vkDeviceWaitIdle(context_.device()));
    }

    destroy_swapchain();
    destroy_frame_resources();
    destroy_command_resources();
}

bool SwapchainRenderer::render_frame(const platform::Window& window)
{
    if (resize_requested_)
    {
        if (!recreate_swapchain(window))
        {
            return false;
        }
    }

    if (swapchain_ == VK_NULL_HANDLE)
    {
        return recreate_swapchain(window);
    }

    FrameResources& frame = frames_[current_frame_];

    require_success(vkWaitForFences(context_.device(),
                                    1,
                                    &frame.render_fence,
                                    VK_TRUE,
                                    std::numeric_limits<std::uint64_t>::max()),
                    "vkWaitForFences(frame)");

    std::uint32_t image_index = 0;

    const VkResult acquire_result = vkAcquireNextImageKHR(context_.device(),
                                                          swapchain_,
                                                          std::numeric_limits<std::uint64_t>::max(),
                                                          frame.image_available,
                                                          VK_NULL_HANDLE,
                                                          &image_index);

    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        static_cast<void>(recreate_swapchain(window));

        return false;
    }

    if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
    {
        throw vulkan_failure("vkAcquireNextImageKHR", acquire_result);
    }

    wait_for_image(image_index, frame.render_fence);

    require_success(vkResetFences(context_.device(), 1, &frame.render_fence), "vkResetFences");

    require_success(vkResetCommandBuffer(frame.command_buffer, 0), "vkResetCommandBuffer");

    record_commands(frame.command_buffer, image_index);

    submit_frame(frame, image_index);

    const VkResult present_result = present_frame(image_index);

    current_frame_ = (current_frame_ + 1) % frames_in_flight;

    const bool presentation_changed = present_result == VK_ERROR_OUT_OF_DATE_KHR ||
                                      present_result == VK_SUBOPTIMAL_KHR ||
                                      acquire_result == VK_SUBOPTIMAL_KHR || resize_requested_;

    if (presentation_changed)
    {
        static_cast<void>(recreate_swapchain(window));
    }
    else if (present_result != VK_SUCCESS)
    {
        throw vulkan_failure("vkQueuePresentKHR", present_result);
    }

    return true;
}

void SwapchainRenderer::request_resize() noexcept
{
    resize_requested_ = true;
}

const SwapchainInfo& SwapchainRenderer::info() const noexcept
{
    return info_;
}

void SwapchainRenderer::create_command_resources()
{
    VkCommandPoolCreateInfo pool_info{};

    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    pool_info.queueFamilyIndex = context_.device_info().graphics_queue_family;

    require_success(vkCreateCommandPool(context_.device(), &pool_info, nullptr, &command_pool_),
                    "vkCreateCommandPool");

    std::array<VkCommandBuffer, frames_in_flight> command_buffers{};

    VkCommandBufferAllocateInfo allocate_info{};

    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

    allocate_info.commandPool = command_pool_;

    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    allocate_info.commandBufferCount = static_cast<std::uint32_t>(command_buffers.size());

    require_success(
        vkAllocateCommandBuffers(context_.device(), &allocate_info, command_buffers.data()),
        "vkAllocateCommandBuffers");

    for (std::size_t index = 0; index < frames_.size(); ++index)
    {
        frames_[index].command_buffer = command_buffers[index];
    }
}

void SwapchainRenderer::create_frame_resources()
{
    VkSemaphoreCreateInfo semaphore_info{};

    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (FrameResources& frame : frames_)
    {
        require_success(
            vkCreateSemaphore(context_.device(), &semaphore_info, nullptr, &frame.image_available),
            "vkCreateSemaphore(image available)");

        require_success(vkCreateFence(context_.device(), &fence_info, nullptr, &frame.render_fence),
                        "vkCreateFence");
    }
}

bool SwapchainRenderer::recreate_swapchain(const platform::Window& window)
{
    const platform::WindowSize pixel_size = window.pixel_size();

    if (pixel_size.width <= 0 || pixel_size.height <= 0)
    {
        resize_requested_ = true;
        return false;
    }

    require_success(vkDeviceWaitIdle(context_.device()), "vkDeviceWaitIdle(swapchain recreation)");

    destroy_swapchain();

    const bool created = create_swapchain(window);

    resize_requested_ = !created;

    return created;
}

bool SwapchainRenderer::create_swapchain(const platform::Window& window)
{
    VkSurfaceCapabilitiesKHR capabilities{};

    require_success(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                        context_.physical_device(), context_.surface(), &capabilities),
                    "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0)
    {
        throw std::runtime_error{"Vulkan surface images cannot be color attachments"};
    }

    const platform::WindowSize pixel_size = window.pixel_size();

    const VkExtent2D extent = choose_swapchain_extent(capabilities, pixel_size);

    if (extent.width == 0 || extent.height == 0)
    {
        return false;
    }

    const std::vector<VkSurfaceFormatKHR> formats =
        query_surface_formats(context_.physical_device(), context_.surface());

    const std::vector<VkPresentModeKHR> modes =
        query_present_modes(context_.physical_device(), context_.surface());

    const VkSurfaceFormatKHR surface_format = choose_surface_format(formats);

    const VkPresentModeKHR present_mode = choose_present_mode(modes);

    const std::uint32_t image_count = choose_swapchain_image_count(capabilities);

    const std::array<std::uint32_t, 2> queue_families{
        context_.device_info().graphics_queue_family,
        context_.device_info().present_queue_family,
    };

    VkSwapchainCreateInfoKHR create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    create_info.surface = context_.surface();
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;

    create_info.imageColorSpace = surface_format.colorSpace;

    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;

    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (queue_families[0] != queue_families[1])
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

        create_info.queueFamilyIndexCount = static_cast<std::uint32_t>(queue_families.size());

        create_info.pQueueFamilyIndices = queue_families.data();
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    create_info.preTransform = capabilities.currentTransform;

    create_info.compositeAlpha = choose_composite_alpha(capabilities.supportedCompositeAlpha);

    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    require_success(vkCreateSwapchainKHR(context_.device(), &create_info, nullptr, &swapchain_),
                    "vkCreateSwapchainKHR");

    std::uint32_t actual_image_count = 0;

    require_success(
        vkGetSwapchainImagesKHR(context_.device(), swapchain_, &actual_image_count, nullptr),
        "vkGetSwapchainImagesKHR(count)");

    if (actual_image_count == 0)
    {
        throw std::runtime_error{"Vulkan swapchain contains no images"};
    }

    images_.resize(actual_image_count);

    require_success(
        vkGetSwapchainImagesKHR(context_.device(), swapchain_, &actual_image_count, images_.data()),
        "vkGetSwapchainImagesKHR(data)");

    image_views_.resize(images_.size());

    image_fences_.assign(images_.size(), VK_NULL_HANDLE);

    info_ = {
        .width = extent.width,
        .height = extent.height,
        .image_count = actual_image_count,
        .format = surface_format.format,
        .present_mode = present_mode,
    };

    create_image_views();
    create_render_finished_semaphores();

    return true;
}

void SwapchainRenderer::create_image_views()
{
    for (std::size_t index = 0; index < images_.size(); ++index)
    {
        VkImageViewCreateInfo create_info{};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        create_info.image = images_[index];

        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        create_info.format = info_.format;

        create_info.components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        };

        create_info.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        require_success(
            vkCreateImageView(context_.device(), &create_info, nullptr, &image_views_[index]),
            "vkCreateImageView");
    }
}

void SwapchainRenderer::create_render_finished_semaphores()
{
    render_finished_.assign(images_.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (VkSemaphore& semaphore : render_finished_)
    {
        require_success(vkCreateSemaphore(context_.device(), &create_info, nullptr, &semaphore),
                        "vkCreateSemaphore(render finished)");
    }
}

void SwapchainRenderer::destroy_swapchain() noexcept
{
    for (const VkSemaphore semaphore : render_finished_)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context_.device(), semaphore, nullptr);
        }
    }

    render_finished_.clear();

    for (const VkImageView image_view : image_views_)
    {
        if (image_view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(context_.device(), image_view, nullptr);
        }
    }

    image_views_.clear();
    images_.clear();
    image_fences_.clear();

    if (swapchain_ != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(context_.device(), swapchain_, nullptr);

        swapchain_ = VK_NULL_HANDLE;
    }

    info_ = {};
}

void SwapchainRenderer::destroy_frame_resources() noexcept
{
    for (FrameResources& frame : frames_)
    {
        if (frame.render_fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(context_.device(), frame.render_fence, nullptr);

            frame.render_fence = VK_NULL_HANDLE;
        }

        if (frame.image_available != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(context_.device(), frame.image_available, nullptr);

            frame.image_available = VK_NULL_HANDLE;
        }
    }
}

void SwapchainRenderer::destroy_command_resources() noexcept
{
    if (command_pool_ != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(context_.device(), command_pool_, nullptr);

        command_pool_ = VK_NULL_HANDLE;
    }

    for (FrameResources& frame : frames_)
    {
        frame.command_buffer = VK_NULL_HANDLE;
    }
}

void SwapchainRenderer::wait_for_image(std::uint32_t image_index, VkFence frame_fence)
{
    if (image_index >= image_fences_.size())
    {
        throw std::runtime_error{"acquired swapchain image index is invalid"};
    }

    const VkFence previous_fence = image_fences_[image_index];

    if (previous_fence != VK_NULL_HANDLE)
    {
        require_success(vkWaitForFences(context_.device(),
                                        1,
                                        &previous_fence,
                                        VK_TRUE,
                                        std::numeric_limits<std::uint64_t>::max()),
                        "vkWaitForFences(image)");
    }

    image_fences_[image_index] = frame_fence;
}

void SwapchainRenderer::record_commands(VkCommandBuffer command_buffer, std::uint32_t image_index)
{
    VkCommandBufferBeginInfo begin_info{};

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    require_success(vkBeginCommandBuffer(command_buffer, &begin_info), "vkBeginCommandBuffer");

    const VkImageMemoryBarrier2 to_render = begin_rendering_barrier(images_[image_index]);

    apply_image_barrier(command_buffer, to_render);

    VkRenderingAttachmentInfo color_attachment{};

    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    color_attachment.imageView = image_views_[image_index];

    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    color_attachment.clearValue.color.float32[0] = 0.004F;

    color_attachment.clearValue.color.float32[1] = 0.008F;

    color_attachment.clearValue.color.float32[2] = 0.018F;

    color_attachment.clearValue.color.float32[3] = 1.0F;

    VkRenderingInfo rendering_info{};

    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

    rendering_info.renderArea = {
        .offset =
            {
                .x = 0,
                .y = 0,
            },
        .extent =
            {
                .width = info_.width,
                .height = info_.height,
            },
    };

    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;

    rendering_info.pColorAttachments = &color_attachment;

    vkCmdBeginRendering(command_buffer, &rendering_info);

    vkCmdEndRendering(command_buffer);

    const VkImageMemoryBarrier2 to_present = present_barrier(images_[image_index]);

    apply_image_barrier(command_buffer, to_present);

    require_success(vkEndCommandBuffer(command_buffer), "vkEndCommandBuffer");
}

void SwapchainRenderer::submit_frame(const FrameResources& frame, std::uint32_t image_index)
{
    if (image_index >= render_finished_.size())
    {
        throw std::runtime_error{"render-completion semaphore index is invalid"};
    }

    VkSemaphoreSubmitInfo wait_info{};

    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

    wait_info.semaphore = frame.image_available;

    wait_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkCommandBufferSubmitInfo command_info{};

    command_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

    command_info.commandBuffer = frame.command_buffer;

    VkSemaphoreSubmitInfo signal_info{};

    signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;

    signal_info.semaphore = render_finished_[image_index];

    signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submit_info{};

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

    submit_info.waitSemaphoreInfoCount = 1;
    submit_info.pWaitSemaphoreInfos = &wait_info;

    submit_info.commandBufferInfoCount = 1;
    submit_info.pCommandBufferInfos = &command_info;

    submit_info.signalSemaphoreInfoCount = 1;
    submit_info.pSignalSemaphoreInfos = &signal_info;

    require_success(vkQueueSubmit2(context_.graphics_queue(), 1, &submit_info, frame.render_fence),
                    "vkQueueSubmit2");
}

VkResult SwapchainRenderer::present_frame(std::uint32_t image_index)
{
    if (image_index >= render_finished_.size())
    {
        throw std::runtime_error{"presentation semaphore index is invalid"};
    }

    const VkSemaphore wait_semaphore = render_finished_[image_index];

    VkPresentInfoKHR present_info{};

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;

    present_info.pWaitSemaphores = &wait_semaphore;

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_;
    present_info.pImageIndices = &image_index;

    return vkQueuePresentKHR(context_.present_queue(), &present_info);
}

} // namespace afterlight::graphics::vulkan
