#include <afterlight/graphics/vulkan/material_texture.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace afterlight::graphics::vulkan
{

namespace
{

constexpr std::uint32_t texture_channel_count = 4U;
constexpr std::uint32_t maximum_texture_dimension = 4096U;
constexpr std::uint32_t maximum_ring_count = 32U;
constexpr VkFormat material_texture_format = VK_FORMAT_R8G8B8A8_SRGB;
constexpr std::uint64_t fnv_offset_basis = UINT64_C(14695981039346656037);
constexpr std::uint64_t fnv_prime = UINT64_C(1099511628211);

struct StagingBuffer final
{
    VkBuffer buffer{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkDeviceSize allocation_size{};
    bool coherent{};
};

[[nodiscard]] std::runtime_error vulkan_failure(std::string_view operation, VkResult result)
{
    return std::runtime_error{
        std::string{operation} + " failed with VkResult " +
            std::to_string(static_cast<int>(result)),
    };
}

void require_success(VkResult result, std::string_view operation)
{
    if (result != VK_SUCCESS)
    {
        throw vulkan_failure(operation, result);
    }
}

[[nodiscard]] std::size_t checked_texture_byte_count(std::uint32_t width, std::uint32_t height)
{
    if (width == 0U || height == 0U)
    {
        throw std::invalid_argument{"material texture extent cannot be zero"};
    }

    if (width > maximum_texture_dimension || height > maximum_texture_dimension)
    {
        throw std::length_error{"material texture extent exceeds the production limit"};
    }

    const std::size_t converted_width = static_cast<std::size_t>(width);
    const std::size_t converted_height = static_cast<std::size_t>(height);

    if (converted_height > std::numeric_limits<std::size_t>::max() / converted_width)
    {
        throw std::length_error{"material texture pixel count overflows size_t"};
    }

    const std::size_t pixel_count = converted_width * converted_height;

    if (pixel_count >
        std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(texture_channel_count))
    {
        throw std::length_error{"material texture byte count overflows size_t"};
    }

    return pixel_count * static_cast<std::size_t>(texture_channel_count);
}

[[nodiscard]] std::uint32_t integer_square_root(std::uint64_t radicand) noexcept
{
    std::uint64_t result = 0U;
    std::uint64_t bit = UINT64_C(1) << 62U;

    while (bit > radicand)
    {
        bit >>= 2U;
    }

    while (bit != 0U)
    {
        if (radicand >= result + bit)
        {
            radicand -= result + bit;
            result = (result >> 1U) + bit;
        }
        else
        {
            result >>= 1U;
        }

        bit >>= 2U;
    }

    return static_cast<std::uint32_t>(result);
}

[[nodiscard]] std::uint64_t texture_checksum(std::span<const std::byte> texels) noexcept
{
    std::uint64_t checksum = fnv_offset_basis;

    for (const std::byte value : texels)
    {
        checksum ^= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(value));
        checksum *= fnv_prime;
    }

    return checksum;
}

[[nodiscard]] std::uint64_t coordinate_square(std::int64_t coordinate) noexcept
{
    const std::uint64_t magnitude = coordinate < 0 ? static_cast<std::uint64_t>(-coordinate)
                                                   : static_cast<std::uint64_t>(coordinate);

    return magnitude * magnitude;
}

[[nodiscard]] std::uint32_t coordinate_magnitude(std::int64_t coordinate) noexcept
{
    return static_cast<std::uint32_t>(coordinate < 0 ? -coordinate : coordinate);
}

void write_texel(MaterialTextureData& texture,
                 std::uint32_t x,
                 std::uint32_t y,
                 std::array<std::uint8_t, texture_channel_count> channels)
{
    const std::size_t pixel_index =
        static_cast<std::size_t>(y) * static_cast<std::size_t>(texture.width) +
        static_cast<std::size_t>(x);

    const std::size_t byte_index = pixel_index * static_cast<std::size_t>(texture_channel_count);

    for (std::size_t component = 0; component < channels.size(); ++component)
    {
        texture.texels[byte_index + component] = static_cast<std::byte>(channels[component]);
    }
}

void destroy_staging_buffer(VkDevice device, StagingBuffer& staging) noexcept
{
    if (staging.buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, staging.buffer, nullptr);
        staging.buffer = VK_NULL_HANDLE;
    }

    if (staging.memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, staging.memory, nullptr);
        staging.memory = VK_NULL_HANDLE;
    }

    staging.allocation_size = 0U;
    staging.coherent = false;
}

