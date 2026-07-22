#include <afterlight/graphics/vulkan/gpu_mesh.hpp>
#include <afterlight/graphics/vulkan/mesh_pipeline.hpp>
#include <afterlight/scene/mesh.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace afterlight::graphics::vulkan
{

namespace
{

constexpr std::uint32_t spirv_magic = 0x07230203U;

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

[[nodiscard]] std::vector<std::uint32_t> load_spirv(const std::filesystem::path& path)
{
    std::error_code error;

    const std::uintmax_t byte_count = std::filesystem::file_size(path, error);

    if (error)
    {
        throw std::runtime_error{"failed to query shader binary: " + path.string()};
    }

    if (byte_count < 5U * sizeof(std::uint32_t) || byte_count % sizeof(std::uint32_t) != 0)
    {
        throw std::runtime_error{"invalid SPIR-V binary size: " + path.string()};
    }

    const auto maximum_size = static_cast<std::uintmax_t>(std::numeric_limits<std::size_t>::max());

    const auto maximum_stream_size =
        static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max());

    if (byte_count > maximum_size || byte_count > maximum_stream_size)
    {
        throw std::runtime_error{"shader binary is too large: " + path.string()};
    }

    std::vector<char> bytes(static_cast<std::size_t>(byte_count));

    std::ifstream stream{path, std::ios::binary};

    if (!stream)
    {
        throw std::runtime_error{"failed to open shader binary: " + path.string()};
    }

    stream.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));

    if (!stream)
    {
        throw std::runtime_error{"failed to read shader binary: " + path.string()};
    }

    std::vector<std::uint32_t> code(bytes.size() / sizeof(std::uint32_t));

    std::memcpy(code.data(), bytes.data(), bytes.size());

    if (code.front() != spirv_magic)
    {
        throw std::runtime_error{"shader binary is not SPIR-V: " + path.string()};
    }

    return code;
}

[[nodiscard]] VkShaderModule create_shader_module(VkDevice device,
                                                  std::span<const std::uint32_t> code)
{
    VkShaderModuleCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    create_info.codeSize = code.size_bytes();
    create_info.pCode = code.data();

    VkShaderModule module = VK_NULL_HANDLE;

    require_success(vkCreateShaderModule(device, &create_info, nullptr, &module),
                    "vkCreateShaderModule");

    return module;
}

} // namespace

MeshPipeline::MeshPipeline(VkDevice device,
                           VkFormat color_format,
                           const std::filesystem::path& shader_directory)
    : device_{device}
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::invalid_argument{"mesh pipeline requires a Vulkan device"};
    }

    try
    {
        create_pipeline_layout();

        create_graphics_pipeline(color_format, shader_directory);
    }
    catch (...)
    {
        reset();
        throw;
    }
}

MeshPipeline::~MeshPipeline()
{
    reset();
}

void MeshPipeline::record(VkCommandBuffer command_buffer,
                          VkExtent2D extent,
                          const scene::TransformRows& transform,
                          const GpuMesh& mesh) const
{
    if (command_buffer == VK_NULL_HANDLE || pipeline_ == VK_NULL_HANDLE)
    {
        throw std::logic_error{"mesh pipeline is not recordable"};
    }

    VkViewport viewport{};

    viewport.x = 0.0F;
    viewport.y = 0.0F;

    viewport.width = static_cast<float>(extent.width);

    viewport.height = static_cast<float>(extent.height);

    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;

    const VkRect2D scissor{
        .offset =
            {
                .x = 0,
                .y = 0,
            },
        .extent = extent,
    };

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    vkCmdPushConstants(command_buffer,
                       pipeline_layout_,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       static_cast<std::uint32_t>(sizeof(scene::TransformRows)),
                       &transform);

    mesh.record(command_buffer);
}

void MeshPipeline::create_pipeline_layout()
{
    VkPushConstantRange push_constant{};

    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    push_constant.offset = 0;

    push_constant.size = static_cast<std::uint32_t>(sizeof(scene::TransformRows));

    VkPipelineLayoutCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    create_info.pushConstantRangeCount = 1;

    create_info.pPushConstantRanges = &push_constant;

    require_success(vkCreatePipelineLayout(device_, &create_info, nullptr, &pipeline_layout_),
                    "vkCreatePipelineLayout");
}

