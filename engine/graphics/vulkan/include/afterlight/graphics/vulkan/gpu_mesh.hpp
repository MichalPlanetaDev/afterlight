#pragma once

#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <afterlight/scene/mesh.hpp>
#include <cstdint>
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

    void destroy_buffer(BufferAllocation& allocation) noexcept;

    void reset() noexcept;

    VulkanContext& context_;

    BufferAllocation vertex_buffer_;
    BufferAllocation index_buffer_;

    std::uint32_t vertex_count_{};
    std::uint32_t index_count_{};
};

} // namespace afterlight::graphics::vulkan