[[nodiscard]] StagingBuffer create_staging_buffer(VulkanContext& context, VkDeviceSize size)
{
    if (size == 0U)
    {
        throw std::invalid_argument{"material texture staging size cannot be zero"};
    }

    StagingBuffer staging{};

    try
    {
        VkBufferCreateInfo buffer_info{};

        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        require_success(vkCreateBuffer(context.device(), &buffer_info, nullptr, &staging.buffer),
                        "vkCreateBuffer(material staging)");

        VkMemoryRequirements memory_requirements{};

        vkGetBufferMemoryRequirements(context.device(), staging.buffer, &memory_requirements);

        VkPhysicalDeviceMemoryProperties properties{};

        vkGetPhysicalDeviceMemoryProperties(context.physical_device(), &properties);

        const auto selection =
            choose_texture_memory_type(properties,
                                       {
                                           .type_bits = memory_requirements.memoryTypeBits,
                                           .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                           .preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       });

        if (!selection.has_value())
        {
            throw std::runtime_error{
                "Vulkan device exposes no host-visible material staging memory",
            };
        }

        VkMemoryAllocateInfo allocation_info{};

        allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation_info.allocationSize = memory_requirements.size;
        allocation_info.memoryTypeIndex = selection->index;

        require_success(
            vkAllocateMemory(context.device(), &allocation_info, nullptr, &staging.memory),
            "vkAllocateMemory(material staging)");

        require_success(vkBindBufferMemory(context.device(), staging.buffer, staging.memory, 0U),
                        "vkBindBufferMemory(material staging)");

        staging.allocation_size = memory_requirements.size;
        staging.coherent = selection->coherent;
    }
    catch (...)
    {
        destroy_staging_buffer(context.device(), staging);
        throw;
    }

    return staging;
}

void write_staging_texture(VkDevice device,
                           const StagingBuffer& staging,
                           std::span<const std::byte> texels)
{
    if (texels.empty() || static_cast<VkDeviceSize>(texels.size()) > staging.allocation_size)
    {
        throw std::invalid_argument{"material staging payload is invalid"};
    }

    void* mapped = nullptr;

    require_success(vkMapMemory(device, staging.memory, 0U, staging.allocation_size, 0U, &mapped),
                    "vkMapMemory(material staging)");

    std::memcpy(mapped, texels.data(), texels.size());

    if (!staging.coherent)
    {
        VkMappedMemoryRange range{};

        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = staging.memory;
        range.offset = 0U;
        range.size = VK_WHOLE_SIZE;

        const VkResult flush_result = vkFlushMappedMemoryRanges(device, 1U, &range);

        if (flush_result != VK_SUCCESS)
        {
            vkUnmapMemory(device, staging.memory);
            throw vulkan_failure("vkFlushMappedMemoryRanges(material staging)", flush_result);
        }
    }

    vkUnmapMemory(device, staging.memory);
}

