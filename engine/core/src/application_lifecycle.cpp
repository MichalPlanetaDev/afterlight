#include <afterlight/core/application_lifecycle.hpp>

namespace afterlight::core
{

LifecycleTransitionResult::LifecycleTransitionResult(bool has_value,
                                                     LifecycleTransitionError error) noexcept
    : has_value_{has_value}, error_{error}
{
}

LifecycleTransitionResult LifecycleTransitionResult::success() noexcept
{
    return {
        true,
        {
            .state = LifecycleState::constructed,
            .action = LifecycleAction::initialize,
        },
    };
}

LifecycleTransitionResult
LifecycleTransitionResult::rejection(LifecycleTransitionError error) noexcept
{
    return {
        false,
        error,
    };
}

bool LifecycleTransitionResult::has_value() const noexcept
{
    return has_value_;
}

LifecycleTransitionError LifecycleTransitionResult::error() const noexcept
{
    return error_;
}

LifecycleState ApplicationLifecycle::state() const noexcept
{
    return state_;
}

LifecycleTransitionResult ApplicationLifecycle::initialize() noexcept
{
    if (state_ != LifecycleState::constructed)
    {
        return reject(LifecycleAction::initialize);
    }

    state_ = LifecycleState::initialized;
    return LifecycleTransitionResult::success();
}

LifecycleTransitionResult ApplicationLifecycle::start() noexcept
{
    if (state_ != LifecycleState::initialized)
    {
        return reject(LifecycleAction::start);
    }

    state_ = LifecycleState::running;
    return LifecycleTransitionResult::success();
}

LifecycleTransitionResult ApplicationLifecycle::request_stop() noexcept
{
    if (state_ != LifecycleState::running)
    {
        return reject(LifecycleAction::request_stop);
    }

    state_ = LifecycleState::stop_requested;
    return LifecycleTransitionResult::success();
}

LifecycleTransitionResult ApplicationLifecycle::fail() noexcept
{
    switch (state_)
    {
        case LifecycleState::constructed:
        case LifecycleState::initialized:
        case LifecycleState::running:
        case LifecycleState::stop_requested:
            state_ = LifecycleState::failed;
            return LifecycleTransitionResult::success();

        case LifecycleState::failed:
        case LifecycleState::shutdown:
            return reject(LifecycleAction::fail);
    }

    return reject(LifecycleAction::fail);
}

LifecycleTransitionResult ApplicationLifecycle::shutdown() noexcept
{
    switch (state_)
    {
        case LifecycleState::initialized:
        case LifecycleState::stop_requested:
        case LifecycleState::failed:
            state_ = LifecycleState::shutdown;
            return LifecycleTransitionResult::success();

        case LifecycleState::constructed:
        case LifecycleState::running:
        case LifecycleState::shutdown:
            return reject(LifecycleAction::shutdown);
    }

    return reject(LifecycleAction::shutdown);
}

LifecycleTransitionResult ApplicationLifecycle::reject(LifecycleAction action) const noexcept
{
    return LifecycleTransitionResult::rejection({
        .state = state_,
        .action = action,
    });
}

} // namespace afterlight::core
