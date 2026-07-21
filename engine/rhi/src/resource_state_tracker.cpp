#include <afterlight/rhi/resource_state_tracker.hpp>

namespace afterlight::rhi
{

bool ResourceStateTracker::track(TextureHandle texture, ResourceState initial_state)
{
    if (!texture.valid())
    {
        return false;
    }

    return texture_states_.emplace(key(texture), initial_state).second;
}

bool ResourceStateTracker::forget(TextureHandle texture) noexcept
{
    if (!texture.valid())
    {
        return false;
    }

    return texture_states_.erase(key(texture)) == 1;
}

std::optional<TextureTransition> ResourceStateTracker::transition(TextureHandle texture,
                                                                  ResourceState next_state)
{
    if (!texture.valid())
    {
        return std::nullopt;
    }

    const auto found = texture_states_.find(key(texture));

    if (found == texture_states_.end() || found->second == next_state)
    {
        return std::nullopt;
    }

    const ResourceState previous = found->second;
    found->second = next_state;

    return TextureTransition{
        .texture = texture,
        .before = previous,
        .after = next_state,
    };
}

std::optional<ResourceState> ResourceStateTracker::state(TextureHandle texture) const noexcept
{
    if (!texture.valid())
    {
        return std::nullopt;
    }

    const auto found = texture_states_.find(key(texture));

    if (found == texture_states_.end())
    {
        return std::nullopt;
    }

    return found->second;
}

std::size_t ResourceStateTracker::tracked_texture_count() const noexcept
{
    return texture_states_.size();
}

std::uint64_t ResourceStateTracker::key(TextureHandle texture) noexcept
{
    return (static_cast<std::uint64_t>(texture.generation) << 32U) |
           static_cast<std::uint64_t>(texture.index);
}

} // namespace afterlight::rhi
