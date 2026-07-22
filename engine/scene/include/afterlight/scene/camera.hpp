#pragma once

#include <array>

namespace afterlight::scene
{

struct TransformRows final
{
    std::array<float, 4> row_0{};
    std::array<float, 4> row_1{};
    std::array<float, 4> row_2{};
    std::array<float, 4> row_3{};
};

static_assert(sizeof(TransformRows) == 16U * sizeof(float));

struct SceneFrameParameters final
{
    float aspect_ratio{};
    float rotation_radians{};
};

struct SceneFrameData final
{
    TransformRows transform{};

    std::array<float, 4> light_direction_intensity{};

    std::array<float, 4> view_direction_exposure{};
};

static_assert(sizeof(SceneFrameData) == 24U * sizeof(float));

[[nodiscard]] SceneFrameData make_observatory_frame(SceneFrameParameters parameters);

} // namespace afterlight::scene
