#pragma once

#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

struct MeshMemoryRequirements final
{
    std::uint32_t type_bits{};
    VkMemoryPropertyFlags required{};
    VkMemoryPropertyFlags preferred{};
};

struct MeshMemoryType final
{
    std::uint32_t index{};
    bool coherent{};
};

struct MeshUploadPayload final
{
    std::span<const std::byte> vertices{};
    std::span<const std::byte> indices{};
};

struct DeviceLocalMeshBuffers final
{
    VkBuffer vertex_buffer{VK_NULL_HANDLE};
    VkDeviceMemory vertex_memory{VK_NULL_HANDLE};
    VkBuffer index_buffer{VK_NULL_HANDLE};
    VkDeviceMemory index_memory{VK_NULL_HANDLE};
};

[[nodiscard]] std::optional<MeshMemoryType>
choose_mesh_memory_type(const VkPhysicalDeviceMemoryProperties& properties,
                        MeshMemoryRequirements requirements) noexcept;

[[nodiscard]] DeviceLocalMeshBuffers upload_device_local_mesh(VulkanContext& context,
                                                              const MeshUploadPayload& payload);

} // namespace afterlight::graphics::vulkan
