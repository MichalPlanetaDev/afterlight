#pragma once

#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <afterlight/scene/camera.hpp>
#include <cstdint>
#include <vector>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

struct UniformMemorySelection final
{
    std::uint32_t index{};
    bool coherent{};
};

[[nodiscard]] UniformMemorySelection
choose_uniform_memory_type(const VkPhysicalDeviceMemoryProperties& properties,
                           std::uint32_t allowed_types);

class SceneUniforms final
{
public:
    SceneUniforms(VulkanContext& context, std::uint32_t frame_count);

    ~SceneUniforms();

    SceneUniforms(const SceneUniforms&) = delete;
    SceneUniforms& operator=(const SceneUniforms&) = delete;
    SceneUniforms(SceneUniforms&&) = delete;
    SceneUniforms& operator=(SceneUniforms&&) = delete;

    void update(std::uint32_t frame_index, const scene::SceneFrameData& frame_data);

    [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const noexcept;

    [[nodiscard]] VkDescriptorSet descriptor_set(std::uint32_t frame_index) const;

    [[nodiscard]] std::uint32_t frame_count() const noexcept;

private:
    struct FrameAllocation final
    {
        VkBuffer buffer{VK_NULL_HANDLE};
        VkDeviceMemory memory{VK_NULL_HANDLE};
        void* mapped{};
        bool coherent{};
    };

    void create_descriptor_set_layout();

    [[nodiscard]] FrameAllocation create_frame_allocation();

    void create_descriptor_pool();
    void allocate_descriptor_sets();
    void write_descriptors();

    void destroy_frame_allocation(FrameAllocation& allocation) noexcept;

    void reset() noexcept;

    VulkanContext& context_;
    std::uint32_t frame_count_{};

    VkDescriptorSetLayout descriptor_set_layout_{VK_NULL_HANDLE};

    VkDescriptorPool descriptor_pool_{VK_NULL_HANDLE};

    std::vector<FrameAllocation> frames_;

    std::vector<VkDescriptorSet> descriptor_sets_;
};

} // namespace afterlight::graphics::vulkan
