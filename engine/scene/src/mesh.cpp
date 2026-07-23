#include <afterlight/scene/mesh.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace afterlight::scene
{

namespace
{

constexpr std::size_t aperture_segment_count = 6U;
constexpr float front_depth = 0.18F;
constexpr float back_depth = -0.18F;

using Position = std::array<float, 3>;
using Direction = std::array<float, 3>;
using Color = std::array<float, 3>;
using PlanarPoint = std::array<float, 2>;
using TextureCoordinate = std::array<float, 2>;
using QuadPositions = std::array<Position, 4>;
using QuadTextureCoordinates = std::array<TextureCoordinate, 4>;

struct QuadSurface final
{
    QuadPositions positions{};
    Direction normal{};
    Color color{};
    QuadTextureCoordinates texture_coordinates{};
};

struct RadialNormalParameters final
{
    PlanarPoint first{};
    PlanarPoint second{};
    bool inward{};
};

struct ColorAdjustment final
{
    float scale{};
    float offset{};
};

constexpr std::array<PlanarPoint, aperture_segment_count> outer_positions{{
    {1.0F, 0.0F},
    {0.5F, 0.8660254F},
    {-0.5F, 0.8660254F},
    {-1.0F, 0.0F},
    {-0.5F, -0.8660254F},
    {0.5F, -0.8660254F},
}};

constexpr std::array<PlanarPoint, aperture_segment_count> inner_positions{{
    {0.58F, 0.0F},
    {0.29F, 0.5022947F},
    {-0.29F, 0.5022947F},
    {-0.58F, 0.0F},
    {-0.29F, -0.5022947F},
    {0.29F, -0.5022947F},
}};

constexpr std::array<Color, aperture_segment_count> segment_colors{{
    {1.0F, 0.31F, 0.08F},
    {1.0F, 0.67F, 0.12F},
    {0.18F, 0.82F, 1.0F},
    {0.11F, 0.48F, 1.0F},
    {0.49F, 0.18F, 1.0F},
    {0.91F, 0.16F, 0.72F},
}};

[[nodiscard]] Position position_at(const PlanarPoint& point, float depth) noexcept
{
    return {
        point[0],
        point[1],
        depth,
    };
}

[[nodiscard]] constexpr TextureCoordinate
planar_texture_coordinate(const PlanarPoint& point) noexcept
{
    return {
        point[0] * 0.5F + 0.5F,
        point[1] * 0.5F + 0.5F,
    };
}

[[nodiscard]] float segment_texture_coordinate(std::size_t boundary) noexcept
{
    return static_cast<float>(boundary) / static_cast<float>(aperture_segment_count);
}

[[nodiscard]] Direction radial_normal(const RadialNormalParameters& parameters)
{
    float horizontal = parameters.first[0] + parameters.second[0];
    float vertical = parameters.first[1] + parameters.second[1];

    const float length = std::sqrt(horizontal * horizontal + vertical * vertical);

    if (length <= 0.0F)
    {
        return {
            1.0F,
            0.0F,
            0.0F,
        };
    }

    const float orientation = parameters.inward ? -1.0F : 1.0F;

    horizontal = horizontal / length * orientation;
    vertical = vertical / length * orientation;

    return {
        horizontal,
        vertical,
        0.0F,
    };
}

[[nodiscard]] Color adjusted_color(const Color& color, const ColorAdjustment& adjustment) noexcept
{
    return {
        color[0] * adjustment.scale + adjustment.offset,
        color[1] * adjustment.scale + adjustment.offset,
        color[2] * adjustment.scale + adjustment.offset,
    };
}

void append_quad(MeshData& mesh, const QuadSurface& surface)
{
    const auto base = static_cast<std::uint16_t>(mesh.vertices.size());

    for (std::size_t corner = 0U; corner < surface.positions.size(); ++corner)
    {
        mesh.vertices.push_back({
            .position = surface.positions[corner],
            .normal = surface.normal,
            .color = surface.color,
            .texture_coordinate = surface.texture_coordinates[corner],
        });
    }

    const auto second = static_cast<std::uint16_t>(base + 1U);

    const auto third = static_cast<std::uint16_t>(base + 2U);

    const auto fourth = static_cast<std::uint16_t>(base + 3U);

    mesh.indices.insert(mesh.indices.end(),
                        {
                            base,
                            second,
                            third,
                            base,
                            third,
                            fourth,
                        });
}

} // namespace

MeshData make_observatory_aperture()
{
    MeshData mesh;

    mesh.vertices.reserve(aperture_segment_count * 16U);
    mesh.indices.reserve(aperture_segment_count * 24U);

    for (std::size_t segment = 0U; segment < aperture_segment_count; ++segment)
    {
        const std::size_t next = (segment + 1U) % aperture_segment_count;

        const PlanarPoint& outer = outer_positions[segment];

        const PlanarPoint& next_outer = outer_positions[next];

        const PlanarPoint& inner = inner_positions[segment];

        const PlanarPoint& next_inner = inner_positions[next];

        const Color& color = segment_colors[segment];

        const Color rear_color = adjusted_color(color,
                                                {
                                                    .scale = 0.20F,
                                                    .offset = 0.012F,
                                                });

        const Color outer_color = adjusted_color(color,
                                                 {
                                                     .scale = 0.52F,
                                                     .offset = 0.025F,
                                                 });

        const Color inner_color = adjusted_color(color,
                                                 {
                                                     .scale = 0.30F,
                                                     .offset = 0.018F,
                                                 });

        const float first_boundary = segment_texture_coordinate(segment);

        const float second_boundary = segment_texture_coordinate(segment + 1U);

        append_quad(mesh,
                    {
                        .positions =
                            {
                                position_at(outer, front_depth),
                                position_at(next_outer, front_depth),
                                position_at(next_inner, front_depth),
                                position_at(inner, front_depth),
                            },
                        .normal =
                            {
                                0.0F,
                                0.0F,
                                1.0F,
                            },
                        .color = color,
                        .texture_coordinates =
                            QuadTextureCoordinates{
                                planar_texture_coordinate(outer),
                                planar_texture_coordinate(next_outer),
                                planar_texture_coordinate(next_inner),
                                planar_texture_coordinate(inner),
                            },
                    });

        append_quad(mesh,
                    {
                        .positions =
                            {
                                position_at(outer, back_depth),
                                position_at(inner, back_depth),
                                position_at(next_inner, back_depth),
                                position_at(next_outer, back_depth),
                            },
                        .normal =
                            {
                                0.0F,
                                0.0F,
                                -1.0F,
                            },
                        .color = rear_color,
                        .texture_coordinates =
                            QuadTextureCoordinates{
                                planar_texture_coordinate(outer),
                                planar_texture_coordinate(inner),
                                planar_texture_coordinate(next_inner),
                                planar_texture_coordinate(next_outer),
                            },
                    });

        append_quad(mesh,
                    {
                        .positions =
                            {
                                position_at(outer, front_depth),
                                position_at(outer, back_depth),
                                position_at(next_outer, back_depth),
                                position_at(next_outer, front_depth),
                            },
                        .normal = radial_normal({
                            .first = outer,
                            .second = next_outer,
                            .inward = false,
                        }),
                        .color = outer_color,
                        .texture_coordinates =
                            QuadTextureCoordinates{
                                TextureCoordinate{
                                    first_boundary,
                                    0.0F,
                                },
                                TextureCoordinate{
                                    first_boundary,
                                    1.0F,
                                },
                                TextureCoordinate{
                                    second_boundary,
                                    1.0F,
                                },
                                TextureCoordinate{
                                    second_boundary,
                                    0.0F,
                                },
                            },
                    });

        append_quad(mesh,
                    {
                        .positions =
                            {
                                position_at(inner, front_depth),
                                position_at(next_inner, front_depth),
                                position_at(next_inner, back_depth),
                                position_at(inner, back_depth),
                            },
                        .normal = radial_normal({
                            .first = inner,
                            .second = next_inner,
                            .inward = true,
                        }),
                        .color = inner_color,
                        .texture_coordinates =
                            QuadTextureCoordinates{
                                TextureCoordinate{
                                    first_boundary,
                                    0.0F,
                                },
                                TextureCoordinate{
                                    second_boundary,
                                    0.0F,
                                },
                                TextureCoordinate{
                                    second_boundary,
                                    1.0F,
                                },
                                TextureCoordinate{
                                    first_boundary,
                                    1.0F,
                                },
                            },
                    });
    }

    return mesh;
}

} // namespace afterlight::scene
