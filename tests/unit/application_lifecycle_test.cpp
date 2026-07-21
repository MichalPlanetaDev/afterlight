#include <afterlight/core/application_lifecycle.hpp>
#include <cstdlib>
#include <expected>
#include <iostream>
#include <string_view>

namespace
{

class TestContext final
{
public:
    void expect(bool condition, std::string_view description)
    {
        if (condition)
        {
            return;
        }

        ++failures_;
        std::cerr << "FAIL: " << description << '\n';
    }

    [[nodiscard]] int exit_code() const noexcept
    {
        return failures_ == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

private:
    int failures_{};
};

using afterlight::core::ApplicationLifecycle;
using afterlight::core::LifecycleAction;
using afterlight::core::LifecycleState;
using afterlight::core::LifecycleTransitionError;

void expect_success(TestContext& test,
                    const std::expected<void, LifecycleTransitionError>& result,
                    std::string_view description)
{
    test.expect(result.has_value(), description);
}

void expect_rejection(TestContext& test,
                      const std::expected<void, LifecycleTransitionError>& result,
                      LifecycleState expected_state,
                      LifecycleAction expected_action,
                      std::string_view description)
{
    if (result.has_value())
    {
        test.expect(false, description);
        return;
    }

    test.expect(result.error().state == expected_state,
                "rejection reports the current lifecycle state");

    test.expect(result.error().action == expected_action,
                "rejection reports the attempted lifecycle action");
}

void test_complete_lifecycle(TestContext& test)
{
    ApplicationLifecycle lifecycle;

    test.expect(lifecycle.state() == LifecycleState::constructed,
                "a new lifecycle begins in the constructed state");

    const auto initialized = lifecycle.initialize();
    expect_success(test, initialized, "construction can initialize");

    test.expect(lifecycle.state() == LifecycleState::initialized,
                "initialize enters the initialized state");

    const auto started = lifecycle.start();
    expect_success(test, started, "initialized application can start");

    test.expect(lifecycle.state() == LifecycleState::running, "start enters the running state");

    const auto stop_requested = lifecycle.request_stop();
    expect_success(test, stop_requested, "running application can request stop");

    test.expect(lifecycle.state() == LifecycleState::stop_requested,
                "request_stop enters the stop-requested state");

    const auto shutdown = lifecycle.shutdown();
    expect_success(test, shutdown, "stop-requested application can shut down");

    test.expect(lifecycle.state() == LifecycleState::shutdown,
                "shutdown enters the terminal shutdown state");
}

void test_illegal_start_preserves_state(TestContext& test)
{
    ApplicationLifecycle lifecycle;

    const auto result = lifecycle.start();

    expect_rejection(test,
                     result,
                     LifecycleState::constructed,
                     LifecycleAction::start,
                     "start before initialization is rejected");

    test.expect(lifecycle.state() == LifecycleState::constructed,
                "a rejected transition leaves state unchanged");
}

void test_duplicate_initialization_is_rejected(TestContext& test)
{
    ApplicationLifecycle lifecycle;

    const auto first_initialize = lifecycle.initialize();
    expect_success(test, first_initialize, "first initialization succeeds");

    const auto second_initialize = lifecycle.initialize();

    expect_rejection(test,
                     second_initialize,
                     LifecycleState::initialized,
                     LifecycleAction::initialize,
                     "duplicate initialization is rejected");

    test.expect(lifecycle.state() == LifecycleState::initialized,
                "duplicate initialization does not corrupt state");
}

void test_failure_can_shutdown_cleanly(TestContext& test)
{
    ApplicationLifecycle lifecycle;

    const auto initialized = lifecycle.initialize();
    expect_success(test, initialized, "failure path initializes");

    const auto started = lifecycle.start();
    expect_success(test, started, "failure path starts");

    const auto failed = lifecycle.fail();
    expect_success(test, failed, "running application can enter failed state");

    test.expect(lifecycle.state() == LifecycleState::failed, "failure enters the failed state");

    const auto shutdown = lifecycle.shutdown();
    expect_success(test, shutdown, "failed application can shut down");

    test.expect(lifecycle.state() == LifecycleState::shutdown,
                "failed application reaches clean shutdown");
}

void test_initialized_application_can_shutdown(TestContext& test)
{
    ApplicationLifecycle lifecycle;

    const auto initialized = lifecycle.initialize();
    expect_success(test, initialized, "application initializes before early exit");

    const auto shutdown = lifecycle.shutdown();
    expect_success(
        test, shutdown, "initialized application can shut down before entering the run loop");

    test.expect(lifecycle.state() == LifecycleState::shutdown,
                "early shutdown reaches the terminal state");
}

void test_running_application_cannot_skip_stop_request(TestContext& test)
{
    ApplicationLifecycle lifecycle;

    const auto initialized = lifecycle.initialize();
    expect_success(test, initialized, "direct-shutdown test initializes");

    const auto started = lifecycle.start();
    expect_success(test, started, "direct-shutdown test starts");

    const auto shutdown = lifecycle.shutdown();

    expect_rejection(test,
                     shutdown,
                     LifecycleState::running,
                     LifecycleAction::shutdown,
                     "running application must request stop before shutdown");

    test.expect(lifecycle.state() == LifecycleState::running,
                "rejected direct shutdown preserves running state");
}

} // namespace

int main()
{
    TestContext test;

    test_complete_lifecycle(test);
    test_illegal_start_preserves_state(test);
    test_duplicate_initialization_is_rejected(test);
    test_failure_can_shutdown_cleanly(test);
    test_initialized_application_can_shutdown(test);
    test_running_application_cannot_skip_stop_request(test);

    return test.exit_code();
}
