# Build CURL from source for Windows using MSVC
# This script downloads and builds CURL with static libraries compatible with MSVC
#
# Usage:
#   .\build-curl-windows.ps1 [-Version <version>] [-InstallPrefix <path>]
#
# Parameters:
#   -Version: CURL version to download (default: 8.11.1)
#   -InstallPrefix: Installation directory (default: C:\curl-install)

param(
    [string]$Version = "8.11.1",
    [string]$InstallPrefix = "C:\curl-install"
)

Write-Host "Building CURL $Version from source with MSVC..."

# Download CURL source
$url = "https://curl.se/download/curl-$Version.tar.gz"
$output = "curl-$Version.tar.gz"

Write-Host "Downloading CURL $Version source from $url"
try {
    Invoke-WebRequest -Uri $url -OutFile $output -MaximumRetryCount 3 -RetryIntervalSec 5
} catch {
    Write-Error "Failed to download CURL: $_"
    exit 1
}

# Extract
Write-Host "Extracting..."
try {
    tar -xzf $output
} catch {
    Write-Error "Failed to extract CURL: $_"
    exit 1
}

# Build with CMake (MSVC)
Set-Location "curl-$Version"

Write-Host "Configuring CURL with CMake..."
Write-Host "Install prefix: $InstallPrefix"

try {
    cmake -G "Visual Studio 17 2022" -A x64 `
        -DCMAKE_INSTALL_PREFIX="$InstallPrefix" `
        -DBUILD_SHARED_LIBS=OFF `
        -DCURL_USE_SCHANNEL=ON `
        -DCURL_DISABLE_TESTS=ON `
        -DBUILD_TESTING=OFF `
        -S . -B build

    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
} catch {
    Write-Error "Failed to configure CURL: $_"
    Set-Location ..
    exit 1
}

Write-Host "Building CURL..."
try {
    cmake --build build --config Release --target install

    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed"
    }
} catch {
    Write-Error "Failed to build CURL: $_"
    Set-Location ..
    exit 1
}

Set-Location ..

Write-Host "SUCCESS: CURL installed to $InstallPrefix"
Write-Host ""
Write-Host "To use this CURL installation with the LaunchDarkly C++ SDK:"
Write-Host "  Set CURL_ROOT=$InstallPrefix"
Write-Host "  Set CMAKE_PREFIX_PATH=$InstallPrefix"
Write-Host "  Configure with: -DLD_CURL_NETWORKING=ON"
