#include <afterlight/graphics/vulkan/depth_target.hpp>
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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
    TestContext test;

    constexpr std::array<VkFormat, 3> all_formats{
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
    };

    test.expect(afterlight::graphics::vulkan::choose_depth_format(all_formats) ==
                    VK_FORMAT_D32_SFLOAT,
                "D32 float is preferred when available");

    constexpr std::array<VkFormat, 2> fallback_formats{
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
    };

    test.expect(afterlight::graphics::vulkan::choose_depth_format(fallback_formats) ==
                    VK_FORMAT_D32_SFLOAT_S8_UINT,
                "D32 float stencil is the first fallback");

    constexpr std::array<VkFormat, 1> final_format{
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    test.expect(afterlight::graphics::vulkan::choose_depth_format(final_format) ==
                    VK_FORMAT_D24_UNORM_S8_UINT,
                "D24 stencil is the final fallback");

    bool empty_rejected = false;

    try
    {
        static_cast<void>(afterlight::graphics::vulkan::choose_depth_format({}));
    }
    catch (const std::invalid_argument&)
    {
        empty_rejected = true;
    }

    test.expect(empty_rejected, "an empty format set is rejected");

    return test.exit_code();
}
