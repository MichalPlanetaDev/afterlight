#include <afterlight/core/build_info.hpp>

namespace afterlight::core
{

BuildInfo current_build_info() noexcept
{
    return {
        .product_name = "Afterlight",
        .semantic_version = "0.8.0-dev",
        .milestone = "Surface Normals and Directional Lighting",
        .revision = "local",
    };
}

} // namespace afterlight::core
