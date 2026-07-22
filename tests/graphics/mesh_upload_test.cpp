#include <afterlight/graphics/vulkan/mesh_upload.hpp>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <volk.h>

namespace
{

[[nodiscard]] int failure(std::string_view message)
{
    std::cerr << "mesh upload policy test failed: " << message << '\n';
    return EXIT_FAILURE;
}

} // namespace

int main()
{
    VkPhysicalDeviceMemoryProperties preferred_properties{};

    preferred_properties.memoryTypeCount = 3;

    preferred_properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    preferred_properties.memoryTypes[1].propertyFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    preferred_properties.memoryTypes[2].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const auto preferred_staging = afterlight::graphics::vulkan::choose_mesh_memory_type(
        preferred_properties,
        {
            .type_bits = 0b111U,
            .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        });

    if (!preferred_staging.has_value())
    {
        return failure("coherent staging selection returned no memory type");
    }

    if (preferred_staging->index != 1U)
    {
        return failure("coherent staging selection chose the wrong memory type");
    }

    if (!preferred_staging->coherent)
    {
        return failure("coherent staging selection lost its coherence property");
    }

    const auto device_local = afterlight::graphics::vulkan::choose_mesh_memory_type(
        preferred_properties,
        {
            .type_bits = 0b111U,
            .required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .preferred = 0,
        });

    if (!device_local.has_value())
    {
        return failure("device-local selection returned no memory type");
    }

    if (device_local->index != 2U)
    {
        return failure("device-local selection chose the wrong memory type");
    }

    VkPhysicalDeviceMemoryProperties fallback_properties{};

    fallback_properties.memoryTypeCount = 2;

    fallback_properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    fallback_properties.memoryTypes[1].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const auto fallback_staging = afterlight::graphics::vulkan::choose_mesh_memory_type(
        fallback_properties,
        {
            .type_bits = 0b11U,
            .required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        });

    if (!fallback_staging.has_value())
    {
        return failure("non-coherent fallback returned no memory type");
    }

    if (fallback_staging->index != 0U)
    {
        return failure("non-coherent fallback chose the wrong memory type");
    }

    if (fallback_staging->coherent)
    {
        return failure("non-coherent fallback was incorrectly marked coherent");
    }

    const auto unavailable = afterlight::graphics::vulkan::choose_mesh_memory_type(
        fallback_properties,
        {
            .type_bits = 0b01U,
            .required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .preferred = 0,
        });

    if (unavailable.has_value())
    {
        return failure("incompatible memory requirements unexpectedly succeeded");
    }

    return EXIT_SUCCESS;
}
