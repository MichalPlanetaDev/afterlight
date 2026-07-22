#include <afterlight/graphics/vulkan/scene_uniforms.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

struct MemoryPropertyRequirement final
{
    VkMemoryPropertyFlags available{};
    VkMemoryPropertyFlags required{};
};

[[nodiscard]] bool contains_properties(MemoryPropertyRequirement requirement) noexcept
{
    return (requirement.available & requirement.required) == requirement.required;
}

} // namespace

UniformMemorySelection
choose_uniform_memory_type(const VkPhysicalDeviceMemoryProperties& properties,
                           std::uint32_t allowed_types)
{
    std::optional<UniformMemorySelection> fallback;

    for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index)
    {
        const bool allowed = (allowed_types & (std::uint32_t{1} << index)) != 0;

        if (!allowed)
        {
            continue;
        }

        const VkMemoryPropertyFlags flags = properties.memoryTypes[index].propertyFlags;

        const bool host_visible = contains_properties({
            .available = flags,
            .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        });

        if (!host_visible)
        {
            continue;
        }

        const bool coherent = contains_properties({
            .available = flags,
            .required = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        });

        if (coherent)
        {
            return {
                .index = index,
                .coherent = true,
            };
        }

        if (!fallback.has_value())
        {
            fallback = {
                .index = index,
                .coherent = false,
            };
        }
    }

    if (fallback.has_value())
    {
        return *fallback;
    }

    throw std::runtime_error{"Vulkan device exposes no host-visible uniform memory"};
}

SceneUniforms::SceneUniforms(VulkanContext& context, std::uint32_t frame_count)
    : context_{context}, frame_count_{frame_count}
{
    if (frame_count_ == 0)
    {
        throw std::invalid_argument{"scene uniforms require at least one frame"};
    }

    try
    {
        create_descriptor_set_layout();

        frames_.reserve(frame_count_);

        for (std::uint32_t index = 0; index < frame_count_; ++index)
        {
            static_cast<void>(index);

            frames_.push_back(create_frame_allocation());
        }

        create_descriptor_pool();
        allocate_descriptor_sets();
        write_descriptors();
    }
    catch (...)
    {
        reset();
        throw;
    }
}

SceneUniforms::~SceneUniforms()
{
    reset();
}

void SceneUniforms::update(std::uint32_t frame_index, const scene::SceneFrameData& frame_data)
{
    const std::size_t resource_index = static_cast<std::size_t>(frame_index);

    if (resource_index >= frames_.size())
    {
        throw std::out_of_range{"scene uniform frame index is invalid"};
    }

    FrameAllocation& allocation = frames_[resource_index];

    if (allocation.mapped == nullptr)
    {
        throw std::logic_error{"scene uniform memory is not mapped"};
    }

    std::memcpy(allocation.mapped, &frame_data, sizeof(frame_data));

    if (allocation.coherent)
    {
        return;
    }

    VkMappedMemoryRange range{};

    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;

    range.memory = allocation.memory;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;

    require_success(vkFlushMappedMemoryRanges(context_.device(), 1, &range),
                    "vkFlushMappedMemoryRanges(scene uniforms)");
}

VkDescriptorSetLayout SceneUniforms::descriptor_set_layout() const noexcept
{
    return descriptor_set_layout_;
}

VkDescriptorSet SceneUniforms::descriptor_set(std::uint32_t frame_index) const
{
    const std::size_t resource_index = static_cast<std::size_t>(frame_index);

    if (resource_index >= descriptor_sets_.size())
    {
        throw std::out_of_range{"scene descriptor frame index is invalid"};
    }

    return descriptor_sets_[resource_index];
}

std::uint32_t SceneUniforms::frame_count() const noexcept
{
    return frame_count_;
}

void SceneUniforms::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding binding{};

    binding.binding = 0;

    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    binding.descriptorCount = 1;

    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    create_info.bindingCount = 1;
    create_info.pBindings = &binding;

    require_success(vkCreateDescriptorSetLayout(
                        context_.device(), &create_info, nullptr, &descriptor_set_layout_),
                    "vkCreateDescriptorSetLayout(scene)");
}

