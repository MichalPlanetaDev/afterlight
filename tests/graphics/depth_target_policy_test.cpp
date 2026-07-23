#include <afterlight/graphics/vulkan/depth_target.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace
{

[[nodiscard]] int failure(std::string_view message)
{
    std::cerr << "depth-target policy test failed: " << message << '\n';
    return EXIT_FAILURE;
}

} // namespace

int main()
{
    using afterlight::graphics::vulkan::depth_target_count_for_swapchain;

    if (depth_target_count_for_swapchain(1U) != 1U)
    {
        return failure("one swapchain image did not require one depth target");
    }

    if (depth_target_count_for_swapchain(3U) != 3U)
    {
        return failure("depth-target count did not match the swapchain image count");
    }

    bool rejected_empty_swapchain = false;

    try
    {
        static_cast<void>(depth_target_count_for_swapchain(0U));
    }
    catch (const std::invalid_argument&)
    {
        rejected_empty_swapchain = true;
    }
    catch (...)
    {
        return failure("zero swapchain images produced the wrong exception type");
    }

    if (!rejected_empty_swapchain)
    {
        return failure("an empty swapchain unexpectedly accepted zero depth targets");
    }

    if constexpr (sizeof(std::size_t) > sizeof(std::uint32_t))
    {
        const std::size_t oversized =
            static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + std::size_t{1U};

        bool rejected_oversized_swapchain = false;

        try
        {
            static_cast<void>(depth_target_count_for_swapchain(oversized));
        }
        catch (const std::length_error&)
        {
            rejected_oversized_swapchain = true;
        }
        catch (...)
        {
            return failure("an oversized count produced the wrong exception type");
        }

        if (!rejected_oversized_swapchain)
        {
            return failure("an unrepresentable depth-target count was accepted");
        }
    }

    return EXIT_SUCCESS;
}
