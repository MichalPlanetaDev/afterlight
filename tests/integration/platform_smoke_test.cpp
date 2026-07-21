#include <afterlight/platform/platform.hpp>
#include <cstdlib>
#include <exception>
#include <iostream>

int main()
{
    try
    {
        afterlight::platform::PlatformContext platform{{
            .video_driver = "dummy",
        }};

        if (platform.video_driver() != "dummy")
        {
            std::cerr << "FAIL: SDL dummy driver was not selected" << '\n';

            return EXIT_FAILURE;
        }

        afterlight::platform::Window window{{
            .title = "Afterlight Platform Smoke Test",
            .width = 640,
            .height = 360,
            .resizable = true,
            .hidden = true,
        }};

        const afterlight::platform::WindowSize size = window.size();

        if (size.width != 640 || size.height != 360)
        {
            std::cerr << "FAIL: unexpected dummy window size" << '\n';

            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';

        return EXIT_FAILURE;
    }
}
