#include <afterlight/scene/mesh.hpp>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
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

[[nodiscard]] float triangle_area_twice(const afterlight::scene::Vertex& first,
                                        const afterlight::scene::Vertex& second,
                                        const afterlight::scene::Vertex& third) noexcept
{
    const float first_x = second.position[0] - first.position[0];

    const float first_y = second.position[1] - first.position[1];

    const float second_x = third.position[0] - first.position[0];

    const float second_y = third.position[1] - first.position[1];

    return first_x * second_y - first_y * second_x;
}

} // namespace

int main()
{
    TestContext test;

    const afterlight::scene::MeshData mesh = afterlight::scene::make_observatory_aperture();

    test.expect(mesh.vertices.size() == 12, "aperture has twelve deterministic vertices");

    test.expect(mesh.indices.size() == 36, "aperture has twelve indexed triangles");

    bool indices_valid = true;
    bool values_finite = true;
    bool triangles_non_degenerate = true;

    for (const std::uint16_t index : mesh.indices)
    {
        indices_valid = indices_valid && static_cast<std::size_t>(index) < mesh.vertices.size();
    }

    for (const afterlight::scene::Vertex& vertex : mesh.vertices)
    {
        for (const float value : vertex.position)
        {
            values_finite = values_finite && std::isfinite(value);
        }

        for (const float value : vertex.color)
        {
            values_finite = values_finite && std::isfinite(value) && value >= 0.0F && value <= 1.0F;
        }
    }

    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3)
    {
        const afterlight::scene::Vertex& first =
            mesh.vertices[static_cast<std::size_t>(mesh.indices[index])];

        const afterlight::scene::Vertex& second =
            mesh.vertices[static_cast<std::size_t>(mesh.indices[index + 1])];

        const afterlight::scene::Vertex& third =
            mesh.vertices[static_cast<std::size_t>(mesh.indices[index + 2])];

        triangles_non_degenerate = triangles_non_degenerate &&
                                   std::fabs(triangle_area_twice(first, second, third)) > 0.0001F;
    }

    test.expect(indices_valid, "all mesh indices address existing vertices");

    test.expect(values_finite, "mesh positions and colors are finite");

    test.expect(triangles_non_degenerate, "all aperture triangles have non-zero area");

    return test.exit_code();
}
