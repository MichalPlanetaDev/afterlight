#include <afterlight/graphics/vulkan/gpu_mesh.hpp>
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

GpuMesh::GpuMesh(VulkanContext& context, const scene::MeshData& mesh) : context_{context}
{
    if (mesh.vertices.empty() || mesh.indices.empty())
    {
        throw std::invalid_argument{"GPU mesh requires vertices and indices"};
    }

    if (mesh.vertices.size() >
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
        mesh.indices.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
    {
        throw std::length_error{"GPU mesh exceeds draw-count limits"};
    }

    for (const std::uint16_t index : mesh.indices)
    {
        if (static_cast<std::size_t>(index) >= mesh.vertices.size())
        {
            throw std::invalid_argument{"GPU mesh contains an invalid index"};
        }
    }

    vertex_count_ = static_cast<std::uint32_t>(mesh.vertices.size());

    index_count_ = static_cast<std::uint32_t>(mesh.indices.size());

    try
    {
        vertex_buffer_ =
            create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          std::as_bytes(std::span{mesh.vertices.data(), mesh.vertices.size()}));

        index_buffer_ =
            create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          std::as_bytes(std::span{mesh.indices.data(), mesh.indices.size()}));
    }
    catch (...)
    {
        reset();
        throw;
    }
}

GpuMesh::~GpuMesh()
{
    reset();
}

void GpuMesh::record(VkCommandBuffer command_buffer) const
{
    if (command_buffer == VK_NULL_HANDLE || vertex_buffer_.buffer == VK_NULL_HANDLE ||
        index_buffer_.buffer == VK_NULL_HANDLE)
    {
        throw std::logic_error{"GPU mesh is not recordable"};
    }

    constexpr VkDeviceSize vertex_offset = 0;

    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer_.buffer, &vertex_offset);

    vkCmdBindIndexBuffer(command_buffer, index_buffer_.buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(command_buffer, index_count_, 1, 0, 0, 0);
}

std::uint32_t GpuMesh::vertex_count() const noexcept
{
    return vertex_count_;
}

std::uint32_t GpuMesh::index_count() const noexcept
{
    return index_count_;
}

GpuMesh::BufferAllocation GpuMesh::create_buffer(VkBufferUsageFlags usage,
                                                 std::span<const std::byte> bytes)
{
    if (bytes.empty())
    {
        throw std::invalid_argument{"GPU buffer data cannot be empty"};
    }

    BufferAllocation allocation;

    try
    {
        VkBufferCreateInfo buffer_info{};

        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

        buffer_info.size = static_cast<VkDeviceSize>(bytes.size());

        buffer_info.usage = usage;

        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        require_success(
            vkCreateBuffer(context_.device(), &buffer_info, nullptr, &allocation.buffer),
            "vkCreateBuffer");

        VkMemoryRequirements requirements{};

        vkGetBufferMemoryRequirements(context_.device(), allocation.buffer, &requirements);

        const MemorySelection memory = select_memory(requirements.memoryTypeBits);

        VkMemoryAllocateInfo allocation_info{};

        allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        allocation_info.allocationSize = requirements.size;

        allocation_info.memoryTypeIndex = memory.index;

        require_success(
            vkAllocateMemory(context_.device(), &allocation_info, nullptr, &allocation.memory),
            "vkAllocateMemory(buffer)");

        require_success(
            vkBindBufferMemory(context_.device(), allocation.buffer, allocation.memory, 0),
            "vkBindBufferMemory");

        void* mapped = nullptr;

        require_success(vkMapMemory(context_.device(),
                                    allocation.memory,
                                    0,
                                    static_cast<VkDeviceSize>(bytes.size()),
                                    0,
                                    &mapped),
                        "vkMapMemory(buffer)");

        std::memcpy(mapped, bytes.data(), bytes.size());

        if (!memory.coherent)
        {
            VkMappedMemoryRange range{};

            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

            range.memory = allocation.memory;
            range.offset = 0;
            range.size = VK_WHOLE_SIZE;

            const VkResult flush_result = vkFlushMappedMemoryRanges(context_.device(), 1, &range);

            if (flush_result != VK_SUCCESS)
            {
                vkUnmapMemory(context_.device(), allocation.memory);

                throw vulkan_failure("vkFlushMappedMemoryRanges", flush_result);
            }
        }

        vkUnmapMemory(context_.device(), allocation.memory);
    }
    catch (...)
    {
        destroy_buffer(allocation);
        throw;
    }

    return allocation;
}

GpuMesh::MemorySelection GpuMesh::select_memory(std::uint32_t allowed_types) const
{
    VkPhysicalDeviceMemoryProperties properties{};

    vkGetPhysicalDeviceMemoryProperties(context_.physical_device(), &properties);

    constexpr std::uint32_t invalid_index = std::numeric_limits<std::uint32_t>::max();

    std::uint32_t visible_fallback = invalid_index;

    for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
    {
        const bool allowed = (allowed_types & (std::uint32_t{1} << index)) != 0;

        const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;

        const bool host_visible = (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

        const bool host_coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;

        if (!allowed || !host_visible)
        {
            continue;
        }

        if (host_coherent)
        {
            return {
                .index = index,
                .coherent = true,
            };
        }

        if (visible_fallback == invalid_index)
        {
            visible_fallback = index;
        }
    }

    if (visible_fallback != invalid_index)
    {
        return {
            .index = visible_fallback,
            .coherent = false,
        };
    }

    throw std::runtime_error{"Vulkan device exposes no host-visible buffer memory"};
}

void GpuMesh::destroy_buffer(BufferAllocation& allocation) noexcept
{
    if (allocation.buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(context_.device(), allocation.buffer, nullptr);

        allocation.buffer = VK_NULL_HANDLE;
    }

    if (allocation.memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(context_.device(), allocation.memory, nullptr);

        allocation.memory = VK_NULL_HANDLE;
    }
}

void GpuMesh::reset() noexcept
{
    destroy_buffer(index_buffer_);
    destroy_buffer(vertex_buffer_);

    vertex_count_ = 0;
    index_count_ = 0;
}

} // namespace afterlight::graphics::vulkan
