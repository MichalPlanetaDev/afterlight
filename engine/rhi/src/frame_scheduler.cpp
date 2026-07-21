#include <afterlight/rhi/frame_scheduler.hpp>
#include <algorithm>
#include <stdexcept>

namespace afterlight::rhi
{

FrameScheduler::FrameScheduler(std::uint32_t frame_count)
    : slots_{static_cast<std::size_t>(frame_count)}
{
    if (frame_count == 0)
    {
        throw std::invalid_argument{"frame scheduler requires at least one slot"};
    }
}

std::uint32_t FrameScheduler::next_slot() const noexcept
{
    return next_slot_;
}

std::optional<FrameToken> FrameScheduler::begin_frame() noexcept
{
    Slot* selected = slot(next_slot_);

    if (selected == nullptr || selected->state != SlotState::available)
    {
        return std::nullopt;
    }

    const FrameToken token{
        .slot = next_slot_,
        .serial = next_serial_,
    };

    ++next_serial_;

    if (next_serial_ == 0)
    {
        next_serial_ = 1;
    }

    selected->state = SlotState::recording;
    selected->serial = token.serial;

    next_slot_ = (next_slot_ + 1) % static_cast<std::uint32_t>(slots_.size());

    return token;
}

bool FrameScheduler::mark_submitted(FrameToken token) noexcept
{
    Slot* selected = slot(token.slot);

    if (selected == nullptr || selected->state != SlotState::recording ||
        selected->serial != token.serial)
    {
        return false;
    }

    selected->state = SlotState::submitted;

    return true;
}

bool FrameScheduler::mark_completed(std::uint32_t slot_index) noexcept
{
    Slot* selected = slot(slot_index);

    if (selected == nullptr || selected->state != SlotState::submitted)
    {
        return false;
    }

    selected->state = SlotState::available;

    return true;
}

bool FrameScheduler::cancel(FrameToken token) noexcept
{
    Slot* selected = slot(token.slot);

    if (selected == nullptr || selected->state != SlotState::recording ||
        selected->serial != token.serial)
    {
        return false;
    }

    selected->state = SlotState::available;
    next_slot_ = token.slot;

    return true;
}

bool FrameScheduler::is_submitted(std::uint32_t slot_index) const noexcept
{
    const Slot* selected = slot(slot_index);

    return selected != nullptr && selected->state == SlotState::submitted;
}

std::uint32_t FrameScheduler::frame_count() const noexcept
{
    return static_cast<std::uint32_t>(slots_.size());
}

void FrameScheduler::reset() noexcept
{
    std::ranges::for_each(slots_,
                          [](Slot& frame_slot)
                          {
                              frame_slot.state = SlotState::available;
                              frame_slot.serial = 0;
                          });

    next_slot_ = 0;
}

FrameScheduler::Slot* FrameScheduler::slot(std::uint32_t index) noexcept
{
    if (static_cast<std::size_t>(index) >= slots_.size())
    {
        return nullptr;
    }

    return &slots_[static_cast<std::size_t>(index)];
}

const FrameScheduler::Slot* FrameScheduler::slot(std::uint32_t index) const noexcept
{
    if (static_cast<std::size_t>(index) >= slots_.size())
    {
        return nullptr;
    }

    return &slots_[static_cast<std::size_t>(index)];
}

} // namespace afterlight::rhi
