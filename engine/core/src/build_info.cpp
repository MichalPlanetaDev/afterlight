#include <afterlight/core/build_info.hpp>

namespace afterlight::core
{

BuildInfo current_build_info() noexcept
{
    return {
        .product_name = "Afterlight",
        .semantic_version = "0.14.0-dev",
        .milestone = "Persistent Material Parameter Block",
        .revision = "local",
    };
}

} // namespace afterlight::core
