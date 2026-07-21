#pragma once

#include <expected>

namespace afterlight::core
{

enum class LifecycleState
{
    constructed,
    initialized,
    running,
    stop_requested,
    failed,
    shutdown,
};

enum class LifecycleAction
{
    initialize,
    start,
    request_stop,
    fail,
    shutdown,
};

struct LifecycleTransitionError final
{
    LifecycleState state;
    LifecycleAction action;
};

class ApplicationLifecycle final
{
public:
    [[nodiscard]] LifecycleState state() const noexcept;

    [[nodiscard]] std::expected<void, LifecycleTransitionError> initialize() noexcept;

    [[nodiscard]] std::expected<void, LifecycleTransitionError> start() noexcept;

    [[nodiscard]] std::expected<void, LifecycleTransitionError> request_stop() noexcept;

    [[nodiscard]] std::expected<void, LifecycleTransitionError> fail() noexcept;

    [[nodiscard]] std::expected<void, LifecycleTransitionError> shutdown() noexcept;

private:
    [[nodiscard]] std::expected<void, LifecycleTransitionError>
    reject(LifecycleAction action) const noexcept;

    LifecycleState state_{LifecycleState::constructed};
};

} // namespace afterlight::core
