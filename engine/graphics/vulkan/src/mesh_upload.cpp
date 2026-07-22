#include <afterlight/graphics/vulkan/mesh_upload.hpp>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace afterlight::graphics::vulkan
{

namespace
{

struct BufferAllocation final
{
    VkBuffer buffer{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkDeviceSize allocation_size{};
    bool coherent{};
};

struct BufferRequest final
{
    VkDeviceSize size{};
    VkBufferUsageFlags usage{};
    VkMemoryPropertyFlags required{};
    VkMemoryPropertyFlags preferred{};
};

struct MeshCopyBuffers final
{
    VkBuffer vertex_staging{VK_NULL_HANDLE};
    VkBuffer index_staging{VK_NULL_HANDLE};
    VkBuffer vertex_destination{VK_NULL_HANDLE};
    VkBuffer index_destination{VK_NULL_HANDLE};
};

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

void destroy_allocation(VkDevice device, BufferAllocation& allocation) noexcept
{
    if (allocation.buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, allocation.buffer, nullptr);
        allocation.buffer = VK_NULL_HANDLE;
    }

    if (allocation.memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, allocation.memory, nullptr);
        allocation.memory = VK_NULL_HANDLE;
    }

    allocation.allocation_size = 0;
    allocation.coherent = false;
}

[[nodiscard]] BufferAllocation create_buffer(VulkanContext& context, const BufferRequest& request)
{
    if (request.size == 0)
    {
        throw std::invalid_argument{"Vulkan mesh buffer size cannot be zero"};
    }

    BufferAllocation allocation{};

    try
    {
        VkBufferCreateInfo buffer_info{};

        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = request.size;
        buffer_info.usage = request.usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        require_success(vkCreateBuffer(context.device(), &buffer_info, nullptr, &allocation.buffer),
                        "vkCreateBuffer(mesh upload)");

        VkMemoryRequirements memory_requirements{};

        vkGetBufferMemoryRequirements(context.device(), allocation.buffer, &memory_requirements);

        VkPhysicalDeviceMemoryProperties properties{};

        vkGetPhysicalDeviceMemoryProperties(context.physical_device(), &properties);

        const auto selection =
            choose_mesh_memory_type(properties,
                                    {
                                        .type_bits = memory_requirements.memoryTypeBits,
                                        .required = request.required,
                                        .preferred = request.preferred,
                                    });

        if (!selection.has_value())
        {
            throw std::runtime_error{"Vulkan device exposes no compatible mesh memory type"};
        }

        VkMemoryAllocateInfo allocation_info{};

        allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation_info.allocationSize = memory_requirements.size;
        allocation_info.memoryTypeIndex = selection->index;

        require_success(
            vkAllocateMemory(context.device(), &allocation_info, nullptr, &allocation.memory),
            "vkAllocateMemory(mesh upload)");

        require_success(
            vkBindBufferMemory(context.device(), allocation.buffer, allocation.memory, 0),
            "vkBindBufferMemory(mesh upload)");

        allocation.allocation_size = memory_requirements.size;
        allocation.coherent = selection->coherent;
    }
    catch (...)
    {
        destroy_allocation(context.device(), allocation);
        throw;
    }

    return allocation;
}

void write_staging_data(VkDevice device,
                        const BufferAllocation& allocation,
                        std::span<const std::byte> data)
{
    if (data.empty() || static_cast<VkDeviceSize>(data.size()) > allocation.allocation_size)
    {
        throw std::invalid_argument{"Vulkan mesh staging payload is invalid"};
    }

    void* mapped = nullptr;

    require_success(
        vkMapMemory(device, allocation.memory, 0, allocation.allocation_size, 0, &mapped),
        "vkMapMemory(mesh staging)");

    std::memcpy(mapped, data.data(), data.size());

    if (!allocation.coherent)
    {
        VkMappedMemoryRange range{};

        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = allocation.memory;
        range.offset = 0;
        range.size = VK_WHOLE_SIZE;

        const VkResult flush_result = vkFlushMappedMemoryRanges(device, 1, &range);

        if (flush_result != VK_SUCCESS)
        {
            vkUnmapMemory(device, allocation.memory);
            throw vulkan_failure("vkFlushMappedMemoryRanges(mesh staging)", flush_result);
        }
    }

    vkUnmapMemory(device, allocation.memory);
}

void copy_mesh_buffers(VulkanContext& context,
                       const MeshCopyBuffers& buffers,
                       const MeshUploadPayload& payload)
{
    if (context.graphics_queue() == VK_NULL_HANDLE)
    {
        throw std::runtime_error{"Vulkan graphics queue is unavailable for mesh upload"};
    }

    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;

    try
    {
        VkCommandPoolCreateInfo pool_info{};

        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool_info.queueFamilyIndex = context.device_info().graphics_queue_family;

        require_success(vkCreateCommandPool(context.device(), &pool_info, nullptr, &command_pool),
                        "vkCreateCommandPool(mesh upload)");

        VkCommandBufferAllocateInfo command_buffer_info{};

        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_info.commandPool = command_pool;
        command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_info.commandBufferCount = 1;

        require_success(
            vkAllocateCommandBuffers(context.device(), &command_buffer_info, &command_buffer),
            "vkAllocateCommandBuffers(mesh upload)");

        VkCommandBufferBeginInfo begin_info{};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        require_success(vkBeginCommandBuffer(command_buffer, &begin_info),
                        "vkBeginCommandBuffer(mesh upload)");

        VkBufferCopy vertex_copy{};

        vertex_copy.size = static_cast<VkDeviceSize>(payload.vertices.size());

        vkCmdCopyBuffer(
            command_buffer, buffers.vertex_staging, buffers.vertex_destination, 1, &vertex_copy);

        VkBufferCopy index_copy{};

        index_copy.size = static_cast<VkDeviceSize>(payload.indices.size());

        vkCmdCopyBuffer(
            command_buffer, buffers.index_staging, buffers.index_destination, 1, &index_copy);

        std::array<VkBufferMemoryBarrier2, 2> barriers{};

        barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
        barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].buffer = buffers.vertex_destination;
        barriers[0].offset = 0;
        barriers[0].size = VK_WHOLE_SIZE;

        barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barriers[1].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        barriers[1].dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT;
        barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barriers[1].buffer = buffers.index_destination;
        barriers[1].offset = 0;
        barriers[1].size = VK_WHOLE_SIZE;

        VkDependencyInfo dependency_info{};

        dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency_info.bufferMemoryBarrierCount = static_cast<std::uint32_t>(barriers.size());
        dependency_info.pBufferMemoryBarriers = barriers.data();

        vkCmdPipelineBarrier2(command_buffer, &dependency_info);

        require_success(vkEndCommandBuffer(command_buffer), "vkEndCommandBuffer(mesh upload)");

        VkFenceCreateInfo fence_info{};

        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        require_success(vkCreateFence(context.device(), &fence_info, nullptr, &fence),
                        "vkCreateFence(mesh upload)");

        VkCommandBufferSubmitInfo command_submit_info{};

        command_submit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_submit_info.commandBuffer = command_buffer;

        VkSubmitInfo2 submit_info{};

        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submit_info.commandBufferInfoCount = 1;
        submit_info.pCommandBufferInfos = &command_submit_info;

        require_success(vkQueueSubmit2(context.graphics_queue(), 1, &submit_info, fence),
                        "vkQueueSubmit2(mesh upload)");

        require_success(
            vkWaitForFences(
                context.device(), 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()),
            "vkWaitForFences(mesh upload)");
    }
    catch (...)
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(context.device(), fence, nullptr);
        }

        if (command_pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(context.device(), command_pool, nullptr);
        }

        throw;
    }

    vkDestroyFence(context.device(), fence, nullptr);
    vkDestroyCommandPool(context.device(), command_pool, nullptr);
}

} // namespace

