#pragma once

#include <afterlight/graphics/vulkan/device_selection.hpp>
#include <afterlight/platform/platform.hpp>
#include <cstdint>
#include <string>
#include <volk.h>

namespace afterlight::graphics::vulkan
{

struct VulkanContextOptions final
{
    std::string application_name{"Afterlight"};
    bool enable_validation{true};
};

struct VulkanDeviceInfo final
{
    std::string name;
    AdapterKind kind{AdapterKind::other};
    std::uint32_t api_version{};
    std::uint32_t driver_version{};
    std::uint32_t vendor_id{};
    std::uint32_t device_id{};
    std::uint32_t graphics_queue_family{};
    std::uint32_t present_queue_family{};
    bool validation_enabled{};
};

class VulkanContext final
{
public:
    VulkanContext(const platform::Window& window, const VulkanContextOptions& options = {});

    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    [[nodiscard]] const VulkanDeviceInfo& device_info() const noexcept;

    [[nodiscard]] VkInstance instance() const noexcept;

    [[nodiscard]] VkPhysicalDevice physical_device() const noexcept;

    [[nodiscard]] VkDevice device() const noexcept;

    [[nodiscard]] VkSurfaceKHR surface() const noexcept;

    [[nodiscard]] VkQueue graphics_queue() const noexcept;

    [[nodiscard]] VkQueue present_queue() const noexcept;

private:
    void initialize_loader();

    void create_instance(const VulkanContextOptions& options);

    void create_debug_messenger();

    void create_surface(const platform::Window& window);

    void select_physical_device();
    void create_logical_device();
    void reset() noexcept;

    bool validation_enabled_{};
    bool debug_utils_enabled_{};

    VkInstance instance_{VK_NULL_HANDLE};

    VkDebugUtilsMessengerEXT debug_messenger_{VK_NULL_HANDLE};

    VkSurfaceKHR surface_{VK_NULL_HANDLE};

    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};

    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphics_queue_{VK_NULL_HANDLE};
    VkQueue present_queue_{VK_NULL_HANDLE};

    VulkanDeviceInfo device_info_;
};

[[nodiscard]] std::string format_api_version(std::uint32_t version);

} // namespace afterlight::graphics::vulkan