void submit_texture_upload(VulkanContext& context,
                           VkBuffer staging_buffer,
                           VkImage destination,
                           VkExtent3D extent)
{
    if (context.graphics_queue() == VK_NULL_HANDLE)
    {
        throw std::runtime_error{
            "Vulkan graphics queue is unavailable for material upload",
        };
    }

    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    const auto cleanup = [&]() noexcept
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(context.device(), fence, nullptr);
            fence = VK_NULL_HANDLE;
        }

        if (command_pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(context.device(), command_pool, nullptr);
            command_pool = VK_NULL_HANDLE;
            command_buffer = VK_NULL_HANDLE;
        }
    };

    try
    {
        VkCommandPoolCreateInfo pool_info{};

        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool_info.queueFamilyIndex = context.device_info().graphics_queue_family;

        require_success(vkCreateCommandPool(context.device(), &pool_info, nullptr, &command_pool),
                        "vkCreateCommandPool(material upload)");

        VkCommandBufferAllocateInfo command_buffer_info{};

        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_info.commandPool = command_pool;
        command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_info.commandBufferCount = 1U;

        require_success(
            vkAllocateCommandBuffers(context.device(), &command_buffer_info, &command_buffer),
            "vkAllocateCommandBuffers(material upload)");

        VkCommandBufferBeginInfo begin_info{};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        require_success(vkBeginCommandBuffer(command_buffer, &begin_info),
                        "vkBeginCommandBuffer(material upload)");

        VkImageMemoryBarrier2 to_transfer{};

        to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        to_transfer.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        to_transfer.srcAccessMask = VK_ACCESS_2_NONE;
        to_transfer.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        to_transfer.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.image = destination;
        to_transfer.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0U,
            .levelCount = 1U,
            .baseArrayLayer = 0U,
            .layerCount = 1U,
        };

        VkDependencyInfo transfer_dependency{};

        transfer_dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        transfer_dependency.imageMemoryBarrierCount = 1U;
        transfer_dependency.pImageMemoryBarriers = &to_transfer;

        vkCmdPipelineBarrier2(command_buffer, &transfer_dependency);

        VkBufferImageCopy copy{};

        copy.bufferOffset = 0U;
        copy.bufferRowLength = 0U;
        copy.bufferImageHeight = 0U;
        copy.imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0U,
            .baseArrayLayer = 0U,
            .layerCount = 1U,
        };
        copy.imageOffset = {
            .x = 0,
            .y = 0,
            .z = 0,
        };
        copy.imageExtent = extent;

        vkCmdCopyBufferToImage(command_buffer,
                               staging_buffer,
                               destination,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1U,
                               &copy);

        VkImageMemoryBarrier2 to_shader_read{};

        to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        to_shader_read.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        to_shader_read.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        to_shader_read.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        to_shader_read.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_shader_read.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_shader_read.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_shader_read.image = destination;
        to_shader_read.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0U,
            .levelCount = 1U,
            .baseArrayLayer = 0U,
            .layerCount = 1U,
        };

        VkDependencyInfo shader_dependency{};

        shader_dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        shader_dependency.imageMemoryBarrierCount = 1U;
        shader_dependency.pImageMemoryBarriers = &to_shader_read;

        vkCmdPipelineBarrier2(command_buffer, &shader_dependency);

        require_success(vkEndCommandBuffer(command_buffer), "vkEndCommandBuffer(material upload)");

        VkFenceCreateInfo fence_info{};

        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        require_success(vkCreateFence(context.device(), &fence_info, nullptr, &fence),
                        "vkCreateFence(material upload)");

        VkCommandBufferSubmitInfo command_submit_info{};

        command_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_submit_info.commandBuffer = command_buffer;

        VkSubmitInfo2 submit_info{};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit_info.commandBufferInfoCount = 1U;
        submit_info.pCommandBufferInfos = &command_submit_info;

        require_success(vkQueueSubmit2(context.graphics_queue(), 1U, &submit_info, fence),
                        "vkQueueSubmit2(material upload)");

        require_success(
            vkWaitForFences(
                context.device(), 1U, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()),
            "vkWaitForFences(material upload)");
    }
    catch (...)
    {
        cleanup();
        throw;
    }

    cleanup();
}

} // namespace

std::optional<TextureMemoryType>
choose_texture_memory_type(const VkPhysicalDeviceMemoryProperties& properties,
                           TextureMemoryRequirements requirements) noexcept
{
    const std::uint32_t memory_type_count =
        std::min(properties.memoryTypeCount, std::uint32_t{VK_MAX_MEMORY_TYPES});

    const auto matches = [&](std::uint32_t index, VkMemoryPropertyFlags flags) noexcept
    {
        if (index >= 32U)
        {
            return false;
        }

        const std::uint32_t bit = std::uint32_t{1U} << index;

        return (requirements.type_bits & bit) != 0U &&
               (properties.memoryTypes[index].propertyFlags & flags) == flags;
    };

    if (requirements.preferred != 0U)
    {
        const VkMemoryPropertyFlags preferred_flags =
            requirements.required | requirements.preferred;

        for (std::uint32_t index = 0U; index < memory_type_count; ++index)
        {
            if (matches(index, preferred_flags))
            {
                const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;

                return TextureMemoryType{
                    .index = index,
                    .coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0U,
                };
            }
        }
    }

    for (std::uint32_t index = 0U; index < memory_type_count; ++index)
    {
        if (matches(index, requirements.required))
        {
            const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;

            return TextureMemoryType{
                .index = index,
                .coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0U,
            };
        }
    }

    return std::nullopt;
}

