#include <afterlight/scene/mesh.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

namespace afterlight::scene
{

namespace
{

constexpr std::size_t aperture_segment_count = 6;
constexpr float front_depth = 0.18F;
constexpr float back_depth = -0.18F;

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

[[nodiscard]] std::array<float, 3>
scaled_color(const std::array<float, 3>& color, float scale, float offset) noexcept
{
    return {
        color[0] * scale + offset,
        color[1] * scale + offset,
        color[2] * scale + offset,
    };
}

[[nodiscard]] std::uint16_t vertex_index(std::uint16_t segment, std::uint16_t component) noexcept
{
    return static_cast<std::uint16_t>(segment * 4U + component);
}

void append_quad(std::vector<std::uint16_t>& indices,
                 std::uint16_t first,
                 std::uint16_t second,
                 std::uint16_t third,
                 std::uint16_t fourth)
{
    indices.insert(indices.end(),
                   {
                       first,
                       second,
                       third,
                       first,
                       third,
                       fourth,
                   });
}

} // namespace

MeshData make_observatory_aperture()
{
    MeshData mesh;

    mesh.vertices.reserve(aperture_segment_count * 4);

    mesh.indices.reserve(aperture_segment_count * 24);

    for (std::size_t segment = 0; segment < aperture_segment_count; ++segment)
    {
        const auto& outer = outer_positions[segment];

        const auto& inner = inner_positions[segment];

        const auto& color = segment_colors[segment];

        mesh.vertices.push_back({
            .position =
                {
                    outer[0],
                    outer[1],
                    front_depth,
                },
            .color = color,
        });

        mesh.vertices.push_back({
            .position =
                {
                    inner[0],
                    inner[1],
                    front_depth,
                },
            .color = scaled_color(color, 0.42F, 0.035F),
        });

        mesh.vertices.push_back({
            .position =
                {
                    outer[0],
                    outer[1],
                    back_depth,
                },
            .color = scaled_color(color, 0.22F, 0.018F),
        });

        mesh.vertices.push_back({
            .position =
                {
                    inner[0],
                    inner[1],
                    back_depth,
                },
            .color = scaled_color(color, 0.13F, 0.012F),
        });
    }

    for (std::uint16_t segment = 0; segment < static_cast<std::uint16_t>(aperture_segment_count);
         ++segment)
    {
        const std::uint16_t next =
            static_cast<std::uint16_t>((segment + 1U) % aperture_segment_count);

        const std::uint16_t outer_front = vertex_index(segment, 0);

        const std::uint16_t inner_front = vertex_index(segment, 1);

        const std::uint16_t outer_back = vertex_index(segment, 2);

        const std::uint16_t inner_back = vertex_index(segment, 3);

        const std::uint16_t next_outer_front = vertex_index(next, 0);

        const std::uint16_t next_inner_front = vertex_index(next, 1);

        const std::uint16_t next_outer_back = vertex_index(next, 2);

        const std::uint16_t next_inner_back = vertex_index(next, 3);

        append_quad(mesh.indices, outer_front, next_outer_front, next_inner_front, inner_front);

        append_quad(mesh.indices, outer_back, inner_back, next_inner_back, next_outer_back);

        append_quad(mesh.indices, outer_front, outer_back, next_outer_back, next_outer_front);

        append_quad(mesh.indices, inner_front, next_inner_front, next_inner_back, inner_back);
    }

    return mesh;
}

} // namespace afterlight::scene
