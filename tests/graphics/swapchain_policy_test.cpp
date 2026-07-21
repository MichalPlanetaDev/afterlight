#include <afterlight/graphics/vulkan/swapchain.hpp>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

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

} // namespace

int main()
{
    using afterlight::graphics::vulkan::choose_present_mode;

    using afterlight::graphics::vulkan::choose_surface_format;

    using afterlight::graphics::vulkan::choose_swapchain_extent;

    using afterlight::graphics::vulkan::choose_swapchain_image_count;

    TestContext test;

    const std::array<VkSurfaceFormatKHR, 2> formats{
        VkSurfaceFormatKHR{
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
        VkSurfaceFormatKHR{
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
    };

    test.expect(choose_surface_format(formats).format == VK_FORMAT_B8G8R8A8_SRGB,
                "sRGB BGRA is preferred");

    const std::array<VkSurfaceFormatKHR, 1> undefined_format{
        VkSurfaceFormatKHR{
            .format = VK_FORMAT_UNDEFINED,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
    };

    test.expect(choose_surface_format(undefined_format).format == VK_FORMAT_B8G8R8A8_SRGB,
                "undefined surface format selects sRGB BGRA");

    const std::array<VkPresentModeKHR, 2> modes{
        VK_PRESENT_MODE_FIFO_KHR,
        VK_PRESENT_MODE_MAILBOX_KHR,
    };

    test.expect(choose_present_mode(modes) == VK_PRESENT_MODE_MAILBOX_KHR,
                "mailbox presentation is preferred");

    VkSurfaceCapabilitiesKHR fixed{};

    fixed.currentExtent = {
        .width = 1920,
        .height = 1080,
    };

    const VkExtent2D fixed_extent = choose_swapchain_extent(fixed,
                                                            {
                                                                .width = 640,
                                                                .height = 360,
                                                            });

    test.expect(fixed_extent.width == 1920 && fixed_extent.height == 1080,
                "fixed surface extent is preserved");

    VkSurfaceCapabilitiesKHR variable{};

    variable.currentExtent = {
        .width = std::numeric_limits<std::uint32_t>::max(),
        .height = std::numeric_limits<std::uint32_t>::max(),
    };

    variable.minImageExtent = {
        .width = 320,
        .height = 240,
    };

    variable.maxImageExtent = {
        .width = 1920,
        .height = 1080,
    };

    const VkExtent2D clamped_extent = choose_swapchain_extent(variable,
                                                              {
                                                                  .width = 2560,
                                                                  .height = 120,
                                                              });

    test.expect(clamped_extent.width == 1920 && clamped_extent.height == 240,
                "variable extent is clamped to surface limits");

    VkSurfaceCapabilitiesKHR image_limits{};

    image_limits.minImageCount = 2;
    image_limits.maxImageCount = 3;

    test.expect(choose_swapchain_image_count(image_limits) == 3,
                "one image beyond the minimum is requested");

    image_limits.minImageCount = 3;
    image_limits.maxImageCount = 3;

    test.expect(choose_swapchain_image_count(image_limits) == 3,
                "image count respects the maximum");

    return test.exit_code();
}
