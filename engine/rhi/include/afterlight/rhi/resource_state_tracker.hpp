#pragma once

#include <afterlight/rhi/resource.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace afterlight::rhi
{

enum class ResourceState : std::uint8_t
{
    undefined,
    color_attachment,
    present,
};

struct TextureTransition final
{
    TextureHandle texture;
    ResourceState before{ResourceState::undefined};
    ResourceState after{ResourceState::undefined};
};

class ResourceStateTracker final
{
public:
    [[nodiscard]] bool track(TextureHandle texture, ResourceState initial_state);

    [[nodiscard]] bool forget(TextureHandle texture) noexcept;

    [[nodiscard]] std::optional<TextureTransition> transition(TextureHandle texture,
                                                              ResourceState next_state);

    [[nodiscard]] std::optional<ResourceState> state(TextureHandle texture) const noexcept;

    [[nodiscard]] std::size_t tracked_texture_count() const noexcept;

private:
    [[nodiscard]] static std::uint64_t key(TextureHandle texture) noexcept;

    std::unordered_map<std::uint64_t, ResourceState> texture_states_;
};

} // namespace afterlight::rhi
