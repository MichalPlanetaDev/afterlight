#pragma once

#include <string>
#include <string_view>

struct SDL_Window;

namespace afterlight::platform
{

class VulkanSurfaceBridge;

struct PlatformOptions final
{
    std::string video_driver;
};

struct WindowConfig final
{
    std::string title{"Afterlight - The Last Observatory"};
    int width{1280};
    int height{720};
    bool resizable{true};
    bool hidden{false};
    bool vulkan{false};
};

enum class WindowConfigError
{
    none,
    empty_title,
    invalid_width,
    invalid_height,
};

enum class PlatformEventType
{
    quit_requested,
    window_close_requested,
    window_resized,
};

struct PlatformEvent final
{
    PlatformEventType type;
    int width{};
    int height{};
};

struct WindowSize final
{
    int width;
    int height;
};

[[nodiscard]] WindowConfigError validate_window_config(const WindowConfig& config) noexcept;

[[nodiscard]] std::string_view to_string(WindowConfigError error) noexcept;

class PlatformContext final
{
public:
    explicit PlatformContext(const PlatformOptions& options = {});

    ~PlatformContext();

    PlatformContext(const PlatformContext&) = delete;
    PlatformContext& operator=(const PlatformContext&) = delete;
    PlatformContext(PlatformContext&&) = delete;
    PlatformContext& operator=(PlatformContext&&) = delete;

    [[nodiscard]] std::string_view video_driver() const noexcept;
};

class Window final
{
public:
    explicit Window(const WindowConfig& config);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    [[nodiscard]] WindowSize size() const;
    [[nodiscard]] WindowSize pixel_size() const;

    [[nodiscard]] bool poll_event(PlatformEvent& event) const noexcept;

private:
    friend class VulkanSurfaceBridge;

    void destroy() noexcept;

    SDL_Window* window_{};
};

} // namespace afterlight::platform
