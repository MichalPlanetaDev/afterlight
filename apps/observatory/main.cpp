#include <afterlight/core/application_lifecycle.hpp>
#include <afterlight/core/build_info.hpp>
#include <cstdlib>
#include <iostream>

int main()
{
    afterlight::core::ApplicationLifecycle lifecycle;

    const auto initialized = lifecycle.initialize();

    if (!initialized.has_value())
    {
        return EXIT_FAILURE;
    }

    const auto started = lifecycle.start();

    if (!started.has_value())
    {
        return EXIT_FAILURE;
    }

    const auto stop_requested = lifecycle.request_stop();

    if (!stop_requested.has_value())
    {
        return EXIT_FAILURE;
    }

    const auto shutdown = lifecycle.shutdown();

    if (!shutdown.has_value())
    {
        return EXIT_FAILURE;
    }

    const afterlight::core::BuildInfo info = afterlight::core::current_build_info();

    std::cout << info.product_name << ' ' << info.semantic_version << " | " << info.milestone
              << " | " << info.revision << '\n';

    return EXIT_SUCCESS;
}