bool valid_aperture_material_parameters(const ApertureMaterialParameters& parameters) noexcept
{
    const auto finite_register = [](const std::array<float, 4>& values) noexcept
    {
        return std::all_of(values.begin(),
                           values.end(),
                           [](float value) noexcept { return std::isfinite(value); });
    };

    if (!finite_register(parameters.surface_response) ||
        !finite_register(parameters.specular_response) ||
        !finite_register(parameters.surface_accents) || !finite_register(parameters.specular_tint))
    {
        return false;
    }

    const auto& surface = parameters.surface_response;
    const auto& specular = parameters.specular_response;
    const auto& accents = parameters.surface_accents;
    const auto& tint = parameters.specular_tint;

    const bool surface_response_valid =
        surface[0] >= 0.0F && surface[0] <= 2.0F && surface[1] >= 0.0F && surface[1] <= 8.0F &&
        surface[2] >= 0.0F && surface[2] <= surface[3] && surface[3] <= 1.0F;

    const bool specular_response_valid = specular[0] >= 1.0F && specular[0] <= 256.0F &&
                                         specular[1] >= 1.0F && specular[1] <= specular[0] &&
                                         specular[2] >= 0.0F && specular[2] <= 1.0F &&
                                         specular[3] >= 0.0F && specular[3] <= specular[2];

    const bool accent_response_valid =
        accents[0] >= 0.0F && accents[0] <= 1.0F && accents[1] >= 1.0F && accents[1] <= 16.0F &&
        accents[2] >= 0.0F && accents[2] <= 1.0F && accents[3] == 0.0F;

    const bool tint_valid = tint[0] >= 0.0F && tint[0] <= 1.0F && tint[1] >= 0.0F &&
                            tint[1] <= 1.0F && tint[2] >= 0.0F && tint[2] <= 1.0F &&
                            tint[3] == 0.0F;

    return surface_response_valid && specular_response_valid && accent_response_valid && tint_valid;
}

MaterialTextureData make_aperture_material_texture(MaterialTextureSpec specification)
{
    const std::size_t byte_count =
        checked_texture_byte_count(specification.width, specification.height);

    if (specification.ring_count == 0U || specification.ring_count > maximum_ring_count)
    {
        throw std::invalid_argument{
            "material texture ring count is outside the supported range",
        };
    }

    MaterialTextureData texture{
        .width = specification.width,
        .height = specification.height,
        .texels = std::vector<std::byte>(byte_count),
        .checksum = 0U,
    };

    const std::uint64_t width_square = static_cast<std::uint64_t>(specification.width) *
                                       static_cast<std::uint64_t>(specification.width);

    const std::uint64_t height_square = static_cast<std::uint64_t>(specification.height) *
                                        static_cast<std::uint64_t>(specification.height);

    const std::uint32_t maximum_radius = integer_square_root(width_square + height_square);

    const std::uint32_t ring_spacing =
        std::max(4U, maximum_radius / (specification.ring_count + 1U));

    for (std::uint32_t row = 0U; row < specification.height; ++row)
    {
        const std::int64_t centered_y = static_cast<std::int64_t>(row) * 2 + 1 -
                                        static_cast<std::int64_t>(specification.height);

        for (std::uint32_t column = 0U; column < specification.width; ++column)
        {
            const std::int64_t centered_x = static_cast<std::int64_t>(column) * 2 + 1 -
                                            static_cast<std::int64_t>(specification.width);

            const std::uint32_t horizontal = coordinate_magnitude(centered_x);

            const std::uint32_t vertical = coordinate_magnitude(centered_y);

            const std::uint32_t radius =
                integer_square_root(coordinate_square(centered_x) + coordinate_square(centered_y));

            const std::uint32_t ring_phase = radius % ring_spacing;

            const std::uint32_t ring_distance = std::min(ring_phase, ring_spacing - ring_phase);

            const std::uint32_t diagonal_distance =
                horizontal > vertical ? horizontal - vertical : vertical - horizontal;

            const std::uint32_t spoke_distance =
                std::min(std::min(horizontal, vertical), diagonal_distance);

            const std::uint32_t panel = ((column / 8U) + (row / 8U)) & 1U;

            const std::uint32_t bounded_radius = std::min(radius, maximum_radius);

            const std::uint32_t radial_lift =
                (maximum_radius - bounded_radius) * 14U / maximum_radius;

            const std::uint32_t base = 38U + radial_lift + panel * 4U;

            std::array<std::uint8_t, texture_channel_count> channels{
                static_cast<std::uint8_t>(base + 7U),
                static_cast<std::uint8_t>(base + 10U),
                static_cast<std::uint8_t>(base + 12U),
                static_cast<std::uint8_t>(172U + panel * 7U +
                                          bounded_radius * 24U / maximum_radius),
            };

            if (ring_distance <= 1U)
            {
                channels = {
                    112U,
                    91U,
                    57U,
                    142U,
                };
            }

            if (spoke_distance <= 1U)
            {
                channels = {
                    76U,
                    92U,
                    99U,
                    154U,
                };
            }

            if (radius < maximum_radius / 8U)
            {
                channels = {
                    43U,
                    116U,
                    132U,
                    128U,
                };
            }

            if (radius > maximum_radius * 4U / 5U)
            {
                channels = {
                    27U,
                    31U,
                    34U,
                    204U,
                };
            }

            write_texel(texture, column, row, channels);
        }
    }

    texture.checksum = texture_checksum(texture.texels);

    return texture;
}

