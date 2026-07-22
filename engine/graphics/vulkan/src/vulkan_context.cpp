#include <afterlight/graphics/vulkan/vulkan_context.hpp>
#include <afterlight/platform/vulkan_surface.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace afterlight::graphics::vulkan
{

namespace
{

constexpr std::string_view validation_layer{"VK_LAYER_KHRONOS_validation"};

[[nodiscard]] std::runtime_error vulkan_failure(std::string_view operation, VkResult result)
{
    return std::runtime_error{std::string{operation} + " failed with VkResult " +
                              std::to_string(static_cast<int>(result))};
}

void require_success(VkResult result, std::string_view operation)
{
    if (result != VK_SUCCESS)
    {
        throw vulkan_failure(operation, result);
    }
}

[[nodiscard]] std::unordered_set<std::string> instance_extensions()
{
    std::uint32_t count = 0;

    require_success(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr),
                    "vkEnumerateInstanceExtensionProperties(count)");

    std::vector<VkExtensionProperties> properties(count);

    require_success(vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.data()),
                    "vkEnumerateInstanceExtensionProperties(data)");

    std::unordered_set<std::string> names;

    for (const VkExtensionProperties& property : properties)
    {
        names.emplace(property.extensionName);
    }

    return names;
}

[[nodiscard]] bool validation_layer_available()
{
    std::uint32_t count = 0;

    require_success(vkEnumerateInstanceLayerProperties(&count, nullptr),
                    "vkEnumerateInstanceLayerProperties(count)");

    std::vector<VkLayerProperties> properties(count);

    require_success(vkEnumerateInstanceLayerProperties(&count, properties.data()),
                    "vkEnumerateInstanceLayerProperties(data)");

    return std::any_of(properties.begin(),
                       properties.end(),
                       [](const VkLayerProperties& property)
                       { return validation_layer == property.layerName; });
}

[[nodiscard]] VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info() noexcept
{
    VkDebugUtilsMessengerCreateInfoEXT info{};

    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    info.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT,
                              VkDebugUtilsMessageTypeFlagsEXT,
                              const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                              void*) noexcept -> VkBool32
    {
        if (callback_data != nullptr && callback_data->pMessage != nullptr)
        {
            std::cerr << "[Vulkan validation] " << callback_data->pMessage << '\n';
        }

        return VK_FALSE;
    };

    return info;
}

[[nodiscard]] bool device_extension_available(VkPhysicalDevice physical_device,
                                              std::string_view extension)
{
    std::uint32_t count = 0;

    require_success(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, nullptr),
                    "vkEnumerateDeviceExtensionProperties(count)");

    std::vector<VkExtensionProperties> properties(count);

    require_success(
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, properties.data()),
        "vkEnumerateDeviceExtensionProperties(data)");

    return std::any_of(properties.begin(),
                       properties.end(),
                       [extension](const VkExtensionProperties& property)
                       { return extension == property.extensionName; });
}

[[nodiscard]] bool has_surface_formats(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    std::uint32_t count = 0;

    const VkResult result =
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr);

    return result == VK_SUCCESS && count > 0;
}

[[nodiscard]] bool has_present_modes(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    std::uint32_t count = 0;

    const VkResult result =
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr);

    return result == VK_SUCCESS && count > 0;
}

[[nodiscard]] std::vector<QueueFamilySupport> queue_family_support(VkPhysicalDevice physical_device,
                                                                   VkSurfaceKHR surface)
{
    std::uint32_t count = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> properties(count);

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties.data());

    std::vector<QueueFamilySupport> support;
    support.reserve(properties.size());

    for (std::uint32_t index = 0; index < count; ++index)
    {
        VkBool32 presentation_supported = VK_FALSE;

        require_success(vkGetPhysicalDeviceSurfaceSupportKHR(
                            physical_device, index, surface, &presentation_supported),
                        "vkGetPhysicalDeviceSurfaceSupportKHR");

        support.push_back({
            .graphics = properties[index].queueCount > 0 &&
                        (properties[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0,
            .presentation = properties[index].queueCount > 0 && presentation_supported == VK_TRUE,
        });
    }

    return support;
}

[[nodiscard]] AdapterCandidate make_candidate(VkPhysicalDevice physical_device,
                                              VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties properties{};

    vkGetPhysicalDeviceProperties(physical_device, &properties);

    VkPhysicalDeviceVulkan13Features features13{};

    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceFeatures2 features{};

    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    features.pNext = &features13;

    vkGetPhysicalDeviceFeatures2(physical_device, &features);

    return {
        .name = properties.deviceName,
        .kind = adapter_kind(properties.deviceType),
        .api_version = properties.apiVersion,
        .max_image_dimension_2d = properties.limits.maxImageDimension2D,
        .swapchain_extension =
            device_extension_available(physical_device, VK_KHR_SWAPCHAIN_EXTENSION_NAME),
        .surface_formats = has_surface_formats(physical_device, surface),
        .present_modes = has_present_modes(physical_device, surface),
        .dynamic_rendering = features13.dynamicRendering == VK_TRUE,
        .synchronization2 = features13.synchronization2 == VK_TRUE,
        .queue_families = queue_family_support(physical_device, surface),
    };
}

[[nodiscard]] std::vector<const char*> extension_pointers(const std::vector<std::string>& names)
{
    std::vector<const char*> pointers;
    pointers.reserve(names.size());

    for (const std::string& name : names)
    {
        pointers.push_back(name.c_str());
    }

    return pointers;
}

} // namespace

