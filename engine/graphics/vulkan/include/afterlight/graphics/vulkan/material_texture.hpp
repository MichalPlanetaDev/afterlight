#pragma once

#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

inline constexpr std::uint32_t material_descriptor_set = 1U;
inline constexpr std::uint32_t material_image_binding = 0U;
inline constexpr std::uint32_t material_sampler_binding = 1U;

struct MaterialTextureSpec final
{
    std::uint32_t width{64U};
    std::uint32_t height{64U};
    std::uint32_t ring_count{6U};
};

struct MaterialTextureData final
{
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<std::byte> texels;
    std::uint64_t checksum{};
};

struct TextureMemoryRequirements final
{
    std::uint32_t type_bits{};
    VkMemoryPropertyFlags required{};
    VkMemoryPropertyFlags preferred{};
};

struct TextureMemoryType final
{
    std::uint32_t index{};
    bool coherent{};
};

[[nodiscard]] std::optional<TextureMemoryType>
choose_texture_memory_type(const VkPhysicalDeviceMemoryProperties& properties,
                           TextureMemoryRequirements requirements) noexcept;

[[nodiscard]] MaterialTextureData
make_aperture_material_texture(MaterialTextureSpec specification = {});

class MaterialTexture final
{
public:
    MaterialTexture(VulkanContext& context, const MaterialTextureData& data);

    ~MaterialTexture();

    MaterialTexture(const MaterialTexture&) = delete;
    MaterialTexture& operator=(const MaterialTexture&) = delete;
    MaterialTexture(MaterialTexture&&) = delete;
    MaterialTexture& operator=(MaterialTexture&&) = delete;

    [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const noexcept;

    [[nodiscard]] VkDescriptorSet descriptor_set() const noexcept;

    [[nodiscard]] std::uint32_t width() const noexcept;

    [[nodiscard]] std::uint32_t height() const noexcept;

    [[nodiscard]] std::uint64_t checksum() const noexcept;

private:
    void create_image_and_upload(const MaterialTextureData& data);
    void create_image_view();
    void create_sampler();
    void create_descriptor_resources();
    void reset() noexcept;

    VulkanContext& context_;

    VkImage image_{VK_NULL_HANDLE};
    VkDeviceMemory memory_{VK_NULL_HANDLE};
    VkImageView image_view_{VK_NULL_HANDLE};
    VkSampler sampler_{VK_NULL_HANDLE};

    VkDescriptorSetLayout descriptor_set_layout_{VK_NULL_HANDLE};
    VkDescriptorPool descriptor_pool_{VK_NULL_HANDLE};
    VkDescriptorSet descriptor_set_{VK_NULL_HANDLE};

    std::uint32_t width_{};
    std::uint32_t height_{};
    std::uint64_t checksum_{};
};

} // namespace afterlight::graphics::vulkan