MaterialTexture::MaterialTexture(VulkanContext& context,
                                 const MaterialTextureData& data,
                                 ApertureMaterialParameters parameters)
    : context_{context}, parameters_{parameters}
{
    if (!valid_aperture_material_parameters(parameters_))
    {
        throw std::invalid_argument{
            "aperture material parameters are outside the supported domain",
        };
    }

    const std::size_t expected_size = checked_texture_byte_count(data.width, data.height);

    if (data.texels.size() != expected_size)
    {
        throw std::invalid_argument{
            "material texture payload does not match its extent",
        };
    }

    if (data.checksum != texture_checksum(data.texels))
    {
        throw std::invalid_argument{
            "material texture checksum does not match its payload",
        };
    }

    width_ = data.width;
    height_ = data.height;
    checksum_ = data.checksum;

    try
    {
        create_image_and_upload(data);
        create_image_view();
        create_sampler();
        create_parameter_buffer();
        create_descriptor_resources();
    }
    catch (...)
    {
        reset();
        throw;
    }
}

MaterialTexture::~MaterialTexture()
{
    reset();
}

VkDescriptorSetLayout MaterialTexture::descriptor_set_layout() const noexcept
{
    return descriptor_set_layout_;
}

VkDescriptorSet MaterialTexture::descriptor_set() const noexcept
{
    return descriptor_set_;
}

std::uint32_t MaterialTexture::width() const noexcept
{
    return width_;
}

std::uint32_t MaterialTexture::height() const noexcept
{
    return height_;
}

std::uint64_t MaterialTexture::checksum() const noexcept
{
    return checksum_;
}

std::uint32_t MaterialTexture::parameter_byte_size() const noexcept
{
    return static_cast<std::uint32_t>(sizeof(ApertureMaterialParameters));
}

const ApertureMaterialParameters& MaterialTexture::parameters() const noexcept
{
    return parameters_;
}

