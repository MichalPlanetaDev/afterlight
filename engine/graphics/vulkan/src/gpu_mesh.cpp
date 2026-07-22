#include <afterlight/graphics/vulkan/gpu_mesh.hpp>
#include <afterlight/graphics/vulkan/mesh_upload.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>

namespace afterlight::graphics::vulkan
{

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

    const DeviceLocalMeshBuffers uploaded =
        upload_device_local_mesh(context_,
                                 {
                                     .vertices = std::as_bytes(std::span{
                                         mesh.vertices.data(),
                                         mesh.vertices.size(),
                                     }),
                                     .indices = std::as_bytes(std::span{
                                         mesh.indices.data(),
                                         mesh.indices.size(),
                                     }),
                                 });

    vertex_buffer_ = {
        .buffer = uploaded.vertex_buffer,
        .memory = uploaded.vertex_memory,
    };

    index_buffer_ = {
        .buffer = uploaded.index_buffer,
        .memory = uploaded.index_memory,
    };

    vertex_count_ = static_cast<std::uint32_t>(mesh.vertices.size());
    index_count_ = static_cast<std::uint32_t>(mesh.indices.size());
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
