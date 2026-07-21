#pragma once

#include <afterlight/platform/platform.hpp>
#include <string>
#include <vector>
#include <volk.h>

namespace afterlight::platform
{

class VulkanSurfaceBridge final
{
public:
    [[nodiscard]] static std::vector<std::string> required_instance_extensions();

    [[nodiscard]] static VkSurfaceKHR create_surface(const Window& window, VkInstance instance);

    static void destroy_surface(VkInstance instance, VkSurfaceKHR surface) noexcept;

private:
    VulkanSurfaceBridge() = delete;
};

} // namespace afterlight::platform