void MeshPipeline::create_graphics_pipeline(VkFormat color_format,
                                            const std::filesystem::path& shader_directory)
{
    const std::vector<std::uint32_t> vertex_code = load_spirv(shader_directory / "mesh.vert.spv");

    const std::vector<std::uint32_t> fragment_code = load_spirv(shader_directory / "mesh.frag.spv");

    const VkShaderModule vertex_module = create_shader_module(device_, vertex_code);

    VkShaderModule fragment_module = VK_NULL_HANDLE;

    try
    {
        fragment_module = create_shader_module(device_, fragment_code);
    }
    catch (...)
    {
        vkDestroyShaderModule(device_, vertex_module, nullptr);

        throw;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};

    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;

    stages[0].module = vertex_module;
    stages[0].pName = "vs_main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    stages[1].module = fragment_module;
    stages[1].pName = "ps_main";

    VkVertexInputBindingDescription binding{};

    binding.binding = 0;

    binding.stride = static_cast<std::uint32_t>(sizeof(scene::Vertex));

    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributes{};

    attributes[0].location = 0;
    attributes[0].binding = 0;

    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;

    attributes[0].offset = static_cast<std::uint32_t>(offsetof(scene::Vertex, position));

    attributes[1].location = 1;
    attributes[1].binding = 0;

    attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;

    attributes[1].offset = static_cast<std::uint32_t>(offsetof(scene::Vertex, color));

    VkPipelineVertexInputStateCreateInfo vertex_input{};

    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertex_input.vertexBindingDescriptionCount = 1;

    vertex_input.pVertexBindingDescriptions = &binding;

    vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());

    vertex_input.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};

    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_state{};

    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization{};

    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    rasterization.polygonMode = VK_POLYGON_MODE_FILL;

    rasterization.cullMode = VK_CULL_MODE_NONE;

    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    rasterization.lineWidth = 1.0F;

    VkPipelineMultisampleStateCreateInfo multisample{};

    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_attachment{};

    color_attachment.colorWriteMask =
        static_cast<VkColorComponentFlags>(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

    VkPipelineColorBlendStateCreateInfo color_blend{};

    color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

    color_blend.attachmentCount = 1;

    color_blend.pAttachments = &color_attachment;

    constexpr std::array<VkDynamicState, 2> dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};

    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

    dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());

    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPipelineRenderingCreateInfo rendering{};

    rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

    rendering.colorAttachmentCount = 1;

    rendering.pColorAttachmentFormats = &color_format;

    VkGraphicsPipelineCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    create_info.pNext = &rendering;

    create_info.stageCount = static_cast<std::uint32_t>(stages.size());

    create_info.pStages = stages.data();

    create_info.pVertexInputState = &vertex_input;

    create_info.pInputAssemblyState = &input_assembly;

    create_info.pViewportState = &viewport_state;

    create_info.pRasterizationState = &rasterization;

    create_info.pMultisampleState = &multisample;

    create_info.pColorBlendState = &color_blend;

    create_info.pDynamicState = &dynamic_state;

    create_info.layout = pipeline_layout_;

    create_info.renderPass = VK_NULL_HANDLE;

    create_info.subpass = 0;

    create_info.basePipelineHandle = VK_NULL_HANDLE;

    create_info.basePipelineIndex = -1;

    const VkResult result =
        vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline_);

    vkDestroyShaderModule(device_, fragment_module, nullptr);

    vkDestroyShaderModule(device_, vertex_module, nullptr);

    require_success(result, "vkCreateGraphicsPipelines");
}

void MeshPipeline::reset() noexcept
{
    if (pipeline_ != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device_, pipeline_, nullptr);

        pipeline_ = VK_NULL_HANDLE;
    }

    if (pipeline_layout_ != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

        pipeline_layout_ = VK_NULL_HANDLE;
    }
}

} // namespace afterlight::graphics::vulkan
