#pragma once

#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <array>
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
inline constexpr std::uint32_t material_parameter_binding = 2U;

struct alignas(16) ApertureMaterialParameters final
{
    std::array<float, 4> surface_response{
        0.62F,
        2.2F,
        0.075F,
        0.19F,
    };

    std::array<float, 4> specular_response{
        72.0F,
        16.0F,
        0.30F,
        0.08F,
    };

    std::array<float, 4> surface_accents{
        0.35F,
        3.0F,
        0.09F,
        0.0F,
    };

    std::array<float, 4> specular_tint{
        0.34F,
        0.36F,
        0.38F,
        0.0F,
    };
};

static_assert(sizeof(ApertureMaterialParameters) == 64U);
static_assert(alignof(ApertureMaterialParameters) == 16U);

[[nodiscard]] bool
valid_aperture_material_parameters(const ApertureMaterialParameters& parameters) noexcept;

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
    MaterialTexture(VulkanContext& context,
                    const MaterialTextureData& data,
                    ApertureMaterialParameters parameters = {});

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

    [[nodiscard]] std::uint32_t parameter_byte_size() const noexcept;

    [[nodiscard]] const ApertureMaterialParameters& parameters() const noexcept;

private:
    void create_image_and_upload(const MaterialTextureData& data);
    void create_image_view();
    void create_sampler();
    void create_parameter_buffer();
    void create_descriptor_resources();
    void reset() noexcept;

    VulkanContext& context_;

    VkImage image_{VK_NULL_HANDLE};
    VkDeviceMemory memory_{VK_NULL_HANDLE};
    VkImageView image_view_{VK_NULL_HANDLE};
    VkSampler sampler_{VK_NULL_HANDLE};

    VkBuffer parameter_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory parameter_memory_{VK_NULL_HANDLE};

    ApertureMaterialParameters parameters_{};

    VkDescriptorSetLayout descriptor_set_layout_{VK_NULL_HANDLE};
    VkDescriptorPool descriptor_pool_{VK_NULL_HANDLE};
    VkDescriptorSet descriptor_set_{VK_NULL_HANDLE};

    std::uint32_t width_{};
    std::uint32_t height_{};
    std::uint64_t checksum_{};
};

} // namespace afterlight::graphics::vulkan