SceneUniforms::FrameAllocation SceneUniforms::create_frame_allocation()
{
    FrameAllocation allocation;

    try
    {
        VkBufferCreateInfo buffer_info{};

        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

        buffer_info.size = sizeof(scene::SceneFrameData);

        buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        require_success(
            vkCreateBuffer(context_.device(), &buffer_info, nullptr, &allocation.buffer),
            "vkCreateBuffer(scene uniforms)");

        VkMemoryRequirements requirements{};

        vkGetBufferMemoryRequirements(context_.device(), allocation.buffer, &requirements);

        VkPhysicalDeviceMemoryProperties properties{};

        vkGetPhysicalDeviceMemoryProperties(context_.physical_device(), &properties);

        const UniformMemorySelection selection =
            choose_uniform_memory_type(properties, requirements.memoryTypeBits);

        allocation.coherent = selection.coherent;

        VkMemoryAllocateInfo allocation_info{};

        allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        allocation_info.allocationSize = requirements.size;

        allocation_info.memoryTypeIndex = selection.index;

        require_success(
            vkAllocateMemory(context_.device(), &allocation_info, nullptr, &allocation.memory),
            "vkAllocateMemory(scene uniforms)");

        require_success(
            vkBindBufferMemory(context_.device(), allocation.buffer, allocation.memory, 0),
            "vkBindBufferMemory(scene uniforms)");

        require_success(
            vkMapMemory(
                context_.device(), allocation.memory, 0, VK_WHOLE_SIZE, 0, &allocation.mapped),
            "vkMapMemory(scene uniforms)");
    }
    catch (...)
    {
        destroy_frame_allocation(allocation);

        throw;
    }

    return allocation;
}

void SceneUniforms::create_descriptor_pool()
{
    VkDescriptorPoolSize pool_size{};

    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    pool_size.descriptorCount = frame_count_;

    VkDescriptorPoolCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    create_info.maxSets = frame_count_;

    create_info.poolSizeCount = 1;
    create_info.pPoolSizes = &pool_size;

    require_success(
        vkCreateDescriptorPool(context_.device(), &create_info, nullptr, &descriptor_pool_),
        "vkCreateDescriptorPool(scene)");
}

void SceneUniforms::allocate_descriptor_sets()
{
    std::vector<VkDescriptorSetLayout> layouts(frame_count_, descriptor_set_layout_);

    descriptor_sets_.resize(frame_count_);

    VkDescriptorSetAllocateInfo allocate_info{};

    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

    allocate_info.descriptorPool = descriptor_pool_;

    allocate_info.descriptorSetCount = frame_count_;

    allocate_info.pSetLayouts = layouts.data();

    require_success(
        vkAllocateDescriptorSets(context_.device(), &allocate_info, descriptor_sets_.data()),
        "vkAllocateDescriptorSets(scene)");
}

void SceneUniforms::write_descriptors()
{
    for (std::uint32_t index = 0; index < frame_count_; ++index)
    {
        const std::size_t resource_index = static_cast<std::size_t>(index);

        VkDescriptorBufferInfo buffer_info{};

        buffer_info.buffer = frames_[resource_index].buffer;

        buffer_info.offset = 0;

        buffer_info.range = sizeof(scene::SceneFrameData);

        VkWriteDescriptorSet write{};

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

        write.dstSet = descriptor_sets_[resource_index];

        write.dstBinding = 0;
        write.dstArrayElement = 0;

        write.descriptorCount = 1;

        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(context_.device(), 1, &write, 0, nullptr);
    }
}

void SceneUniforms::destroy_frame_allocation(FrameAllocation& allocation) noexcept
{
    if (allocation.mapped != nullptr)
    {
        vkUnmapMemory(context_.device(), allocation.memory);

        allocation.mapped = nullptr;
    }

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

    allocation.coherent = false;
}

void SceneUniforms::reset() noexcept
{
    if (descriptor_pool_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(context_.device(), descriptor_pool_, nullptr);

        descriptor_pool_ = VK_NULL_HANDLE;
    }

    descriptor_sets_.clear();

    for (FrameAllocation& allocation : frames_)
    {
        destroy_frame_allocation(allocation);
    }

    frames_.clear();

    if (descriptor_set_layout_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(context_.device(), descriptor_set_layout_, nullptr);

        descriptor_set_layout_ = VK_NULL_HANDLE;
    }
}

} // namespace afterlight::graphics::vulkan
