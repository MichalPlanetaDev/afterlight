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

[[nodiscard]] TransformRows make_observatory_transform(float aspect_ratio, float rotation_radians);

} // namespace afterlight::scene
