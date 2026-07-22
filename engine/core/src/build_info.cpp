#include <afterlight/core/build_info.hpp>

namespace afterlight::core
{

BuildInfo current_build_info() noexcept
{
    return {
        .product_name = "Afterlight",
        .semantic_version = "0.6.0-dev",
        .milestone = "GPU Mesh and Camera",
        .revision = "local",
    };
}

} // namespace afterlight::core
