#include <afterlight/graphics/vulkan/device_selection.hpp>

namespace afterlight::graphics::vulkan
{

namespace
{

struct QueueSelection final
{
    std::uint32_t graphics{};
    std::uint32_t presentation{};
};

[[nodiscard]] std::optional<QueueSelection>
select_queues(std::span<const QueueFamilySupport> families)
{
    std::optional<std::uint32_t> graphics;
    std::optional<std::uint32_t> presentation;

    for (std::size_t index = 0; index < families.size(); ++index)
    {
        const QueueFamilySupport& family = families[index];

        const auto family_index = static_cast<std::uint32_t>(index);

        if (family.graphics && family.presentation)
        {
            return QueueSelection{
                .graphics = family_index,
                .presentation = family_index,
            };
        }

        if (family.graphics && !graphics.has_value())
        {
            graphics = family_index;
        }

        if (family.presentation && !presentation.has_value())
        {
            presentation = family_index;
        }
    }

    if (!graphics.has_value() || !presentation.has_value())
    {
        return std::nullopt;
    }

    return QueueSelection{
        .graphics = *graphics,
        .presentation = *presentation,
    };
}

[[nodiscard]] std::uint64_t kind_score(AdapterKind kind) noexcept
{
    switch (kind)
    {
        case AdapterKind::discrete:
            return 10'000;

        case AdapterKind::integrated:
            return 5'000;

        case AdapterKind::virtual_gpu:
            return 2'500;

        case AdapterKind::cpu:
            return 1'000;

        case AdapterKind::other:
            return 500;
    }

    return 0;
}

[[nodiscard]] bool meets_requirements(const AdapterCandidate& candidate) noexcept
{
    return candidate.api_version >= VK_API_VERSION_1_3 && candidate.swapchain_extension &&
           candidate.surface_formats && candidate.present_modes && candidate.dynamic_rendering &&
           candidate.synchronization2;
}

} // namespace

std::optional<AdapterSelection> select_adapter(std::span<const AdapterCandidate> candidates)
{
    std::optional<AdapterSelection> best;

    for (std::size_t index = 0; index < candidates.size(); ++index)
    {
        const AdapterCandidate& candidate = candidates[index];

        if (!meets_requirements(candidate))
        {
            continue;
        }

        const std::optional<QueueSelection> queues = select_queues(candidate.queue_families);

        if (!queues.has_value())
        {
            continue;
        }

        std::uint64_t score = kind_score(candidate.kind) + candidate.max_image_dimension_2d;

        if (queues->graphics == queues->presentation)
        {
            score += 500;
        }

        if (!best.has_value() || score > best->score)
        {
            best = AdapterSelection{
                .candidate_index = index,
                .graphics_queue_family = queues->graphics,
                .present_queue_family = queues->presentation,
                .score = score,
            };
        }
    }

    return best;
}

AdapterKind adapter_kind(VkPhysicalDeviceType type) noexcept
{
    switch (type)
    {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return AdapterKind::integrated;

        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return AdapterKind::discrete;

        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return AdapterKind::virtual_gpu;

        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return AdapterKind::cpu;

        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        default:
            return AdapterKind::other;
    }
}

std::string_view to_string(AdapterKind kind) noexcept
{
    switch (kind)
    {
        case AdapterKind::integrated:
            return "integrated";

        case AdapterKind::discrete:
            return "discrete";

        case AdapterKind::virtual_gpu:
            return "virtual";

        case AdapterKind::cpu:
            return "cpu";

        case AdapterKind::other:
            return "other";
    }

    return "other";
}

} // namespace afterlight::graphics::vulkan
