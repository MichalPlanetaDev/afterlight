#include <afterlight/scene/camera.hpp>
#include <afterlight/scene/mesh.hpp>
#include <algorithm>
#include <array>
#include <cmath>
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

[[nodiscard]] float lighting_response(const std::array<float, 3>& normal,
                                      const std::array<float, 4>& light) noexcept
{
    return normal[0] * light[0] + normal[1] * light[1] + normal[2] * light[2];
}

} // namespace

int main()
{
    TestContext test;

    const afterlight::scene::MeshData mesh = afterlight::scene::make_observatory_aperture();

    const afterlight::scene::SceneFrameData frame = afterlight::scene::make_observatory_frame({
        .aspect_ratio = 16.0F / 9.0F,
        .rotation_radians = 0.32F,
    });

    float minimum_response = std::numeric_limits<float>::max();

    float maximum_response = std::numeric_limits<float>::lowest();

    for (const afterlight::scene::Vertex& vertex : mesh.vertices)
    {
        const float response = lighting_response(vertex.normal, frame.light_direction_intensity);

        minimum_response = std::min(minimum_response, response);

        maximum_response = std::max(maximum_response, response);
    }

    test.expect(minimum_response < -0.35F, "some aperture surfaces face away from the light");

    test.expect(maximum_response > 0.35F, "some aperture surfaces face the light");

    test.expect(maximum_response - minimum_response > 1.0F,
                "surface normals provide meaningful lighting contrast");

    test.expect(std::isfinite(minimum_response) && std::isfinite(maximum_response),
                "lighting responses remain finite");

    return test.exit_code();
}
