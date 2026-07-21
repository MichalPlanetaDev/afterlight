#include <afterlight/platform/platform.hpp>
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
    TestContext test;

    test.expect(afterlight::platform::validate_window_config({}) ==
                    afterlight::platform::WindowConfigError::none,
                "default window configuration is valid");

    test.expect(afterlight::platform::validate_window_config({
                    .title = "",
                }) == afterlight::platform::WindowConfigError::empty_title,
                "empty title is rejected");

    test.expect(afterlight::platform::validate_window_config({
                    .width = 0,
                }) == afterlight::platform::WindowConfigError::invalid_width,
                "non-positive width is rejected");

    test.expect(afterlight::platform::validate_window_config({
                    .height = -1,
                }) == afterlight::platform::WindowConfigError::invalid_height,
                "non-positive height is rejected");

    return test.exit_code();
}