VulkanContext::VulkanContext(const platform::Window& window, const VulkanContextOptions& options)
{
    try
    {
        initialize_loader();
        create_instance(options);
        create_debug_messenger();
        create_surface(window);
        select_physical_device();
        create_logical_device();
    }
    catch (...)
    {
        reset();
        throw;
    }
}

VulkanContext::~VulkanContext()
{
    reset();
}

const VulkanDeviceInfo& VulkanContext::device_info() const noexcept
{
    return device_info_;
}

VkInstance VulkanContext::instance() const noexcept
{
    return instance_;
}

VkPhysicalDevice VulkanContext::physical_device() const noexcept
{
    return physical_device_;
}

VkDevice VulkanContext::device() const noexcept
{
    return device_;
}

VkSurfaceKHR VulkanContext::surface() const noexcept
{
    return surface_;
}

VkQueue VulkanContext::graphics_queue() const noexcept
{
    return graphics_queue_;
}

VkQueue VulkanContext::present_queue() const noexcept
{
    return present_queue_;
}

void VulkanContext::initialize_loader()
{
    require_success(volkInitialize(), "volkInitialize");

    std::uint32_t loader_version = VK_API_VERSION_1_0;

    if (vkEnumerateInstanceVersion != nullptr)
    {
        require_success(vkEnumerateInstanceVersion(&loader_version), "vkEnumerateInstanceVersion");
    }

    if (loader_version < VK_API_VERSION_1_3)
    {
        throw std::runtime_error{"Afterlight requires Vulkan 1.3 or newer"};
    }
}

void VulkanContext::create_instance(const VulkanContextOptions& options)
{
    std::vector<std::string> extensions =
        platform::VulkanSurfaceBridge::required_instance_extensions();

    const std::unordered_set<std::string> available = instance_extensions();

    for (const std::string& extension : extensions)
    {
        if (!available.contains(extension))
        {
            throw std::runtime_error{"required Vulkan instance extension is unavailable: " +
                                     extension};
        }
    }

    validation_enabled_ = options.enable_validation && validation_layer_available();

    debug_utils_enabled_ =
        validation_enabled_ && available.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    if (debug_utils_enabled_)
    {
        extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    const std::vector<const char*> extension_names = extension_pointers(extensions);

    const char* layer_name = validation_layer.data();

    VkApplicationInfo application_info{};

    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    application_info.pApplicationName = options.application_name.c_str();

    application_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 6, 0);

    application_info.pEngineName = "Afterlight";

    application_info.engineVersion = VK_MAKE_API_VERSION(0, 0, 6, 0);

    application_info.apiVersion = VK_API_VERSION_1_3;

    VkDebugUtilsMessengerCreateInfoEXT debug_info = debug_messenger_create_info();

    VkInstanceCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    create_info.pApplicationInfo = &application_info;

    create_info.enabledExtensionCount = static_cast<std::uint32_t>(extension_names.size());

    create_info.ppEnabledExtensionNames = extension_names.data();

    if (validation_enabled_)
    {
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = &layer_name;
    }

    if (debug_utils_enabled_)
    {
        create_info.pNext = &debug_info;
    }

    require_success(vkCreateInstance(&create_info, nullptr, &instance_), "vkCreateInstance");

    volkLoadInstance(instance_);
}

void VulkanContext::create_debug_messenger()
{
    if (!debug_utils_enabled_)
    {
        return;
    }

    if (vkCreateDebugUtilsMessengerEXT == nullptr)
    {
        throw std::runtime_error{"VK_EXT_debug_utils entry point is unavailable"};
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info = debug_messenger_create_info();

    require_success(
        vkCreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr, &debug_messenger_),
        "vkCreateDebugUtilsMessengerEXT");
}