void MaterialTexture::create_image_and_upload(const MaterialTextureData& data)
{
    VkFormatProperties format_properties{};

    vkGetPhysicalDeviceFormatProperties(
        context_.physical_device(), material_texture_format, &format_properties);

    constexpr VkFormatFeatureFlags required_features =
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;

    if ((format_properties.optimalTilingFeatures & required_features) != required_features)
    {
        throw std::runtime_error{
            "Vulkan device does not support the material texture format",
        };
    }

    if (data.texels.size() > static_cast<std::size_t>(std::numeric_limits<VkDeviceSize>::max()))
    {
        throw std::length_error{
            "material texture payload exceeds VkDeviceSize",
        };
    }

    StagingBuffer staging =
        create_staging_buffer(context_, static_cast<VkDeviceSize>(data.texels.size()));

    try
    {
        VkImageCreateInfo image_info{};

        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.format = material_texture_format;
        image_info.extent = {
            .width = width_,
            .height = height_,
            .depth = 1U,
        };
        image_info.mipLevels = 1U;
        image_info.arrayLayers = 1U;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        require_success(vkCreateImage(context_.device(), &image_info, nullptr, &image_),
                        "vkCreateImage(material texture)");

        VkMemoryRequirements memory_requirements{};

        vkGetImageMemoryRequirements(context_.device(), image_, &memory_requirements);

        VkPhysicalDeviceMemoryProperties memory_properties{};

        vkGetPhysicalDeviceMemoryProperties(context_.physical_device(), &memory_properties);

        const auto selection =
            choose_texture_memory_type(memory_properties,
                                       {
                                           .type_bits = memory_requirements.memoryTypeBits,
                                           .required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                           .preferred = 0U,
                                       });

        if (!selection.has_value())
        {
            throw std::runtime_error{
                "Vulkan device exposes no device-local material texture memory",
            };
        }

        VkMemoryAllocateInfo allocation_info{};

        allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation_info.allocationSize = memory_requirements.size;
        allocation_info.memoryTypeIndex = selection->index;

        require_success(vkAllocateMemory(context_.device(), &allocation_info, nullptr, &memory_),
                        "vkAllocateMemory(material texture)");

        require_success(vkBindImageMemory(context_.device(), image_, memory_, 0U),
                        "vkBindImageMemory(material texture)");

        write_staging_texture(context_.device(), staging, data.texels);

        submit_texture_upload(context_,
                              staging.buffer,
                              image_,
                              {
                                  .width = width_,
                                  .height = height_,
                                  .depth = 1U,
                              });
    }
    catch (...)
    {
        destroy_staging_buffer(context_.device(), staging);
        throw;
    }

    destroy_staging_buffer(context_.device(), staging);
}

void MaterialTexture::create_image_view()
{
    VkImageViewCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image_;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = material_texture_format;
    create_info.components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    create_info.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0U,
        .levelCount = 1U,
        .baseArrayLayer = 0U,
        .layerCount = 1U,
    };

    require_success(vkCreateImageView(context_.device(), &create_info, nullptr, &image_view_),
                    "vkCreateImageView(material texture)");
}

void MaterialTexture::create_sampler()
{
    VkSamplerCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter = VK_FILTER_LINEAR;
    create_info.minFilter = VK_FILTER_LINEAR;
    create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.mipLodBias = 0.0F;
    create_info.anisotropyEnable = VK_FALSE;
    create_info.maxAnisotropy = 1.0F;
    create_info.compareEnable = VK_FALSE;
    create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    create_info.minLod = 0.0F;
    create_info.maxLod = 0.0F;
    create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;

    require_success(vkCreateSampler(context_.device(), &create_info, nullptr, &sampler_),
                    "vkCreateSampler(material texture)");
}

void MaterialTexture::create_parameter_buffer()
{
    const VkDeviceSize parameter_size =
        static_cast<VkDeviceSize>(sizeof(ApertureMaterialParameters));

    VkBufferCreateInfo buffer_info{};

    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = parameter_size;
    buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    require_success(vkCreateBuffer(context_.device(), &buffer_info, nullptr, &parameter_buffer_),
                    "vkCreateBuffer(material parameters)");

    VkMemoryRequirements requirements{};

    vkGetBufferMemoryRequirements(context_.device(), parameter_buffer_, &requirements);

    VkPhysicalDeviceMemoryProperties properties{};

    vkGetPhysicalDeviceMemoryProperties(context_.physical_device(), &properties);

    const auto selection =
        choose_texture_memory_type(properties,
                                   {
                                       .type_bits = requirements.memoryTypeBits,
                                       .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                       .preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   });

    if (!selection.has_value())
    {
        throw std::runtime_error{
            "Vulkan device exposes no host-visible "
            "material parameter memory",
        };
    }

    VkMemoryAllocateInfo allocation_info{};

    allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation_info.allocationSize = requirements.size;
    allocation_info.memoryTypeIndex = selection->index;

    require_success(
        vkAllocateMemory(context_.device(), &allocation_info, nullptr, &parameter_memory_),
        "vkAllocateMemory(material parameters)");

    require_success(vkBindBufferMemory(context_.device(), parameter_buffer_, parameter_memory_, 0U),
                    "vkBindBufferMemory(material parameters)");

    void* mapped = nullptr;

    require_success(
        vkMapMemory(context_.device(), parameter_memory_, 0U, requirements.size, 0U, &mapped),
        "vkMapMemory(material parameters)");

    std::memcpy(mapped, &parameters_, sizeof(parameters_));

    if (!selection->coherent)
    {
        VkMappedMemoryRange range{};

        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = parameter_memory_;
        range.offset = 0U;
        range.size = VK_WHOLE_SIZE;

        const VkResult flush_result = vkFlushMappedMemoryRanges(context_.device(), 1U, &range);

        if (flush_result != VK_SUCCESS)
        {
            vkUnmapMemory(context_.device(), parameter_memory_);

            throw vulkan_failure("vkFlushMappedMemoryRanges"
                                 "(material parameters)",
                                 flush_result);
        }
    }

    vkUnmapMemory(context_.device(), parameter_memory_);
}

