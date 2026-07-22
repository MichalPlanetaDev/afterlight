#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_ROOT="$(
    cd "$(dirname "${BASH_SOURCE[0]}")/.." &&
    pwd
)"

LOG_ROOT="${AFTERLIGHT_LOG_ROOT:-$PROJECT_ROOT/out/logs/validate-$(date -u +%Y%m%d-%H%M%S)}"
SELECTED_PRESET="${1:-all}"

mkdir -p "$LOG_ROOT"
cd "$PROJECT_ROOT"

pass()
{
    printf '[PASS] %-34s %s\n' "$1" "${2:-}"
}

run()
{
    local name="$1"
    local log="$2"
    shift 2

    local started
    local status
    local elapsed

    started="$(date +%s)"

    set +e
    "$@" >"$log" 2>&1
    status=$?
    set -e

    elapsed="$(( $(date +%s) - started ))"

    if [[ "$status" -ne 0 ]]; then
        printf '\n[FAIL] %s\n\n' "$name"

        grep -Ei \
            'error:|fatal:|failed|failure|undefined reference|tests failed|validation error|VUID-' \
            "$log" |
            tail -n 60 ||
            tail -n 60 "$log"

        printf '\nLog: %s\n' "$log"
        exit "$status"
    fi

    pass "$name" "${elapsed}s"
}

test_preset()
{
    local preset="$1"
    local count

    xvfb-run \
        -a \
        -s "-screen 0 1280x720x24" \
        ctest \
        --preset "$preset"

    count="$(
        ctest \
            --preset "$preset" \
            --show-only=json-v1 |
        python3 -c '
import json
import sys
print(len(json.load(sys.stdin).get("tests", [])))
'
    )"

    [[ "$count" == "17" ]] || {
        echo "Expected 17 tests, found $count"
        return 1
    }
}

platform_smoke()
{
    local output
    local expected

    output="$(
        ./build/linux-clang-debug/apps/observatory/afterlight \
            --smoke
    )"

    expected="Afterlight 0.9.0-dev | platform=dummy | window=1280x720"

    [[ "$output" == "$expected" ]] || {
        echo "Expected: $expected"
        echo "Actual:   $output"
        return 1
    }
}

vulkan_smoke()
{
    local output

    output="$(
        xvfb-run \
            -a \
            -s "-screen 0 1280x720x24" \
            ./build/linux-clang-debug/apps/observatory/afterlight \
            --vulkan-smoke
    )"

    AFTERLIGHT_VULKAN_SMOKE_OUTPUT="$output" \
        python3 - <<'PYTHON'
import os
import re
import sys

output = os.environ[
    "AFTERLIGHT_VULKAN_SMOKE_OUTPUT"
].strip()

pattern = re.compile(
    r"Afterlight 0[.]9[.]0-dev"
    r" [|] backend=vulkan"
    r" [|] device=.+"
    r" [|] presented=3"
    r" [|] extent=[0-9]+x[0-9]+"
    r" [|] images=[0-9]+"
    r" [|] format=[0-9]+"
    r" [|] depth=[0-9]+"
    r" [|] geometry=extruded-observatory-aperture"
    r" [|] vertices=96"
    r" [|] indices=144"
    r" [|] normals=96"
    r" [|] lighting=directional"
    r" [|] uniforms=descriptor-set"
    r" [|] uniform-frames=2"
    r" [|] validation=on"
)

if pattern.fullmatch(output) is None:
    print("Unexpected Vulkan smoke output:")
    print(repr(output))
    sys.exit(1)
PYTHON
}

printf '\nAfterlight Validation\n\n'

run \
    "Source hygiene" \
    "$LOG_ROOT/source-hygiene.log" \
    python3 \
    scripts/check-source-hygiene.py

run \
    "Shell scripts" \
    "$LOG_ROOT/shellcheck.log" \
    shellcheck \
    scripts/bootstrap-vcpkg.sh \
    scripts/install-linux-prerequisites.sh \
    scripts/validate.sh

run \
    "Manifests" \
    "$LOG_ROOT/manifests.log" \
    python3 \
    -c \
    'import json; json.load(open("CMakePresets.json")); json.load(open("vcpkg.json"))'

run \
    "Dependencies" \
    "$LOG_ROOT/dependencies.log" \
    ./scripts/bootstrap-vcpkg.sh

mapfile -t cpp_files < <(
    find apps engine tests \
        -type f \
        \( \
            -name '*.cpp' -o \
            -name '*.hpp' -o \
            -name '*.h' \
        \) |
    sort
)

run \
    "Formatting" \
    "$LOG_ROOT/formatting.log" \
    clang-format \
    --dry-run \
    --Werror \
    "${cpp_files[@]}"

if [[ "$SELECTED_PRESET" == "all" ]]; then
    presets=(
        linux-clang-debug
        linux-clang-sanitize
        linux-clang-analysis
        linux-gcc-debug
    )
else
    presets=("$SELECTED_PRESET")
fi

for preset in "${presets[@]}"; do
    run \
        "$preset configure" \
        "$LOG_ROOT/$preset-configure.log" \
        cmake \
        --preset \
        "$preset"

    run \
        "$preset build" \
        "$LOG_ROOT/$preset-build.log" \
        cmake \
        --build \
        --preset \
        "$preset"

    if [[ "$preset" != "linux-clang-analysis" ]]; then
        run \
            "$preset tests" \
            "$LOG_ROOT/$preset-tests.log" \
            test_preset \
            "$preset"
    fi

    if [[ "$preset" == "linux-clang-debug" ]]; then
        run \
            "Platform smoke" \
            "$LOG_ROOT/platform-smoke.log" \
            platform_smoke

        run \
            "Vulkan smoke" \
            "$LOG_ROOT/vulkan-smoke.log" \
            vulkan_smoke
    fi
done

printf '\nValidation complete\nLogs: %s\n' "$LOG_ROOT"
