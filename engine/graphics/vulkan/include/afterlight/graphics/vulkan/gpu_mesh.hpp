#pragma once

#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <afterlight/scene/mesh.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

class GpuMesh final
{
public:
    GpuMesh(VulkanContext& context, const scene::MeshData& mesh);

    ~GpuMesh();

    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&&) = delete;
    GpuMesh& operator=(GpuMesh&&) = delete;

    void record(VkCommandBuffer command_buffer) const;

    [[nodiscard]] std::uint32_t vertex_count() const noexcept;

    [[nodiscard]] std::uint32_t index_count() const noexcept;

private:
    struct BufferAllocation final
    {
        VkBuffer buffer{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
    };

    struct MemorySelection final
    {
        std::uint32_t index{};
        bool coherent{};
    };

    [[nodiscard]] BufferAllocation create_buffer(VkBufferUsageFlags usage,
                                                 std::span<const std::byte> bytes);

    [[nodiscard]] MemorySelection select_memory(std::uint32_t allowed_types) const;

    void destroy_buffer(BufferAllocation& allocation) noexcept;

    void reset() noexcept;

    VulkanContext& context_;

    BufferAllocation vertex_buffer_;
    BufferAllocation index_buffer_;

    std::uint32_t vertex_count_{};
    std::uint32_t index_count_{};
};

} // namespace afterlight::graphics::vulkan
