#include <afterlight/scene/camera.hpp>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

[[nodiscard]] std::array<float, 4>
transform_point(const afterlight::scene::TransformRows& transform,
                const std::array<float, 4>& point) noexcept
{
    const std::array<std::array<float, 4>, 4> rows{
        transform.row_0,
        transform.row_1,
        transform.row_2,
        transform.row_3,
    };

    std::array<float, 4> result{};

    for (std::size_t row = 0; row < rows.size(); ++row)
    {
        for (std::size_t column = 0; column < point.size(); ++column)
        {
            result[row] += rows[row][column] * point[column];
        }
    }

    return result;
}

} // namespace

int main()
{
    TestContext test;

    const afterlight::scene::TransformRows transform =
        afterlight::scene::make_observatory_transform(16.0F / 9.0F, 0.0F);

    const std::array<float, 4> clip = transform_point(transform,
                                                      {
                                                          0.0F,
                                                          0.0F,
                                                          0.0F,
                                                          1.0F,
                                                      });

    const float normalized_x = clip[0] / clip[3];

    const float normalized_y = clip[1] / clip[3];

    const float normalized_z = clip[2] / clip[3];

    test.expect(clip[3] > 0.0F, "camera places the aperture in front of the eye");

    test.expect(std::fabs(normalized_x) < 1.0F && std::fabs(normalized_y) < 1.0F,
                "aperture origin lies inside the viewport");

    test.expect(normalized_z >= 0.0F && normalized_z <= 1.0F,
                "camera uses Vulkan zero-to-one depth");

    const afterlight::scene::TransformRows rotated =
        afterlight::scene::make_observatory_transform(16.0F / 9.0F, 0.7F);

    test.expect(std::fabs(transform.row_0[0] - rotated.row_0[0]) > 0.01F,
                "model rotation changes the camera transform");

    bool invalid_aspect_rejected = false;

    try
    {
        static_cast<void>(afterlight::scene::make_observatory_transform(0.0F, 0.0F));
    }
    catch (const std::invalid_argument&)
    {
        invalid_aspect_rejected = true;
    }

    test.expect(invalid_aspect_rejected, "non-positive aspect ratios are rejected");

    return test.exit_code();
}
