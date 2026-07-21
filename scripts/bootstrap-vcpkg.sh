#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly PROJECT_ROOT
readonly VCPKG_ROOT="$PROJECT_ROOT/.cache/vcpkg"
readonly VCPKG_BASELINE="3ddaad9be959816602453ecb05533f8732464ef4"

mkdir -p "$PROJECT_ROOT/.cache"

if [[ -e "$VCPKG_ROOT" && ! -d "$VCPKG_ROOT/.git" ]]; then
    echo "ERROR: $VCPKG_ROOT exists but is not a Git repository."
    exit 1
fi

if [[ ! -d "$VCPKG_ROOT/.git" ]]; then
    git clone                 --filter=blob:none                 https://github.com/microsoft/vcpkg.git                 "$VCPKG_ROOT"
fi

git -C "$VCPKG_ROOT" remote set-url             origin             https://github.com/microsoft/vcpkg.git

git -C "$VCPKG_ROOT" fetch             --depth 1             origin             "$VCPKG_BASELINE"

git -C "$VCPKG_ROOT" checkout             --detach             "$VCPKG_BASELINE"

"$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

echo "vcpkg ready at $VCPKG_BASELINE"
