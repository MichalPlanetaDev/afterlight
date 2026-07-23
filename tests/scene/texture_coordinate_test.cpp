#include <afterlight/scene/mesh.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{

constexpr std::size_t aperture_segment_count = 6U;
constexpr std::size_t vertices_per_segment = 16U;
constexpr std::size_t front_surface_offset = 0U;
constexpr std::size_t rear_surface_offset = 4U;
constexpr std::size_t outer_wall_offset = 8U;
constexpr std::size_t inner_wall_offset = 12U;
constexpr float coordinate_tolerance = 0.0001F;

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

struct CoordinateExpectation final
{
    std::array<float, 2> value{};
};

[[nodiscard]] bool matches(const afterlight::scene::Vertex& vertex,
                           CoordinateExpectation expectation) noexcept
{
    return std::fabs(vertex.texture_coordinate[0] - expectation.value[0]) <= coordinate_tolerance &&
           std::fabs(vertex.texture_coordinate[1] - expectation.value[1]) <= coordinate_tolerance;
}

[[nodiscard]] CoordinateExpectation
planar_expectation(const afterlight::scene::Vertex& vertex) noexcept
{
    return {
        .value =
            {
                vertex.position[0] * 0.5F + 0.5F,
                vertex.position[1] * 0.5F + 0.5F,
            },
    };
}

[[nodiscard]] float segment_boundary_coordinate(std::size_t boundary) noexcept
{
    return static_cast<float>(boundary) / static_cast<float>(aperture_segment_count);
}

} // namespace

int main()
{
    TestContext test;

    const afterlight::scene::MeshData mesh = afterlight::scene::make_observatory_aperture();

    const afterlight::scene::MeshData repeated = afterlight::scene::make_observatory_aperture();

    test.expect(mesh.vertices.size() == aperture_segment_count * vertices_per_segment,
                "texture-coordinate contract preserves ninety-six vertices");

    test.expect(mesh.vertices.size() == repeated.vertices.size(),
                "repeated aperture generation preserves vertex count");

    bool deterministic = true;
    bool finite_and_bounded = true;
    bool planar_faces_valid = true;
    bool radial_walls_valid = true;

    for (std::size_t index = 0U; index < mesh.vertices.size(); ++index)
    {
        const auto& coordinate = mesh.vertices[index].texture_coordinate;

        deterministic = deterministic && coordinate == repeated.vertices[index].texture_coordinate;

        finite_and_bounded = finite_and_bounded && std::isfinite(coordinate[0]) &&
                             std::isfinite(coordinate[1]) && coordinate[0] >= 0.0F &&
                             coordinate[0] <= 1.0F && coordinate[1] >= 0.0F &&
                             coordinate[1] <= 1.0F;
    }

    for (std::size_t segment = 0U; segment < aperture_segment_count; ++segment)
    {
        const std::size_t base = segment * vertices_per_segment;

        for (std::size_t corner = 0U; corner < 4U; ++corner)
        {
            const auto& front = mesh.vertices[base + front_surface_offset + corner];

            const auto& rear = mesh.vertices[base + rear_surface_offset + corner];

            planar_faces_valid = planar_faces_valid && matches(front, planar_expectation(front)) &&
                                 matches(rear, planar_expectation(rear));
        }

        const float first_boundary = segment_boundary_coordinate(segment);

        const float second_boundary = segment_boundary_coordinate(segment + 1U);

        const std::array<CoordinateExpectation, 4> outer_expectations{{
            {
                .value =
                    {
                        first_boundary,
                        0.0F,
                    },
            },
            {
                .value =
                    {
                        first_boundary,
                        1.0F,
                    },
            },
            {
                .value =
                    {
                        second_boundary,
                        1.0F,
                    },
            },
            {
                .value =
                    {
                        second_boundary,
                        0.0F,
                    },
            },
        }};

        const std::array<CoordinateExpectation, 4> inner_expectations{{
            {
                .value =
                    {
                        first_boundary,
                        0.0F,
                    },
            },
            {
                .value =
                    {
                        second_boundary,
                        0.0F,
                    },
            },
            {
                .value =
                    {
                        second_boundary,
                        1.0F,
                    },
            },
            {
                .value =
                    {
                        first_boundary,
                        1.0F,
                    },
            },
        }};

        for (std::size_t corner = 0U; corner < 4U; ++corner)
        {
            radial_walls_valid = radial_walls_valid &&
                                 matches(mesh.vertices[base + outer_wall_offset + corner],
                                         outer_expectations[corner]) &&
                                 matches(mesh.vertices[base + inner_wall_offset + corner],
                                         inner_expectations[corner]);
        }
    }

    const auto& first_seam_vertex = mesh.vertices[outer_wall_offset];

    const std::size_t final_segment_base = (aperture_segment_count - 1U) * vertices_per_segment;

    const auto& final_seam_vertex = mesh.vertices[final_segment_base + outer_wall_offset + 3U];

    const bool seam_positions_match = first_seam_vertex.position == final_seam_vertex.position;

    const bool seam_coordinates_are_explicit =
        std::fabs(first_seam_vertex.texture_coordinate[0] - 0.0F) <= coordinate_tolerance &&
        std::fabs(final_seam_vertex.texture_coordinate[0] - 1.0F) <= coordinate_tolerance;

    test.expect(deterministic, "texture coordinates are byte deterministic");

    test.expect(finite_and_bounded, "all texture coordinates are finite and bounded");

    test.expect(planar_faces_valid, "front and rear surfaces use planar aperture mapping");

    test.expect(radial_walls_valid, "inner and outer walls use radial boundary mapping");

    test.expect(seam_positions_match, "radial seam duplicates the same geometric boundary");

    test.expect(seam_coordinates_are_explicit,
                "radial seam preserves distinct zero and one coordinates");

    return test.exit_code();
}
