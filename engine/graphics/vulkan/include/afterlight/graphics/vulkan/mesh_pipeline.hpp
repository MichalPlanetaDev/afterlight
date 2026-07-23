#pragma once

#include <filesystem>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

class GpuMesh;

struct MeshPipelineFormats final
{
    VkFormat color{VK_FORMAT_UNDEFINED};
    VkFormat depth{VK_FORMAT_UNDEFINED};
};

class MeshPipeline final
{
public:
    MeshPipeline(VkDevice device,
                 MeshPipelineFormats formats,
                 VkDescriptorSetLayout scene_set_layout,
                 VkDescriptorSetLayout material_set_layout,
                 const std::filesystem::path& shader_directory);

    ~MeshPipeline();

    MeshPipeline(const MeshPipeline&) = delete;
    MeshPipeline& operator=(const MeshPipeline&) = delete;
    MeshPipeline(MeshPipeline&&) = delete;
    MeshPipeline& operator=(MeshPipeline&&) = delete;

    void record(VkCommandBuffer command_buffer,
                VkExtent2D extent,
                VkDescriptorSet scene_descriptor_set,
                VkDescriptorSet material_descriptor_set,
                const GpuMesh& mesh) const;

private:
    void create_pipeline_layout(VkDescriptorSetLayout scene_set_layout,
                                VkDescriptorSetLayout material_set_layout);

    void create_graphics_pipeline(MeshPipelineFormats formats,
                                  const std::filesystem::path& shader_directory);

    void reset() noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    VkPipelineLayout pipeline_layout_{VK_NULL_HANDLE};
    VkPipeline pipeline_{VK_NULL_HANDLE};
};

} // namespace afterlight::graphics::vulkan
