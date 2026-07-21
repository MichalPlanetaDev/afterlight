#include <afterlight/rhi/frame_scheduler.hpp>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{

class TestContext final
{
public:
    void expect(bool condition, std::string_view description)
    {
        if (!condition)
        {
            ++failures_;

            std::cerr << "FAIL: " << description << '\n';
        }
    }

    [[nodiscard]] int exit_code() const noexcept
    {
        return failures_ == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

private:
    int failures_{};
};

} // namespace

int main()
{
    using afterlight::rhi::FrameScheduler;

    TestContext test;
    FrameScheduler scheduler{2};

    test.expect(scheduler.frame_count() == 2 && scheduler.next_slot() == 0,
                "frame slots start at zero");

    const auto first = scheduler.begin_frame();

    test.expect(first.has_value() && first->slot == 0 && scheduler.mark_submitted(*first),
                "the first frame occupies slot zero");

    const auto second = scheduler.begin_frame();

    test.expect(second.has_value() && second->slot == 1 && scheduler.mark_submitted(*second),
                "the second frame occupies slot one");

    test.expect(!scheduler.begin_frame().has_value(), "submitted slots cannot be overwritten");

    test.expect(scheduler.is_submitted(0) && scheduler.mark_completed(0),
                "completed GPU work releases its slot");

    const auto third = scheduler.begin_frame();

    test.expect(third.has_value() && third->slot == 0 && first.has_value() &&
                    third->serial > first->serial,
                "released slots receive a new frame serial");

    test.expect(third.has_value() && scheduler.cancel(*third) && scheduler.next_slot() == 0,
                "cancelled recording returns its slot");

    scheduler.reset();

    test.expect(scheduler.next_slot() == 0 && !scheduler.is_submitted(0) &&
                    !scheduler.is_submitted(1),
                "reset releases all frame slots");

    return test.exit_code();
}
