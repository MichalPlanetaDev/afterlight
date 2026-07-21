#include <afterlight/rhi/resource.hpp>
#include <stdexcept>

namespace afterlight::rhi
{

TextureCreateResult::TextureCreateResult(TextureHandle handle,
                                         ResourceError error,
                                         bool successful) noexcept
    : value_{handle}, error_{error}, successful_{successful}
{
}

TextureCreateResult TextureCreateResult::success(TextureHandle handle) noexcept
{
    return TextureCreateResult{
        handle,
        ResourceError::none,
        true,
    };
}

TextureCreateResult TextureCreateResult::failure(ResourceError error) noexcept
{
    return TextureCreateResult{
        {},
        error,
        false,
    };
}

bool TextureCreateResult::has_value() const noexcept
{
    return successful_;
}

const TextureHandle& TextureCreateResult::operator*() const
{
    if (!successful_)
    {
        throw std::logic_error{"texture creation result has no value"};
    }

    return value_;
}

const TextureHandle* TextureCreateResult::operator->() const
{
    if (!successful_)
    {
        throw std::logic_error{"texture creation result has no value"};
    }

    return &value_;
}

ResourceError TextureCreateResult::error() const noexcept
{
    return error_;
}

TextureCreateResult ResourceRegistry::create_texture(const TextureDesc& descriptor)
{
    if (descriptor.width == 0 || descriptor.height == 0)
    {
        return TextureCreateResult::failure(ResourceError::invalid_extent);
    }

    if (descriptor.format == TextureFormat::undefined)
    {
        return TextureCreateResult::failure(ResourceError::undefined_format);
    }

    if (!free_texture_indices_.empty())
    {
        const std::uint32_t index = free_texture_indices_.back();

        free_texture_indices_.pop_back();

        TextureSlot& slot = texture_slots_[static_cast<std::size_t>(index)];

        slot.descriptor = descriptor;
        slot.occupied = true;

        ++live_texture_count_;

        return TextureCreateResult::success({
            .index = index,
            .generation = slot.generation,
        });
    }

    if (texture_slots_.size() >= static_cast<std::size_t>(TextureHandle::invalid_index))
    {
        return TextureCreateResult::failure(ResourceError::capacity_exhausted);
    }

    const auto index = static_cast<std::uint32_t>(texture_slots_.size());

    texture_slots_.push_back({
        .descriptor = descriptor,
        .generation = 1,
        .occupied = true,
    });

    ++live_texture_count_;

    return TextureCreateResult::success({
        .index = index,
        .generation = 1,
    });
}

bool ResourceRegistry::destroy_texture(TextureHandle handle) noexcept
{
    TextureSlot* slot = resolve(handle);

    if (slot == nullptr)
    {
        return false;
    }

    slot->descriptor = {};
    slot->occupied = false;
    ++slot->generation;

    if (slot->generation == 0)
    {
        slot->generation = 1;
    }

    free_texture_indices_.push_back(handle.index);
    --live_texture_count_;

    return true;
}

bool ResourceRegistry::contains(TextureHandle handle) const noexcept
{
    return resolve(handle) != nullptr;
}

const TextureDesc* ResourceRegistry::descriptor(TextureHandle handle) const noexcept
{
    const TextureSlot* slot = resolve(handle);

    return slot == nullptr ? nullptr : &slot->descriptor;
}

std::size_t ResourceRegistry::live_texture_count() const noexcept
{
    return live_texture_count_;
}

ResourceRegistry::TextureSlot* ResourceRegistry::resolve(TextureHandle handle) noexcept
{
    if (!handle.valid() || static_cast<std::size_t>(handle.index) >= texture_slots_.size())
    {
        return nullptr;
    }

    TextureSlot& slot = texture_slots_[static_cast<std::size_t>(handle.index)];

    if (!slot.occupied || slot.generation != handle.generation)
    {
        return nullptr;
    }

    return &slot;
}

const ResourceRegistry::TextureSlot* ResourceRegistry::resolve(TextureHandle handle) const noexcept
{
    if (!handle.valid() || static_cast<std::size_t>(handle.index) >= texture_slots_.size())
    {
        return nullptr;
    }

    const TextureSlot& slot = texture_slots_[static_cast<std::size_t>(handle.index)];

    if (!slot.occupied || slot.generation != handle.generation)
    {
        return nullptr;
    }

    return &slot;
}

} // namespace afterlight::rhi
