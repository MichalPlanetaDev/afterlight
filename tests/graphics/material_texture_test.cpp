#include <afterlight/graphics/vulkan/material_texture.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <volk.h>

namespace
{

class TestContext final
{
public:
    void expect(bool condition, std::string_view description)
    {
        if (!condition)
        {
            ++failures_;
            std::cerr << "FAIL: " << description << '\n';
        }
    }

    [[nodiscard]] int exit_code() const noexcept
    {
        return failures_ == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

private:
    int failures_{};
};

struct TexelCoordinate final
{
    std::uint32_t x{};
    std::uint32_t y{};
    std::size_t component{};
};

[[nodiscard]] std::uint8_t channel(const afterlight::graphics::vulkan::MaterialTextureData& texture,
                                   TexelCoordinate coordinate)
{
    constexpr std::size_t channel_count = 4U;

    const std::size_t pixel_index =
        static_cast<std::size_t>(coordinate.y) * static_cast<std::size_t>(texture.width) +
        static_cast<std::size_t>(coordinate.x);

    const std::size_t byte_index = pixel_index * channel_count + coordinate.component;

    return std::to_integer<std::uint8_t>(texture.texels.at(byte_index));
}

} // namespace

int main()
{
    using afterlight::graphics::vulkan::choose_texture_memory_type;
    using afterlight::graphics::vulkan::make_aperture_material_texture;
    using afterlight::graphics::vulkan::MaterialTextureSpec;
    using afterlight::graphics::vulkan::TextureMemoryRequirements;

    TestContext test;

    const auto texture = make_aperture_material_texture();
    const auto repeated = make_aperture_material_texture();

    constexpr std::size_t expected_texel_count =
        std::size_t{64U} * std::size_t{64U} * std::size_t{4U};

    test.expect(texture.width == 64U, "default texture width is stable");

    test.expect(texture.height == 64U, "default texture height is stable");

    test.expect(texture.texels.size() == expected_texel_count,
                "texture contains tightly packed RGBA8 texels");

    test.expect(texture.texels == repeated.texels,
                "procedural texture generation is byte deterministic");

    test.expect(texture.checksum == repeated.checksum, "procedural checksum is deterministic");

    test.expect(texture.checksum == UINT64_C(0xad3a091625158275),
                "procedural texture checksum matches the versioned contract");

    test.expect(channel(texture,
                        {
                            .x = 0U,
                            .y = 0U,
                            .component = 0U,
                        }) == 27U,
                "outer calibration field stores the expected red channel");

    test.expect(channel(texture,
                        {
                            .x = 0U,
                            .y = 0U,
                            .component = 3U,
                        }) == 204U,
                "outer calibration field stores the expected roughness");

    test.expect(channel(texture,
                        {
                            .x = 32U,
                            .y = 32U,
                            .component = 0U,
                        }) == 43U,
                "optical hub stores the expected red channel");

    test.expect(channel(texture,
                        {
                            .x = 32U,
                            .y = 32U,
                            .component = 1U,
                        }) == 116U,
                "optical hub stores the expected green channel");

    test.expect(channel(texture,
                        {
                            .x = 32U,
                            .y = 32U,
                            .component = 2U,
                        }) == 132U,
                "optical hub stores the expected blue channel");

    test.expect(channel(texture,
                        {
                            .x = 32U,
                            .y = 32U,
                            .component = 3U,
                        }) == 128U,
                "optical hub stores the expected roughness");

    bool zero_extent_rejected = false;

    try
    {
        static_cast<void>(make_aperture_material_texture({
            .width = 0U,
            .height = 64U,
            .ring_count = 6U,
        }));
    }
    catch (const std::invalid_argument&)
    {
        zero_extent_rejected = true;
    }
    catch (...)
    {
        test.expect(false, "zero extent produced the wrong exception type");
    }

    test.expect(zero_extent_rejected, "zero-width textures are rejected");

    bool oversized_extent_rejected = false;

    try
    {
        static_cast<void>(make_aperture_material_texture({
            .width = 4097U,
            .height = 64U,
            .ring_count = 6U,
        }));
    }
    catch (const std::length_error&)
    {
        oversized_extent_rejected = true;
    }
    catch (...)
    {
        test.expect(false, "oversized extent produced the wrong exception type");
    }

    test.expect(oversized_extent_rejected, "oversized textures are rejected");

    bool invalid_ring_count_rejected = false;

    try
    {
        static_cast<void>(make_aperture_material_texture({
            .width = 64U,
            .height = 64U,
            .ring_count = 0U,
        }));
    }
    catch (const std::invalid_argument&)
    {
        invalid_ring_count_rejected = true;
    }
    catch (...)
    {
        test.expect(false, "invalid ring count produced the wrong exception type");
    }

    test.expect(invalid_ring_count_rejected, "zero calibration rings are rejected");

    VkPhysicalDeviceMemoryProperties properties{};

    properties.memoryTypeCount = 3U;

    properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    properties.memoryTypes[1].propertyFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    properties.memoryTypes[2].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const auto coherent =
        choose_texture_memory_type(properties,
                                   TextureMemoryRequirements{
                                       .type_bits = 0b111U,
                                       .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                       .preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   });

    test.expect(coherent.has_value(), "coherent staging memory is available");

    test.expect(coherent.has_value() && coherent->index == 1U,
                "coherent staging memory is preferred");

    test.expect(coherent.has_value() && coherent->coherent,
                "coherent staging selection reports coherence");

    const auto device_local =
        choose_texture_memory_type(properties,
                                   TextureMemoryRequirements{
                                       .type_bits = 0b111U,
                                       .required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       .preferred = 0U,
                                   });

    test.expect(device_local.has_value(), "device-local texture memory is available");

    test.expect(device_local.has_value() && device_local->index == 2U,
                "device-local texture memory is selected");

    const auto fallback =
        choose_texture_memory_type(properties,
                                   TextureMemoryRequirements{
                                       .type_bits = 0b001U,
                                       .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                       .preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   });

    test.expect(fallback.has_value(), "non-coherent staging fallback is available");

    test.expect(fallback.has_value() && fallback->index == 0U,
                "non-coherent staging fallback uses the compatible type");

    test.expect(fallback.has_value() && !fallback->coherent,
                "non-coherent staging fallback requires an explicit flush");

    const auto unavailable =
        choose_texture_memory_type(properties,
                                   TextureMemoryRequirements{
                                       .type_bits = 0b001U,
                                       .required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       .preferred = 0U,
                                   });

    test.expect(!unavailable.has_value(), "incompatible texture memory is rejected");

    test.expect(afterlight::graphics::vulkan::material_descriptor_set == 1U,
                "material descriptors occupy set one");

    test.expect(afterlight::graphics::vulkan::material_image_binding == 0U,
                "material image occupies binding zero");

    test.expect(afterlight::graphics::vulkan::material_sampler_binding == 1U,
                "material sampler occupies binding one");

    return test.exit_code();
}