void MaterialTexture::create_descriptor_resources()
{
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

    bindings[0].binding = material_image_binding;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bindings[0].descriptorCount = 1U;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = material_sampler_binding;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    bindings[1].descriptorCount = 1U;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[2].binding = material_parameter_binding;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1U;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info{};

    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();

    require_success(vkCreateDescriptorSetLayout(
                        context_.device(), &layout_info, nullptr, &descriptor_set_layout_),
                    "vkCreateDescriptorSetLayout(material)");

    std::array<VkDescriptorPoolSize, 3> pool_sizes{};

    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    pool_sizes[0].descriptorCount = 1U;

    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    pool_sizes[1].descriptorCount = 1U;

    pool_sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[2].descriptorCount = 1U;

    VkDescriptorPoolCreateInfo pool_info{};

    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1U;
    pool_info.poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();

    require_success(
        vkCreateDescriptorPool(context_.device(), &pool_info, nullptr, &descriptor_pool_),
        "vkCreateDescriptorPool(material)");

    VkDescriptorSetAllocateInfo allocate_info{};

    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_pool_;
    allocate_info.descriptorSetCount = 1U;
    allocate_info.pSetLayouts = &descriptor_set_layout_;

    require_success(vkAllocateDescriptorSets(context_.device(), &allocate_info, &descriptor_set_),
                    "vkAllocateDescriptorSets(material)");

    VkDescriptorImageInfo image_info{};

    image_info.imageView = image_view_;
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo sampler_info{};

    sampler_info.sampler = sampler_;

    VkDescriptorBufferInfo parameter_info{};

    parameter_info.buffer = parameter_buffer_;
    parameter_info.offset = 0U;
    parameter_info.range = static_cast<VkDeviceSize>(sizeof(ApertureMaterialParameters));

    std::array<VkWriteDescriptorSet, 3> writes{};

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptor_set_;
    writes[0].dstBinding = material_image_binding;
    writes[0].descriptorCount = 1U;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writes[0].pImageInfo = &image_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptor_set_;
    writes[1].dstBinding = material_sampler_binding;
    writes[1].descriptorCount = 1U;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writes[1].pImageInfo = &sampler_info;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = descriptor_set_;
    writes[2].dstBinding = material_parameter_binding;
    writes[2].descriptorCount = 1U;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[2].pBufferInfo = &parameter_info;

    vkUpdateDescriptorSets(
        context_.device(), static_cast<std::uint32_t>(writes.size()), writes.data(), 0U, nullptr);
}

void MaterialTexture::reset() noexcept
{
    descriptor_set_ = VK_NULL_HANDLE;

    if (descriptor_pool_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(context_.device(), descriptor_pool_, nullptr);

        descriptor_pool_ = VK_NULL_HANDLE;
    }

    if (descriptor_set_layout_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(context_.device(), descriptor_set_layout_, nullptr);

        descriptor_set_layout_ = VK_NULL_HANDLE;
    }

    if (parameter_buffer_ != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(context_.device(), parameter_buffer_, nullptr);

        parameter_buffer_ = VK_NULL_HANDLE;
    }

    if (parameter_memory_ != VK_NULL_HANDLE)
    {
        vkFreeMemory(context_.device(), parameter_memory_, nullptr);

        parameter_memory_ = VK_NULL_HANDLE;
    }

    if (sampler_ != VK_NULL_HANDLE)
    {
        vkDestroySampler(context_.device(), sampler_, nullptr);

        sampler_ = VK_NULL_HANDLE;
    }

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

    width_ = 0U;
    height_ = 0U;
    checksum_ = 0U;
    parameters_ = {};
}

} // namespace afterlight::graphics::vulkan