void VulkanContext::create_surface(const platform::Window& window)
{
    surface_ = platform::VulkanSurfaceBridge::create_surface(window, instance_);
}

void VulkanContext::select_physical_device()
{
    std::uint32_t count = 0;

    require_success(vkEnumeratePhysicalDevices(instance_, &count, nullptr),
                    "vkEnumeratePhysicalDevices(count)");

    if (count == 0)
    {
        throw std::runtime_error{"no Vulkan physical devices were found"};
    }

    std::vector<VkPhysicalDevice> physical_devices(count);

    require_success(vkEnumeratePhysicalDevices(instance_, &count, physical_devices.data()),
                    "vkEnumeratePhysicalDevices(data)");

    std::vector<AdapterCandidate> candidates;
    candidates.reserve(physical_devices.size());

    for (VkPhysicalDevice physical_device : physical_devices)
    {
        candidates.push_back(make_candidate(physical_device, surface_));
    }

    const std::optional<AdapterSelection> selection = select_adapter(candidates);

    if (!selection.has_value())
    {
        throw std::runtime_error{
            "no Vulkan 1.3 device supports presentation, dynamic rendering and synchronization2"};
    }

    physical_device_ = physical_devices[selection->candidate_index];

    VkPhysicalDeviceProperties properties{};

    vkGetPhysicalDeviceProperties(physical_device_, &properties);

    device_info_ = {
        .name = properties.deviceName,
        .kind = adapter_kind(properties.deviceType),
        .api_version = properties.apiVersion,
        .driver_version = properties.driverVersion,
        .vendor_id = properties.vendorID,
        .device_id = properties.deviceID,
        .graphics_queue_family = selection->graphics_queue_family,
        .present_queue_family = selection->present_queue_family,
        .validation_enabled = validation_enabled_,
    };
}

void VulkanContext::create_logical_device()
{
    constexpr float queue_priority = 1.0F;

    std::array<std::uint32_t, 2> family_indices{
        device_info_.graphics_queue_family,
        device_info_.present_queue_family,
    };

    const std::size_t queue_info_count = family_indices[0] == family_indices[1] ? 1 : 2;

    std::array<VkDeviceQueueCreateInfo, 2> queue_create_infos{};

    for (std::size_t index = 0; index < queue_info_count; ++index)
    {
        queue_create_infos[index].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

        queue_create_infos[index].queueFamilyIndex = family_indices[index];

        queue_create_infos[index].queueCount = 1;

        queue_create_infos[index].pQueuePriorities = &queue_priority;
    }

    constexpr std::array<const char*, 1> device_extensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceVulkan13Features features13{};

    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkDeviceCreateInfo create_info{};

    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    create_info.pNext = &features13;

    create_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_info_count);

    create_info.pQueueCreateInfos = queue_create_infos.data();

    create_info.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size());

    create_info.ppEnabledExtensionNames = device_extensions.data();

    require_success(vkCreateDevice(physical_device_, &create_info, nullptr, &device_),
                    "vkCreateDevice");

    volkLoadDevice(device_);

    vkGetDeviceQueue(device_, device_info_.graphics_queue_family, 0, &graphics_queue_);

    vkGetDeviceQueue(device_, device_info_.present_queue_family, 0, &present_queue_);

    if (graphics_queue_ == VK_NULL_HANDLE || present_queue_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error{"Vulkan device queues were not created"};
    }
}

void VulkanContext::reset() noexcept
{
    if (device_ != VK_NULL_HANDLE)
    {
        if (vkDeviceWaitIdle != nullptr)
        {
            static_cast<void>(vkDeviceWaitIdle(device_));
        }

        vkDestroyDevice(device_, nullptr);

        device_ = VK_NULL_HANDLE;
        graphics_queue_ = VK_NULL_HANDLE;
        present_queue_ = VK_NULL_HANDLE;
    }

    if (surface_ != VK_NULL_HANDLE)
    {
        platform::VulkanSurfaceBridge::destroy_surface(instance_, surface_);

        surface_ = VK_NULL_HANDLE;
    }

    if (debug_messenger_ != VK_NULL_HANDLE && vkDestroyDebugUtilsMessengerEXT != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);

        debug_messenger_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);

        instance_ = VK_NULL_HANDLE;
    }

    physical_device_ = VK_NULL_HANDLE;
}

std::string format_api_version(std::uint32_t version)
{
    return std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
           std::to_string(VK_API_VERSION_MINOR(version)) + "." +
           std::to_string(VK_API_VERSION_PATCH(version));
}

} // namespace afterlight::graphics::vulkan
