#include <afterlight/graphics/vulkan/device_selection.hpp>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

class TestContext final
{
public:
    void expect(bool condition, std::string_view description)
    {
        if (!condition)
        {
            ++failures_;

            std::cerr << "FAIL: " << description << '\n';
        }
    }

    [[nodiscard]] int exit_code() const noexcept
    {
        return failures_ == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

private:
    int failures_{};
};

[[nodiscard]] afterlight::graphics::vulkan::AdapterCandidate
valid_candidate(std::string name, afterlight::graphics::vulkan::AdapterKind kind)
{
    return {
        .name = std::move(name),
        .kind = kind,
        .api_version = VK_API_VERSION_1_3,
        .max_image_dimension_2d = 8192,
        .swapchain_extension = true,
        .surface_formats = true,
        .present_modes = true,
        .dynamic_rendering = true,
        .synchronization2 = true,
        .queue_families =
            {
                {
                    .graphics = true,
                    .presentation = true,
                },
            },
    };
}

} // namespace

int main()
{
    using afterlight::graphics::vulkan::AdapterCandidate;

    using afterlight::graphics::vulkan::AdapterKind;

    using afterlight::graphics::vulkan::select_adapter;

    TestContext test;

    std::vector<AdapterCandidate> candidates;

    candidates.push_back(valid_candidate("Integrated", AdapterKind::integrated));

    candidates.push_back(valid_candidate("Discrete", AdapterKind::discrete));

    const auto preferred = select_adapter(candidates);

    test.expect(preferred.has_value(), "a compatible adapter is selected");

    test.expect(preferred.has_value() && preferred->candidate_index == 1,
                "a discrete adapter is preferred");

    AdapterCandidate split_queues = valid_candidate("Split queues", AdapterKind::integrated);

    split_queues.queue_families = {
        {
            .graphics = true,
            .presentation = false,
        },
        {
            .graphics = false,
            .presentation = true,
        },
    };

    const std::array<AdapterCandidate, 1> split_candidates{split_queues};

    const auto split_selection = select_adapter(split_candidates);

    test.expect(split_selection.has_value() && split_selection->graphics_queue_family == 0 &&
                    split_selection->present_queue_family == 1,
                "separate graphics and presentation queues are supported");

    AdapterCandidate missing_swapchain = valid_candidate("No swapchain", AdapterKind::discrete);

    missing_swapchain.swapchain_extension = false;

    const std::array<AdapterCandidate, 1> missing_swapchain_candidates{missing_swapchain};

    test.expect(!select_adapter(missing_swapchain_candidates).has_value(),
                "an adapter without VK_KHR_swapchain is rejected");

    AdapterCandidate old_api = valid_candidate("Vulkan 1.2", AdapterKind::discrete);

    old_api.api_version = VK_API_VERSION_1_2;

    const std::array<AdapterCandidate, 1> old_api_candidates{old_api};

    test.expect(!select_adapter(old_api_candidates).has_value(),
                "an adapter below Vulkan 1.3 is rejected");

    AdapterCandidate missing_dynamic_rendering =
        valid_candidate("No dynamic rendering", AdapterKind::discrete);

    missing_dynamic_rendering.dynamic_rendering = false;

    const std::array<AdapterCandidate, 1> missing_dynamic_rendering_candidates{
        missing_dynamic_rendering};

    test.expect(!select_adapter(missing_dynamic_rendering_candidates).has_value(),
                "an adapter without dynamic rendering is rejected");

    AdapterCandidate missing_synchronization2 =
        valid_candidate("No synchronization2", AdapterKind::discrete);

    missing_synchronization2.synchronization2 = false;

    const std::array<AdapterCandidate, 1> missing_synchronization2_candidates{
        missing_synchronization2};

    test.expect(!select_adapter(missing_synchronization2_candidates).has_value(),
                "an adapter without synchronization2 is rejected");

    AdapterCandidate no_present_queue = valid_candidate("No presentation", AdapterKind::discrete);

    no_present_queue.queue_families = {
        {
            .graphics = true,
            .presentation = false,
        },
    };

    const std::array<AdapterCandidate, 1> no_present_candidates{no_present_queue};

    test.expect(!select_adapter(no_present_candidates).has_value(),
                "an adapter without presentation support is rejected");

    return test.exit_code();
}
