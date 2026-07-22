#include <afterlight/scene/mesh.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

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

[[nodiscard]] float triangle_area_squared(const afterlight::scene::Vertex& first,
                                          const afterlight::scene::Vertex& second,
                                          const afterlight::scene::Vertex& third) noexcept
{
    const float first_x = second.position[0] - first.position[0];

    const float first_y = second.position[1] - first.position[1];

    const float first_z = second.position[2] - first.position[2];

    const float second_x = third.position[0] - first.position[0];

    const float second_y = third.position[1] - first.position[1];

    const float second_z = third.position[2] - first.position[2];

    const float cross_x = first_y * second_z - first_z * second_y;

    const float cross_y = first_z * second_x - first_x * second_z;

    const float cross_z = first_x * second_y - first_y * second_x;

    return cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;
}

[[nodiscard]] float normal_length(const afterlight::scene::Vertex& vertex) noexcept
{
    return std::sqrt(vertex.normal[0] * vertex.normal[0] + vertex.normal[1] * vertex.normal[1] +
                     vertex.normal[2] * vertex.normal[2]);
}

} // namespace

int main()
{
    TestContext test;

    const afterlight::scene::MeshData mesh = afterlight::scene::make_observatory_aperture();

    test.expect(mesh.vertices.size() == 96, "flat-shaded aperture has ninety-six vertices");

    test.expect(mesh.indices.size() == 144, "aperture retains forty-eight triangles");

    bool indices_valid = true;
    bool values_valid = true;
    bool normals_normalized = true;
    bool triangles_non_degenerate = true;

    float minimum_depth = std::numeric_limits<float>::max();

    float maximum_depth = std::numeric_limits<float>::lowest();

    for (const std::uint16_t index : mesh.indices)
    {
        indices_valid = indices_valid && static_cast<std::size_t>(index) < mesh.vertices.size();
    }

    for (const afterlight::scene::Vertex& vertex : mesh.vertices)
    {
        minimum_depth = std::min(minimum_depth, vertex.position[2]);

        maximum_depth = std::max(maximum_depth, vertex.position[2]);

        for (const float value : vertex.position)
        {
            values_valid = values_valid && std::isfinite(value);
        }

        for (const float value : vertex.normal)
        {
            values_valid = values_valid && std::isfinite(value);
        }

        for (const float value : vertex.color)
        {
            values_valid = values_valid && std::isfinite(value) && value >= 0.0F && value <= 1.0F;
        }

        normals_normalized =
            normals_normalized && std::fabs(normal_length(vertex) - 1.0F) < 0.0001F;
    }

    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3)
    {
        const afterlight::scene::Vertex& first =
            mesh.vertices[static_cast<std::size_t>(mesh.indices[index])];

        const afterlight::scene::Vertex& second =
            mesh.vertices[static_cast<std::size_t>(mesh.indices[index + 1])];

        const afterlight::scene::Vertex& third =
            mesh.vertices[static_cast<std::size_t>(mesh.indices[index + 2])];

        triangles_non_degenerate =
            triangles_non_degenerate && triangle_area_squared(first, second, third) > 0.000001F;
    }

    test.expect(indices_valid, "all mesh indices address existing vertices");

    test.expect(values_valid, "mesh attributes are finite and bounded");

    test.expect(normals_normalized, "every surface normal has unit length");

    test.expect(minimum_depth < -0.1F && maximum_depth > 0.1F,
                "aperture contains front and rear surfaces");

    test.expect(triangles_non_degenerate, "all aperture triangles have non-zero area");

    return test.exit_code();
}
