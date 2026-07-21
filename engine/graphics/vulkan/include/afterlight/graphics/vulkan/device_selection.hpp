#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

enum class AdapterKind : std::uint8_t
{
    other,
    integrated,
    discrete,
    virtual_gpu,
    cpu,
};

struct QueueFamilySupport final
{
    bool graphics{};
    bool presentation{};
};

struct AdapterCandidate final
{
    std::string name;
    AdapterKind kind{AdapterKind::other};
    std::uint32_t api_version{};
    std::uint32_t max_image_dimension_2d{};
    bool swapchain_extension{};
    bool surface_formats{};
    bool present_modes{};
    bool dynamic_rendering{};
    bool synchronization2{};
    std::vector<QueueFamilySupport> queue_families;
};

struct AdapterSelection final
{
    std::size_t candidate_index{};
    std::uint32_t graphics_queue_family{};
    std::uint32_t present_queue_family{};
    std::uint64_t score{};
};

[[nodiscard]] std::optional<AdapterSelection>
select_adapter(std::span<const AdapterCandidate> candidates);

[[nodiscard]] AdapterKind adapter_kind(VkPhysicalDeviceType type) noexcept;

[[nodiscard]] std::string_view to_string(AdapterKind kind) noexcept;

} // namespace afterlight::graphics::vulkan
