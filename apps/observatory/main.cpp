#include <afterlight/core/application_lifecycle.hpp>
#include <afterlight/core/build_info.hpp>
#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <afterlight/platform/platform.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace
{

enum class RunMode : std::uint8_t
{
    desktop,
    platform_smoke,
    vulkan_smoke,
};

struct RunOptions final
{
    RunMode mode{RunMode::desktop};
};

[[nodiscard]] RunOptions parse_options(int argc, char* argv[])
{
    RunOptions options;

    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument{argv[index]};

        if (argument == "--smoke")
        {
            options.mode = RunMode::platform_smoke;
            continue;
        }

        if (argument == "--vulkan-smoke")
        {
            options.mode = RunMode::vulkan_smoke;
            continue;
        }

        throw std::invalid_argument{"unknown command-line argument: " + std::string{argument}};
    }

    return options;
}

void require_transition(const afterlight::core::LifecycleTransitionResult& result,
                        std::string_view operation)
{
    if (!result.has_value())
    {
        throw std::runtime_error{std::string{operation} + " lifecycle transition rejected"};
    }
}

void print_platform_smoke(const afterlight::core::BuildInfo& build,
                          const afterlight::platform::PlatformContext& platform,
                          const afterlight::platform::Window& window)
{
    const afterlight::platform::WindowSize size = window.size();

    std::cout << build.product_name << ' ' << build.semantic_version
              << " | platform=" << platform.video_driver() << " | window=" << size.width << 'x'
              << size.height << '\n';
}

void print_vulkan_device(const afterlight::core::BuildInfo& build,
                         const afterlight::graphics::vulkan::VulkanDeviceInfo& device)
{
    std::cout << build.product_name << ' ' << build.semantic_version << " | backend=vulkan"
              << " | device=" << device.name
              << " | api=" << afterlight::graphics::vulkan::format_api_version(device.api_version)
              << " | graphics_queue=" << device.graphics_queue_family
              << " | present_queue=" << device.present_queue_family
              << " | validation=" << (device.validation_enabled ? "on" : "off") << '\n';
}

int run(const RunOptions& options)
{
    afterlight::core::ApplicationLifecycle lifecycle;

    require_transition(lifecycle.initialize(), "initialize");

    const bool platform_smoke = options.mode == RunMode::platform_smoke;

    const bool finite_run = options.mode != RunMode::desktop;

    afterlight::platform::PlatformOptions platform_options;

    if (platform_smoke)
    {
        platform_options.video_driver = "dummy";
    }

    afterlight::platform::PlatformContext platform{platform_options};

    afterlight::platform::Window window{{
        .title = "Afterlight - The Last Observatory",
        .width = 1280,
        .height = 720,
        .resizable = true,
        .hidden = finite_run,
        .vulkan = !platform_smoke,
    }};

    std::unique_ptr<afterlight::graphics::vulkan::VulkanContext> vulkan;

    if (!platform_smoke)
    {
        vulkan = std::make_unique<afterlight::graphics::vulkan::VulkanContext>(window);
    }

    require_transition(lifecycle.start(), "start");

    const afterlight::core::BuildInfo build = afterlight::core::current_build_info();

    if (platform_smoke)
    {
        print_platform_smoke(build, platform, window);
    }
    else
    {
        print_vulkan_device(build, vulkan->device_info());
    }

    bool running = true;
    std::uint32_t frame_count = 0;

    const std::uint32_t frame_limit = finite_run ? 2U : 0U;

    while (running)
    {
        afterlight::platform::PlatformEvent event{
            .type = afterlight::platform::PlatformEventType::quit_requested,
        };

        while (window.poll_event(event))
        {
            if (event.type == afterlight::platform::PlatformEventType::quit_requested ||
                event.type == afterlight::platform::PlatformEventType::window_close_requested)
            {
                running = false;
            }
        }

        ++frame_count;

        if (frame_limit != 0U && frame_count >= frame_limit)
        {
            running = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    require_transition(lifecycle.request_stop(), "request stop");

    vulkan.reset();

    require_transition(lifecycle.shutdown(), "shutdown");

    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char* argv[])
{
    try
    {
        return run(parse_options(argc, argv));
    }
    catch (const std::exception& error)
    {
        std::cerr << "Afterlight startup failed: " << error.what() << '\n';

        return EXIT_FAILURE;
    }
}
