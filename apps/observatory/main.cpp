#include <afterlight/core/application_lifecycle.hpp>
#include <afterlight/core/build_info.hpp>
#include <afterlight/graphics/vulkan/swapchain.hpp>
#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <afterlight/observatory/shader_paths.hpp>
#include <afterlight/platform/platform.hpp>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
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

struct FrameLoopResult final
{
    std::uint32_t presented_frames{};
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

void print_desktop_device(const afterlight::core::BuildInfo& build,
                          const afterlight::graphics::vulkan::VulkanDeviceInfo& device)
{
    std::cout << build.product_name << ' ' << build.semantic_version << " | backend=vulkan"
              << " | device=" << device.name
              << " | api=" << afterlight::graphics::vulkan::format_api_version(device.api_version)
              << " | validation=" << (device.validation_enabled ? "on" : "off") << '\n';
}

void print_frame_smoke(const afterlight::core::BuildInfo& build,
                       const afterlight::graphics::vulkan::VulkanDeviceInfo& device,
                       const afterlight::graphics::vulkan::SwapchainInfo& swapchain,
                       const afterlight::graphics::vulkan::GeometryInfo& geometry,
                       const afterlight::graphics::vulkan::SceneBindingInfo& bindings,
                       std::uint32_t presented_frames)
{
    std::cout << build.product_name << ' ' << build.semantic_version << " | backend=vulkan";

    std::cout << " | device=" << device.name;

    std::cout << " | presented=" << presented_frames;

    std::cout << " | extent=" << swapchain.width << 'x' << swapchain.height;

    std::cout << " | images=" << swapchain.image_count;

    std::cout << " | format=" << static_cast<std::uint32_t>(swapchain.format);

    std::cout << " | depth=" << static_cast<std::uint32_t>(swapchain.depth_format);

    std::cout << " | geometry=extruded-observatory-aperture";

    std::cout << " | vertices=" << geometry.vertex_count;

    std::cout << " | indices=" << geometry.index_count;

    std::cout << " | normals=" << geometry.normal_count;

    std::cout << " | mesh-memory=device-local";

    std::cout << " | lighting=directional";

    std::cout << " | uniforms=" << (bindings.descriptor_backed ? "descriptor-set" : "unavailable");

    std::cout << " | uniform-frames=" << bindings.frame_count;

    std::cout << " | validation=" << (device.validation_enabled ? "on" : "off") << '\n';
}

[[nodiscard]] bool process_events(afterlight::platform::Window& window,
                                  afterlight::graphics::vulkan::SwapchainRenderer& renderer)
{
    afterlight::platform::PlatformEvent event{
        .type = afterlight::platform::PlatformEventType::quit_requested,
    };

    while (window.poll_event(event))
    {
        switch (event.type)
        {
            case afterlight::platform::PlatformEventType::quit_requested:

            case afterlight::platform::PlatformEventType::window_close_requested:
                return false;

            case afterlight::platform::PlatformEventType::window_resized:
                renderer.request_resize();
                break;
        }
    }

    return true;
}

[[nodiscard]] FrameLoopResult
run_frame_loop(afterlight::platform::Window& window,
               afterlight::graphics::vulkan::SwapchainRenderer& renderer,
               bool smoke)
{
    FrameLoopResult result;

    std::uint32_t smoke_iterations = 0;
    bool running = true;

    while (running)
    {
        running = process_events(window, renderer);

        if (running && renderer.render_frame(window))
        {
            ++result.presented_frames;
        }

        if (smoke)
        {
            ++smoke_iterations;

            if (result.presented_frames >= 3)
            {
                running = false;
            }
            else if (smoke_iterations >= 240)
            {
                throw std::runtime_error{"Vulkan smoke did not present three frames"};
            }
        }

        if (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
    }

    return result;
}

int run_platform_smoke(afterlight::core::ApplicationLifecycle& lifecycle)
{
    {
        afterlight::platform::PlatformContext platform{{
            .video_driver = "dummy",
        }};

        afterlight::platform::Window window{{
            .title = "Afterlight - The Last Observatory",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .hidden = true,
            .vulkan = false,
        }};

        require_transition(lifecycle.start(), "start");

        print_platform_smoke(afterlight::core::current_build_info(), platform, window);

        require_transition(lifecycle.request_stop(), "request stop");
    }

    require_transition(lifecycle.shutdown(), "shutdown");

    return EXIT_SUCCESS;
}

int run_vulkan(afterlight::core::ApplicationLifecycle& lifecycle, bool smoke)
{
    {
        afterlight::platform::PlatformContext platform;

        afterlight::platform::Window window{{
            .title = "Afterlight - The Last Observatory",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .hidden = smoke,
            .vulkan = true,
        }};

        require_transition(lifecycle.start(), "start");

        const afterlight::core::BuildInfo build = afterlight::core::current_build_info();

        afterlight::graphics::vulkan::VulkanContext vulkan{window};

        afterlight::graphics::vulkan::SwapchainRenderer renderer{
            vulkan,
            window,
            afterlight::observatory::shader_directory,
        };

        if (!smoke)
        {
            print_desktop_device(build, vulkan.device_info());
        }

        const FrameLoopResult result = run_frame_loop(window, renderer, smoke);

        if (smoke)
        {
            print_frame_smoke(build,
                              vulkan.device_info(),
                              renderer.info(),
                              renderer.geometry_info(),
                              renderer.scene_binding_info(),
                              result.presented_frames);
        }

        require_transition(lifecycle.request_stop(), "request stop");
    }

    require_transition(lifecycle.shutdown(), "shutdown");

    return EXIT_SUCCESS;
}

int run(const RunOptions& options)
{
    afterlight::core::ApplicationLifecycle lifecycle;

    require_transition(lifecycle.initialize(), "initialize");

    if (options.mode == RunMode::platform_smoke)
    {
        return run_platform_smoke(lifecycle);
    }

    return run_vulkan(lifecycle, options.mode == RunMode::vulkan_smoke);
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
