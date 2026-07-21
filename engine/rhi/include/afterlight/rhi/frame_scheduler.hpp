#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace afterlight::rhi
{

struct FrameToken final
{
    std::uint32_t slot{};
    std::uint64_t serial{};

    friend bool operator==(const FrameToken&, const FrameToken&) = default;
};

class FrameScheduler final
{
public:
    explicit FrameScheduler(std::uint32_t frame_count);

    [[nodiscard]] std::uint32_t next_slot() const noexcept;

    [[nodiscard]] std::optional<FrameToken> begin_frame() noexcept;

    [[nodiscard]] bool mark_submitted(FrameToken token) noexcept;

    [[nodiscard]] bool mark_completed(std::uint32_t slot) noexcept;

    [[nodiscard]] bool cancel(FrameToken token) noexcept;

    [[nodiscard]] bool is_submitted(std::uint32_t slot) const noexcept;

    [[nodiscard]] std::uint32_t frame_count() const noexcept;

    void reset() noexcept;

private:
    enum class SlotState : std::uint8_t
    {
        available,
        recording,
        submitted,
    };

    struct Slot final
    {
        SlotState state{SlotState::available};
        std::uint64_t serial{};
    };

    [[nodiscard]] Slot* slot(std::uint32_t index) noexcept;

    [[nodiscard]] const Slot* slot(std::uint32_t index) const noexcept;

    std::vector<Slot> slots_;
    std::uint32_t next_slot_{};
    std::uint64_t next_serial_{1};
};

} // namespace afterlight::rhi
