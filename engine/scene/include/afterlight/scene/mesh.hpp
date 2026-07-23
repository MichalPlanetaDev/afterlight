#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace afterlight::scene
{

struct Vertex final
{
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<float, 3> color{};
    std::array<float, 2> texture_coordinate{};
};

struct MeshData final
{
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
};

[[nodiscard]] MeshData make_observatory_aperture();

} // namespace afterlight::scene
