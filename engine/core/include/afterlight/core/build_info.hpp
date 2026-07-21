#pragma once

#include <string_view>

namespace afterlight::core
{

struct BuildInfo final
{
    std::string_view product_name;
    std::string_view semantic_version;
    std::string_view milestone;
    std::string_view revision;
};

[[nodiscard]] BuildInfo current_build_info() noexcept;

} // namespace afterlight::core
