#include <afterlight/core/application_lifecycle.hpp>

namespace afterlight::core
{

LifecycleState ApplicationLifecycle::state() const noexcept
{
    return state_;
}

std::expected<void, LifecycleTransitionError> ApplicationLifecycle::initialize() noexcept
{
    if (state_ != LifecycleState::constructed)
    {
        return reject(LifecycleAction::initialize);
    }

    state_ = LifecycleState::initialized;
    return {};
}

std::expected<void, LifecycleTransitionError> ApplicationLifecycle::start() noexcept
{
    if (state_ != LifecycleState::initialized)
    {
        return reject(LifecycleAction::start);
    }

    state_ = LifecycleState::running;
    return {};
}

std::expected<void, LifecycleTransitionError> ApplicationLifecycle::request_stop() noexcept
{
    if (state_ != LifecycleState::running)
    {
        return reject(LifecycleAction::request_stop);
    }

    state_ = LifecycleState::stop_requested;
    return {};
}

std::expected<void, LifecycleTransitionError> ApplicationLifecycle::fail() noexcept
{
    switch (state_)
    {
        case LifecycleState::constructed:
        case LifecycleState::initialized:
        case LifecycleState::running:
        case LifecycleState::stop_requested:
            state_ = LifecycleState::failed;
            return {};

        case LifecycleState::failed:
        case LifecycleState::shutdown:
            return reject(LifecycleAction::fail);
    }

    return reject(LifecycleAction::fail);
}

std::expected<void, LifecycleTransitionError> ApplicationLifecycle::shutdown() noexcept
{
    switch (state_)
    {
        case LifecycleState::initialized:
        case LifecycleState::stop_requested:
        case LifecycleState::failed:
            state_ = LifecycleState::shutdown;
            return {};

        case LifecycleState::constructed:
        case LifecycleState::running:
        case LifecycleState::shutdown:
            return reject(LifecycleAction::shutdown);
    }

    return reject(LifecycleAction::shutdown);
}

std::expected<void, LifecycleTransitionError>
ApplicationLifecycle::reject(LifecycleAction action) const noexcept
{
    return std::unexpected(LifecycleTransitionError{
        .state = state_,
        .action = action,
    });
}

} // namespace afterlight::core
