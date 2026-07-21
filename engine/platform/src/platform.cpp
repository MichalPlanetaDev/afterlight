#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <afterlight/platform/platform.hpp>
#include <stdexcept>
#include <utility>

namespace afterlight::platform
{

namespace
{

[[nodiscard]] std::runtime_error sdl_failure(std::string_view operation)
{
    std::string message{operation};
    message.append(": ");
    message.append(SDL_GetError());

    return std::runtime_error{message};
}

[[nodiscard]] SDL_WindowFlags window_flags(const WindowConfig& config) noexcept
{
    SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;

    if (config.resizable)
    {
        flags |= SDL_WINDOW_RESIZABLE;
    }

    if (config.hidden)
    {
        flags |= SDL_WINDOW_HIDDEN;
    }

    if (config.vulkan)
    {
        flags |= SDL_WINDOW_VULKAN;
    }

    return flags;
}

} // namespace

WindowConfigError validate_window_config(const WindowConfig& config) noexcept
{
    if (config.title.empty())
    {
        return WindowConfigError::empty_title;
    }

    if (config.width <= 0)
    {
        return WindowConfigError::invalid_width;
    }

    if (config.height <= 0)
    {
        return WindowConfigError::invalid_height;
    }

    return WindowConfigError::none;
}

std::string_view to_string(WindowConfigError error) noexcept
{
    switch (error)
    {
        case WindowConfigError::none:
            return "none";

        case WindowConfigError::empty_title:
            return "window title cannot be empty";

        case WindowConfigError::invalid_width:
            return "window width must be positive";

        case WindowConfigError::invalid_height:
            return "window height must be positive";
    }

    return "unknown window configuration error";
}

PlatformContext::PlatformContext(const PlatformOptions& options)
{
    if (!options.video_driver.empty() &&
        !SDL_SetHint(SDL_HINT_VIDEO_DRIVER, options.video_driver.c_str()))
    {
        throw sdl_failure("SDL_SetHint(SDL_HINT_VIDEO_DRIVER)");
    }

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        throw sdl_failure("SDL_InitSubSystem(SDL_INIT_VIDEO)");
    }
}

PlatformContext::~PlatformContext()
{
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

std::string_view PlatformContext::video_driver() const noexcept
{
    const char* driver = SDL_GetCurrentVideoDriver();

    return driver == nullptr ? std::string_view{} : std::string_view{driver};
}

Window::Window(const WindowConfig& config)
{
    const WindowConfigError validation_error = validate_window_config(config);

    if (validation_error != WindowConfigError::none)
    {
        throw std::invalid_argument{std::string{to_string(validation_error)}};
    }

    window_ =
        SDL_CreateWindow(config.title.c_str(), config.width, config.height, window_flags(config));

    if (window_ == nullptr)
    {
        throw sdl_failure("SDL_CreateWindow");
    }
}

Window::~Window()
{
    destroy();
}

Window::Window(Window&& other) noexcept : window_{std::exchange(other.window_, nullptr)} {}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        window_ = std::exchange(other.window_, nullptr);
    }

    return *this;
}

WindowSize Window::size() const
{
    int width = 0;
    int height = 0;

    if (!SDL_GetWindowSize(window_, &width, &height))
    {
        throw sdl_failure("SDL_GetWindowSize");
    }

    return {
        .width = width,
        .height = height,
    };
}

WindowSize Window::pixel_size() const
{
    int width = 0;
    int height = 0;

    if (!SDL_GetWindowSizeInPixels(window_, &width, &height))
    {
        throw sdl_failure("SDL_GetWindowSizeInPixels");
    }

    return {
        .width = width,
        .height = height,
    };
}

bool Window::poll_event(PlatformEvent& event) const noexcept
{
    SDL_Event sdl_event{};
    const SDL_WindowID window_id = SDL_GetWindowID(window_);

    while (SDL_PollEvent(&sdl_event))
    {
        if (sdl_event.type == SDL_EVENT_QUIT)
        {
            event = {
                .type = PlatformEventType::quit_requested,
            };

            return true;
        }

        if (sdl_event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
            sdl_event.window.windowID == window_id)
        {
            event = {
                .type = PlatformEventType::window_close_requested,
            };

            return true;
        }

        if (sdl_event.type == SDL_EVENT_WINDOW_RESIZED && sdl_event.window.windowID == window_id)
        {
            event = {
                .type = PlatformEventType::window_resized,
                .width = sdl_event.window.data1,
                .height = sdl_event.window.data2,
            };

            return true;
        }
    }

    return false;
}

void Window::destroy() noexcept
{
    if (window_ != nullptr)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

} // namespace afterlight::platform
