#include <afterlight/graphics/vulkan/barrier.hpp>
#include <cstdlib>
#include <iostream>
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
    using afterlight::graphics::vulkan::make_image_barrier;

    using afterlight::graphics::vulkan::translate_resource_state;

    using afterlight::rhi::ResourceState;
    using afterlight::rhi::TextureHandle;
    using afterlight::rhi::TextureTransition;

    TestContext test;

    const auto color = translate_resource_state(ResourceState::color_attachment);

    test.expect(color.stage_mask == VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT &&
                    color.access_mask == VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT &&
                    color.layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                "color attachments map to writable color output");

    const auto present = translate_resource_state(ResourceState::present);

    test.expect(present.stage_mask == VK_PIPELINE_STAGE_2_NONE &&
                    present.access_mask == VK_ACCESS_2_NONE &&
                    present.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                "presentation state maps to present layout");

    const TextureTransition transition{
        .texture =
            TextureHandle{
                .index = 2,
                .generation = 4,
            },
        .before = ResourceState::undefined,
        .after = ResourceState::color_attachment,
    };

    const VkImageMemoryBarrier2 barrier = make_image_barrier(VK_NULL_HANDLE, transition);

    test.expect(barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                    barrier.newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
                    barrier.image == VK_NULL_HANDLE &&
                    barrier.subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT &&
                    barrier.subresourceRange.levelCount == 1 &&
                    barrier.subresourceRange.layerCount == 1,
                "RHI transitions produce complete Vulkan image barriers");

    return test.exit_code();
}
