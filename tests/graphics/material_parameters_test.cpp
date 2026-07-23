#include <afterlight/graphics/vulkan/material_texture.hpp>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>

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

} // namespace

int main()
{
    using afterlight::graphics::vulkan::ApertureMaterialParameters;
    using afterlight::graphics::vulkan::valid_aperture_material_parameters;

    TestContext test;

    const ApertureMaterialParameters parameters{};
    const ApertureMaterialParameters repeated{};

    using ParameterBytes = std::array<std::byte, sizeof(ApertureMaterialParameters)>;

    const ParameterBytes parameter_bytes = std::bit_cast<ParameterBytes>(parameters);

    const ParameterBytes repeated_bytes = std::bit_cast<ParameterBytes>(repeated);

    test.expect(sizeof(ApertureMaterialParameters) == 64U,
                "aperture material parameters occupy four sixteen-byte registers");

    test.expect(alignof(ApertureMaterialParameters) == 16U,
                "aperture material parameters preserve constant-buffer alignment");

    test.expect(std::is_standard_layout_v<ApertureMaterialParameters>,
                "aperture material parameters use a standard host layout");

    test.expect(parameter_bytes == repeated_bytes,
                "default aperture material bytes are deterministic");

    test.expect(valid_aperture_material_parameters(parameters),
                "default aperture material parameters are valid");

    test.expect(parameters.surface_response == std::array<float, 4>{0.62F, 2.2F, 0.075F, 0.19F},
                "surface response preserves the P13 shader defaults");

    test.expect(parameters.specular_response == std::array<float, 4>{72.0F, 16.0F, 0.30F, 0.08F},
                "specular response preserves the P13 shader defaults");

    test.expect(parameters.surface_accents == std::array<float, 4>{0.35F, 3.0F, 0.09F, 0.0F},
                "roughness tint and rim response preserve the P13 defaults");

    test.expect(parameters.specular_tint == std::array<float, 4>{0.34F, 0.36F, 0.38F, 0.0F},
                "specular tint preserves the P13 shader defaults");

    ApertureMaterialParameters negative_gain = parameters;
    negative_gain.surface_response[1] = -0.01F;

    test.expect(!valid_aperture_material_parameters(negative_gain),
                "negative material texture gain is rejected");

    ApertureMaterialParameters inverted_hemisphere = parameters;
    inverted_hemisphere.surface_response[2] = 0.25F;
    inverted_hemisphere.surface_response[3] = 0.10F;

    test.expect(!valid_aperture_material_parameters(inverted_hemisphere),
                "inverted hemisphere bounds are rejected");

    ApertureMaterialParameters inverted_specular_power = parameters;
    inverted_specular_power.specular_response[0] = 8.0F;
    inverted_specular_power.specular_response[1] = 32.0F;

    test.expect(!valid_aperture_material_parameters(inverted_specular_power),
                "rough surfaces cannot use a sharper highlight than smooth surfaces");

    ApertureMaterialParameters non_finite = parameters;
    non_finite.surface_accents[2] = std::numeric_limits<float>::infinity();

    test.expect(!valid_aperture_material_parameters(non_finite),
                "non-finite material parameters are rejected");

    ApertureMaterialParameters invalid_padding = parameters;
    invalid_padding.specular_tint[3] = 1.0F;

    test.expect(!valid_aperture_material_parameters(invalid_padding),
                "reserved material register components remain zero");

    test.expect(afterlight::graphics::vulkan::material_descriptor_set == 1U,
                "material descriptors remain in set one");

    test.expect(afterlight::graphics::vulkan::material_image_binding == 0U,
                "material image remains at binding zero");

    test.expect(afterlight::graphics::vulkan::material_sampler_binding == 1U,
                "material sampler remains at binding one");

    test.expect(afterlight::graphics::vulkan::material_parameter_binding == 2U,
                "persistent material parameters occupy binding two");

    return test.exit_code();
}
