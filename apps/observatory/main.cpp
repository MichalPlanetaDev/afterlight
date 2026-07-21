#include <afterlight/core/application_lifecycle.hpp>
#include <afterlight/core/build_info.hpp>
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

struct RunOptions final
{
    bool smoke_test{};
};

[[nodiscard]] RunOptions parse_options(int argc, char* argv[])
{
    RunOptions options;

    for (int index = 1; index < argc; ++index)
    {
        const std::string_view argument{argv[index]};

        if (argument == "--smoke")
        {
            options.smoke_test = true;
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

int run(const RunOptions& options)
{
    afterlight::core::ApplicationLifecycle lifecycle;

    require_transition(lifecycle.initialize(), "initialize");

    afterlight::platform::PlatformOptions platform_options;

    if (options.smoke_test)
    {
        platform_options.video_driver = "dummy";
    }

    afterlight::platform::PlatformContext platform{platform_options};

    afterlight::platform::Window window{{
        .title = "Afterlight - The Last Observatory",
        .width = 1280,
        .height = 720,
        .resizable = true,
        .hidden = options.smoke_test,
    }};

    require_transition(lifecycle.start(), "start");

    const afterlight::core::BuildInfo build = afterlight::core::current_build_info();

    const afterlight::platform::WindowSize initial_size = window.size();

    std::cout << build.product_name << ' ' << build.semantic_version
              << " | platform=" << platform.video_driver() << " | window=" << initial_size.width
              << 'x' << initial_size.height << '\n';

    bool running = true;
    std::uint32_t frame_count = 0;
    const std::uint32_t frame_limit = options.smoke_test ? 2U : 0U;

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
