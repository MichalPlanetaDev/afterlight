#include <afterlight/scene/camera.hpp>
#include <array>
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
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

[[nodiscard]] std::array<float, 4> direction_parameter(const glm::vec3& direction,
                                                       float parameter) noexcept
{
    return {
        direction.x,
        direction.y,
        direction.z,
        parameter,
    };
}

} // namespace

SceneFrameData make_observatory_frame(SceneFrameParameters parameters)
{
    if (!std::isfinite(parameters.aspect_ratio) || parameters.aspect_ratio <= 0.0F)
    {
        throw std::invalid_argument{"camera aspect ratio must be finite and positive"};
    }

    if (!std::isfinite(parameters.rotation_radians))
    {
        throw std::invalid_argument{"camera rotation must be finite"};
    }

    glm::mat4 model{1.0F};

    model = glm::rotate(model,
                        parameters.rotation_radians,
                        glm::vec3{
                            0.0F,
                            1.0F,
                            0.0F,
                        });

    model = glm::rotate(model,
                        glm::radians(-10.0F),
                        glm::vec3{
                            1.0F,
                            0.0F,
                            0.0F,
                        });

    const glm::vec3 camera_position{
        0.0F,
        0.0F,
        3.1F,
    };

    const glm::mat4 view = glm::lookAtRH(camera_position,
                                         glm::vec3{
                                             0.0F,
                                             0.0F,
                                             0.0F,
                                         },
                                         glm::vec3{
                                             0.0F,
                                             1.0F,
                                             0.0F,
                                         });

    glm::mat4 projection =
        glm::perspectiveRH_ZO(glm::radians(48.0F), parameters.aspect_ratio, 0.1F, 20.0F);

    projection[1][1] *= -1.0F;

    const glm::mat3 inverse_rotation = glm::transpose(glm::mat3{model});

    const glm::vec3 world_light_direction = glm::normalize(glm::vec3{
        -0.42F,
        0.68F,
        0.60F,
    });

    const glm::vec3 local_light_direction =
        glm::normalize(inverse_rotation * world_light_direction);

    const glm::vec3 world_view_direction = glm::normalize(camera_position);

    const glm::vec3 local_view_direction = glm::normalize(inverse_rotation * world_view_direction);

    return {
        .transform = to_rows(projection * view * model),
        .light_direction_intensity = direction_parameter(local_light_direction, 1.85F),
        .view_direction_exposure = direction_parameter(local_view_direction, 1.20F),
    };
}

} // namespace afterlight::scene
