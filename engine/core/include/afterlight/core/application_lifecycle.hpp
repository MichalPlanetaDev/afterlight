#pragma once

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

class LifecycleTransitionResult final
{
public:
    [[nodiscard]] static LifecycleTransitionResult success() noexcept;

    [[nodiscard]] static LifecycleTransitionResult
    rejection(LifecycleTransitionError error) noexcept;

    [[nodiscard]] bool has_value() const noexcept;

    [[nodiscard]] LifecycleTransitionError error() const noexcept;

private:
    LifecycleTransitionResult(bool has_value, LifecycleTransitionError error) noexcept;

    bool has_value_;
    LifecycleTransitionError error_;
};

class ApplicationLifecycle final
{
public:
    [[nodiscard]] LifecycleState state() const noexcept;

    [[nodiscard]] LifecycleTransitionResult initialize() noexcept;

    [[nodiscard]] LifecycleTransitionResult start() noexcept;

    [[nodiscard]] LifecycleTransitionResult request_stop() noexcept;

    [[nodiscard]] LifecycleTransitionResult fail() noexcept;

    [[nodiscard]] LifecycleTransitionResult shutdown() noexcept;

private:
    [[nodiscard]] LifecycleTransitionResult reject(LifecycleAction action) const noexcept;

    LifecycleState state_{LifecycleState::constructed};
};

} // namespace afterlight::core
