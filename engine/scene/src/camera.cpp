#include <afterlight/scene/camera.hpp>
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <stdexcept>

namespace afterlight::scene
{

namespace
{

[[nodiscard]] TransformRows to_rows(const glm::mat4& matrix) noexcept
{
    return {
        .row_0 =
            {
                matrix[0][0],
                matrix[1][0],
                matrix[2][0],
                matrix[3][0],
            },
        .row_1 =
            {
                matrix[0][1],
                matrix[1][1],
                matrix[2][1],
                matrix[3][1],
            },
        .row_2 =
            {
                matrix[0][2],
                matrix[1][2],
                matrix[2][2],
                matrix[3][2],
            },
        .row_3 =
            {
                matrix[0][3],
                matrix[1][3],
                matrix[2][3],
                matrix[3][3],
            },
    };
}

} // namespace

TransformRows make_observatory_transform(float aspect_ratio, float rotation_radians)
{
    if (!std::isfinite(aspect_ratio) || aspect_ratio <= 0.0F)
    {
        throw std::invalid_argument{"camera aspect ratio must be finite and positive"};
    }

    if (!std::isfinite(rotation_radians))
    {
        throw std::invalid_argument{"camera rotation must be finite"};
    }

    glm::mat4 model{1.0F};

    model = glm::rotate(model, rotation_radians, glm::vec3{0.0F, 1.0F, 0.0F});

    model = glm::rotate(model, glm::radians(-10.0F), glm::vec3{1.0F, 0.0F, 0.0F});

    const glm::mat4 view = glm::lookAtRH(
        glm::vec3{0.0F, 0.0F, 3.1F}, glm::vec3{0.0F, 0.0F, 0.0F}, glm::vec3{0.0F, 1.0F, 0.0F});

    glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(48.0F), aspect_ratio, 0.1F, 20.0F);

    projection[1][1] *= -1.0F;

    return to_rows(projection * view * model);
}

} // namespace afterlight::scene
