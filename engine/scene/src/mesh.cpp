#include <afterlight/scene/mesh.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

namespace afterlight::scene
{

namespace
{

constexpr std::size_t aperture_segment_count = 6;

constexpr std::array<std::array<float, 2>, aperture_segment_count> outer_positions{{
    {1.0F, 0.0F},
    {0.5F, 0.8660254F},
    {-0.5F, 0.8660254F},
    {-1.0F, 0.0F},
    {-0.5F, -0.8660254F},
    {0.5F, -0.8660254F},
}};

constexpr std::array<std::array<float, 2>, aperture_segment_count> inner_positions{{
    {0.58F, 0.0F},
    {0.29F, 0.5022947F},
    {-0.29F, 0.5022947F},
    {-0.58F, 0.0F},
    {-0.29F, -0.5022947F},
    {0.29F, -0.5022947F},
}};

constexpr std::array<std::array<float, 3>, aperture_segment_count> segment_colors{{
    {1.0F, 0.31F, 0.08F},
    {1.0F, 0.67F, 0.12F},
    {0.18F, 0.82F, 1.0F},
    {0.11F, 0.48F, 1.0F},
    {0.49F, 0.18F, 1.0F},
    {0.91F, 0.16F, 0.72F},
}};

[[nodiscard]] std::array<float, 3> inner_color(const std::array<float, 3>& color) noexcept
{
    return {
        color[0] * 0.32F + 0.04F,
        color[1] * 0.32F + 0.04F,
        color[2] * 0.32F + 0.06F,
    };
}

} // namespace

MeshData make_observatory_aperture()
{
    MeshData mesh;

    mesh.vertices.reserve(aperture_segment_count * 2);
    mesh.indices.reserve(aperture_segment_count * 6);

    for (std::size_t segment = 0; segment < aperture_segment_count; ++segment)
    {
        const std::array<float, 2>& outer = outer_positions[segment];

        const std::array<float, 2>& inner = inner_positions[segment];

        const std::array<float, 3>& color = segment_colors[segment];

        mesh.vertices.push_back({
            .position =
                {
                    outer[0],
                    outer[1],
                    0.0F,
                },
            .color = color,
        });

        mesh.vertices.push_back({
            .position =
                {
                    inner[0],
                    inner[1],
                    0.0F,
                },
            .color = inner_color(color),
        });
    }

    for (std::uint16_t segment = 0; segment < static_cast<std::uint16_t>(aperture_segment_count);
         ++segment)
    {
        const std::uint16_t next =
            static_cast<std::uint16_t>((segment + 1U) % aperture_segment_count);

        const std::uint16_t outer_current = static_cast<std::uint16_t>(segment * 2U);

        const std::uint16_t inner_current = static_cast<std::uint16_t>(outer_current + 1U);

        const std::uint16_t outer_next = static_cast<std::uint16_t>(next * 2U);

        const std::uint16_t inner_next = static_cast<std::uint16_t>(outer_next + 1U);

        mesh.indices.insert(mesh.indices.end(),
                            {
                                outer_current,
                                outer_next,
                                inner_next,
                                outer_current,
                                inner_next,
                                inner_current,
                            });
    }

    return mesh;
}

} // namespace afterlight::scene
