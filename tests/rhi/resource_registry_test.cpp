#include <afterlight/rhi/resource.hpp>
#include <afterlight/rhi/resource_state_tracker.hpp>
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
    using afterlight::rhi::ResourceError;
    using afterlight::rhi::ResourceRegistry;
    using afterlight::rhi::ResourceState;
    using afterlight::rhi::ResourceStateTracker;
    using afterlight::rhi::TextureDesc;
    using afterlight::rhi::TextureFormat;

    TestContext test;
    ResourceRegistry registry;
    ResourceStateTracker states;

    const auto invalid_extent = registry.create_texture({
        .width = 0,
        .height = 720,
        .format = TextureFormat::bgra8_srgb,
    });

    test.expect(!invalid_extent.has_value() &&
                    invalid_extent.error() == ResourceError::invalid_extent,
                "zero-width textures are rejected");

    const auto undefined_format = registry.create_texture({
        .width = 1280,
        .height = 720,
        .format = TextureFormat::undefined,
    });

    test.expect(!undefined_format.has_value() &&
                    undefined_format.error() == ResourceError::undefined_format,
                "undefined texture formats are rejected");

    const TextureDesc descriptor{
        .width = 1280,
        .height = 720,
        .format = TextureFormat::bgra8_srgb,
        .external = true,
    };

    const auto first = registry.create_texture(descriptor);

    test.expect(first.has_value() && first->valid() && registry.contains(*first),
                "a valid texture receives a live handle");

    test.expect(registry.live_texture_count() == 1, "live resource count is maintained");

    const TextureDesc* stored = first.has_value() ? registry.descriptor(*first) : nullptr;

    test.expect(stored != nullptr && stored->width == 1280 && stored->height == 720 &&
                    stored->external,
                "texture descriptors remain queryable");

    test.expect(first.has_value() && states.track(*first, ResourceState::undefined),
                "live resources can be state-tracked");

    const auto transition = first.has_value()
                                ? states.transition(*first, ResourceState::color_attachment)
                                : std::nullopt;

    test.expect(transition.has_value() && transition->before == ResourceState::undefined &&
                    transition->after == ResourceState::color_attachment,
                "resource transitions preserve before and after states");

    test.expect(first.has_value() && states.forget(*first),
                "resource state entries can be removed");

    test.expect(first.has_value() && registry.destroy_texture(*first),
                "live handles can be destroyed");

    test.expect(first.has_value() && !registry.contains(*first), "destroyed handles become stale");

    const auto second = registry.create_texture(descriptor);

    test.expect(first.has_value() && second.has_value() && first->index == second->index &&
                    first->generation != second->generation,
                "reused slots advance their generation");

    test.expect(registry.live_texture_count() == 1, "slot reuse preserves the live count");

    return test.exit_code();
}
