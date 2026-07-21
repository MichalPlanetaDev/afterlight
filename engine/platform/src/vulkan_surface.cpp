#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>
#include <afterlight/platform/vulkan_surface.hpp>
#include <cstddef>
#include <stdexcept>

namespace afterlight::platform
{

std::vector<std::string> VulkanSurfaceBridge::required_instance_extensions()
{
    Uint32 extension_count = 0;

    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);

    if (extensions == nullptr)
    {
        throw std::runtime_error{std::string{"SDL_Vulkan_GetInstanceExtensions: "} +
                                 SDL_GetError()};
    }

    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(extension_count));

    for (Uint32 index = 0; index < extension_count; ++index)
    {
        result.emplace_back(extensions[index]);
    }

    return result;
}

VkSurfaceKHR VulkanSurfaceBridge::create_surface(const Window& window, VkInstance instance)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    if (!SDL_Vulkan_CreateSurface(window.window_, instance, nullptr, &surface))
    {
        throw std::runtime_error{std::string{"SDL_Vulkan_CreateSurface: "} + SDL_GetError()};
    }

    return surface;
}

void VulkanSurfaceBridge::destroy_surface(VkInstance instance, VkSurfaceKHR surface) noexcept
{
    if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE)
    {
        SDL_Vulkan_DestroySurface(instance, surface, nullptr);
    }
}

} // namespace afterlight::platform
