$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$VcpkgRoot = Join-Path $ProjectRoot ".cache/vcpkg"
$VcpkgBaseline = "3ddaad9be959816602453ecb05533f8732464ef4"

New-Item `
    -ItemType Directory `
    -Force `
    -Path (Join-Path $ProjectRoot ".cache") |
    Out-Null

if ((Test-Path $VcpkgRoot) -and
    -not (Test-Path (Join-Path $VcpkgRoot ".git"))) {
    throw "$VcpkgRoot exists but is not a Git repository."
}

if (-not (Test-Path (Join-Path $VcpkgRoot ".git"))) {
    git clone `
        --filter=blob:none `
        https://github.com/microsoft/vcpkg.git `
        $VcpkgRoot
}

git -C $VcpkgRoot remote set-url `
    origin `
    https://github.com/microsoft/vcpkg.git

git -C $VcpkgRoot fetch `
    --depth 1 `
    origin `
    $VcpkgBaseline

git -C $VcpkgRoot checkout `
    --detach `
    $VcpkgBaseline

& (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics

Write-Host "vcpkg ready at $VcpkgBaseline"
