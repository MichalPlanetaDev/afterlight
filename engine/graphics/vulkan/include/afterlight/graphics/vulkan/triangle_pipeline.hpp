#pragma once

#include <filesystem>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

class TrianglePipeline final
{
public:
    TrianglePipeline(VkDevice device,
                     VkFormat color_format,
                     const std::filesystem::path& shader_directory);

    ~TrianglePipeline();

    TrianglePipeline(const TrianglePipeline&) = delete;
    TrianglePipeline& operator=(const TrianglePipeline&) = delete;
    TrianglePipeline(TrianglePipeline&&) = delete;
    TrianglePipeline& operator=(TrianglePipeline&&) = delete;

    void record(VkCommandBuffer command_buffer, VkExtent2D extent) const;

private:
    void create_pipeline_layout();

    void create_graphics_pipeline(VkFormat color_format,
                                  const std::filesystem::path& shader_directory);

    void reset() noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    VkPipelineLayout pipeline_layout_{VK_NULL_HANDLE};
    VkPipeline pipeline_{VK_NULL_HANDLE};
};

} // namespace afterlight::graphics::vulkan