std::optional<MeshMemoryType>
choose_mesh_memory_type(const VkPhysicalDeviceMemoryProperties& properties,
                        MeshMemoryRequirements requirements) noexcept
{
    const auto matches = [&](std::uint32_t index, VkMemoryPropertyFlags flags) noexcept
    {
        if (index >= 32)
        {
            return false;
        }

        const std::uint32_t bit = std::uint32_t{1} << index;

        return (requirements.type_bits & bit) != 0 &&
               (properties.memoryTypes[index].propertyFlags & flags) == flags;
    };

    if (requirements.preferred != 0)
    {
        const VkMemoryPropertyFlags preferred = requirements.required | requirements.preferred;

        for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
        {
            if (matches(index, preferred))
            {
                const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;

                return MeshMemoryType{
                    .index = index,
                    .coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0,
                };
            }
        }
    }

    for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
    {
        if (matches(index, requirements.required))
        {
            const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;

            return MeshMemoryType{
                .index = index,
                .coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0,
            };
        }
    }

    return std::nullopt;
}

DeviceLocalMeshBuffers upload_device_local_mesh(VulkanContext& context,
                                                const MeshUploadPayload& payload)
{
    if (payload.vertices.empty() || payload.indices.empty())
    {
        throw std::invalid_argument{"Mesh upload requires vertices and indices"};
    }

    BufferAllocation vertex_staging{};
    BufferAllocation index_staging{};
    BufferAllocation vertex_destination{};
    BufferAllocation index_destination{};

    try
    {
        vertex_staging =
            create_buffer(context,
                          {
                              .size = static_cast<VkDeviceSize>(payload.vertices.size()),
                              .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                              .preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          });

        index_staging = create_buffer(context,
                                      {
                                          .size = static_cast<VkDeviceSize>(payload.indices.size()),
                                          .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                          .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                          .preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      });

        vertex_destination = create_buffer(
            context,
            {
                .size = static_cast<VkDeviceSize>(payload.vertices.size()),
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                .required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .preferred = 0,
            });

        index_destination = create_buffer(
            context,
            {
                .size = static_cast<VkDeviceSize>(payload.indices.size()),
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                .required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .preferred = 0,
            });

        write_staging_data(context.device(), vertex_staging, payload.vertices);
        write_staging_data(context.device(), index_staging, payload.indices);

        copy_mesh_buffers(context,
                          {
                              .vertex_staging = vertex_staging.buffer,
                              .index_staging = index_staging.buffer,
                              .vertex_destination = vertex_destination.buffer,
                              .index_destination = index_destination.buffer,
                          },
                          payload);

        DeviceLocalMeshBuffers result{
            .vertex_buffer = vertex_destination.buffer,
            .vertex_memory = vertex_destination.memory,
            .index_buffer = index_destination.buffer,
            .index_memory = index_destination.memory,
        };

        vertex_destination.buffer = VK_NULL_HANDLE;
        vertex_destination.memory = VK_NULL_HANDLE;
        index_destination.buffer = VK_NULL_HANDLE;
        index_destination.memory = VK_NULL_HANDLE;

        destroy_allocation(context.device(), vertex_staging);
        destroy_allocation(context.device(), index_staging);

        return result;
    }
    catch (...)
    {
        destroy_allocation(context.device(), index_destination);
        destroy_allocation(context.device(), vertex_destination);
        destroy_allocation(context.device(), index_staging);
        destroy_allocation(context.device(), vertex_staging);
        throw;
    }
}

} // namespace afterlight::graphics::vulkan
