#include <afterlight/graphics/vulkan/scene_uniforms.hpp>
#include <afterlight/scene/camera.hpp>
#include <cstdint>
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

    VkPhysicalDeviceMemoryProperties properties{};

    properties.memoryTypeCount = 3;

    properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    properties.memoryTypes[1].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    properties.memoryTypes[2].propertyFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    const std::uint32_t all_types =
        (std::uint32_t{1} << 0U) | (std::uint32_t{1} << 1U) | (std::uint32_t{1} << 2U);

    const auto coherent =
        afterlight::graphics::vulkan::choose_uniform_memory_type(properties, all_types);

    test.expect(coherent.index == 2, "coherent host-visible memory is preferred");

    test.expect(coherent.coherent, "coherent selection reports its property");

    const auto fallback = afterlight::graphics::vulkan::choose_uniform_memory_type(
        properties, std::uint32_t{1} << 1U);

    test.expect(fallback.index == 1, "host-visible fallback is selected");

    test.expect(!fallback.coherent, "fallback requires an explicit flush");

    bool unavailable_rejected = false;

    try
    {
        static_cast<void>(afterlight::graphics::vulkan::choose_uniform_memory_type(
            properties, std::uint32_t{1} << 0U));
    }
    catch (const std::runtime_error&)
    {
        unavailable_rejected = true;
    }

    test.expect(unavailable_rejected, "non-host-visible memory is rejected");

    test.expect(sizeof(afterlight::scene::SceneFrameData) == 96U,
                "scene uniform block is ninety-six bytes");

    return test.exit_code();
}
