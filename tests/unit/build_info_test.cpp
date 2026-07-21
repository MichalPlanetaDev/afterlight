#include <afterlight/core/build_info.hpp>
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

} // namespace

int main()
{
    TestContext test;
    const afterlight::core::BuildInfo info = afterlight::core::current_build_info();

    test.expect(info.product_name == "Afterlight", "product name is stable");
    test.expect(info.semantic_version == "0.0.0-dev", "development version is explicit");
    test.expect(info.milestone == "Milestone Zero", "milestone is reported");
    test.expect(info.revision == "local", "unreleased revision is not fabricated");

    return test.exit_code();
}
