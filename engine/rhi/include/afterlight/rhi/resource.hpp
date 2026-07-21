#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace afterlight::rhi
{

enum class TextureFormat : std::uint8_t
{
    undefined,
    bgra8_srgb,
    rgba8_srgb,
    other,
};

enum class ResourceError : std::uint8_t
{
    none,
    invalid_extent,
    undefined_format,
    capacity_exhausted,
};

struct TextureHandle final
{
    static constexpr std::uint32_t invalid_index = std::numeric_limits<std::uint32_t>::max();

    std::uint32_t index{invalid_index};
    std::uint32_t generation{};

    [[nodiscard]] bool valid() const noexcept
    {
        return index != invalid_index && generation != 0;
    }

    friend bool operator==(const TextureHandle&, const TextureHandle&) = default;
};

class TextureCreateResult final
{
public:
    [[nodiscard]] static TextureCreateResult success(TextureHandle handle) noexcept;

    [[nodiscard]] static TextureCreateResult failure(ResourceError error) noexcept;

    [[nodiscard]] bool has_value() const noexcept;

    [[nodiscard]] const TextureHandle& operator*() const;

    [[nodiscard]] const TextureHandle* operator->() const;

    [[nodiscard]] ResourceError error() const noexcept;

private:
    TextureCreateResult(TextureHandle handle, ResourceError error, bool successful) noexcept;

    TextureHandle value_;
    ResourceError error_{ResourceError::none};
    bool successful_{};
};

struct TextureDesc final
{
    std::uint32_t width{};
    std::uint32_t height{};
    TextureFormat format{TextureFormat::undefined};
    bool external{};
};

class ResourceRegistry final
{
public:
    [[nodiscard]] TextureCreateResult create_texture(const TextureDesc& descriptor);

    [[nodiscard]] bool destroy_texture(TextureHandle handle) noexcept;

    [[nodiscard]] bool contains(TextureHandle handle) const noexcept;

    [[nodiscard]] const TextureDesc* descriptor(TextureHandle handle) const noexcept;

    [[nodiscard]] std::size_t live_texture_count() const noexcept;

private:
    struct TextureSlot final
    {
        TextureDesc descriptor;
        std::uint32_t generation{1};
        bool occupied{};
    };

    [[nodiscard]] TextureSlot* resolve(TextureHandle handle) noexcept;

    [[nodiscard]] const TextureSlot* resolve(TextureHandle handle) const noexcept;

    std::vector<TextureSlot> texture_slots_;
    std::vector<std::uint32_t> free_texture_indices_;
    std::size_t live_texture_count_{};
};

} // namespace afterlight::rhi
