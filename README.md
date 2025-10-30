# LaunchDarkly monorepo for C++ SDKs.

This repository contains LaunchDarkly SDK packages which are written in C++.
This includes shared libraries, used by SDKs and other tools, as well as SDKs.

## Packages

| Readme                                                                 | issues                                                           | tests                                                               | docs     (C++)                                                                                                                    | docs (C)                                                                                                                            | latest release                                                                                       |
|------------------------------------------------------------------------|------------------------------------------------------------------|---------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------|
| [libs/client-sdk](libs/client-sdk/README.md)                           | [C++ Client SDK][package-cpp-client-issues]                      | [![Actions Status][cpp-client-ci-badge]][cpp-client-ci]             | [![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)][cpp-client-docs]       | [![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)][cpp-client-c-docs]       | [On Github](https://github.com/launchdarkly/cpp-sdks/releases?q=%22launchdarkly-cpp-client%22)       | 
| [libs/server-sdk](libs/server-sdk/README.md)                           | [C++ Server SDK][package-cpp-server-issues]                      | [![Actions Status][cpp-server-ci-badge]][cpp-server-ci]             | [![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)][cpp-server-docs]       | [![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)][cpp-server-c-docs]       | [On Github](https://github.com/launchdarkly/cpp-sdks/releases?q=%22launchdarkly-cpp-server%22)       |
| [libs/server-sdk-redis-source](libs/server-sdk-redis-source/README.md) | [C++ Server SDK - Redis Source][package-cpp-server-redis-issues] | [![Actions Status][cpp-server-redis-ci-badge]][cpp-server-redis-ci] | [![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)][cpp-server-redis-docs] | [![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)][cpp-server-redis-c-docs] | [On Github](https://github.com/launchdarkly/cpp-sdks/releases?q=%22launchdarkly-cpp-server-redis%22) |

| Shared packages                                              | issues                                                 | tests                                                             |
|--------------------------------------------------------------|--------------------------------------------------------|-------------------------------------------------------------------|
| [libs/common](libs/common/README.md)                         | [Common][package-shared-common-issues]                 | [![Actions Status][shared-common-ci-badge]][shared-common-ci]     |
| [libs/internal](libs/internal/README.md)                     | [Internal][package-shared-internal-issues]             | [![Actions Status][shared-internal-ci-badge]][shared-internal-ci] |
| [libs/server-sent-events](libs/server-sent-events/README.md) | [Common Server-Sent-Events][package-shared-sse-issues] | [![Actions Status][shared-sse-ci-badge]][shared-sse-ci]           |

## Organization

| Directory      | Description                                                                                                                                                                                                                |
|----------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| .github        | Contains CI and release process workflows and actions.                                                                                                                                                                     |
| examples       | Contains examples (hello-world style).                                                                                                                                                                                     |
| contract-tests | Contains contract test service.                                                                                                                                                                                            |
| cmake          | Contains cmake files for importing and configuring external libraries.                                                                                                                                                     |
| libs           | Contains library implementations. This includes libraries shared within the project as well as SDK libraries like the client-sdk.                                                                                          |
| scripts        | Contains scripts used in the release process.                                                                                                                                                                              |
| vendor         | Contains third party source which is directly integrated into the project. Generally third party source is included through CMake using FetchContent, but some libraries require modification specific to this repository. |

## Build Requirements

### Dependencies

1. C++17 and above
1. CMake 3.19 or higher
1. Ninja (if using the included build scripts)
1. Boost version 1.81 or higher (**excluding** Boost 1.83, see note below)
1. OpenSSL

> [!NOTE]   
> Boost 1.83 is not supported due to an incompatibility in Boost.JSON. This issue appears to be resolved
> in versions prior and subsequent to 1.83.

Additional dependencies are fetched via CMake. For details see the `cmake` folder.

GoogleTest is used for testing.

For information on integrating an SDK package please refer to the SDK specific README.

## CMake Options

Various CMake options are available to customize the client/server SDK builds.

| Option                        | Description                                                                                                                                                                                                                                                              | Default                                                | Requires                                  |
|-------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------|-------------------------------------------|
| `BUILD_TESTING`               | Coarse-grained switch; turn off to disable all testing and only build the SDK targets.                                                                                                                                                                                   | On                                                     | N/A                                       |
| `LD_BUILD_UNIT_TESTS`         | Whether C++ unit tests are built.                                                                                                                                                                                                                                        | On                                                     | `BUILD_TESTING; NOT LD_BUILD_SHARED_LIBS` |
| `LD_TESTING_SANITIZERS`       | Whether sanitizers should be enabled.                                                                                                                                                                                                                                    | On                                                     | `LD_BUILD_UNIT_TESTS`                     |
| `LD_BUILD_CONTRACT_TESTS`     | Whether the contract test service (used in CI) is built.                                                                                                                                                                                                                 | Off                                                    | `BUILD_TESTING`                           |
| `LD_BUILD_EXAMPLES`           | Whether example apps (hello world) are built.                                                                                                                                                                                                                            | On                                                     | N/A                                       |
| `LD_BUILD_SHARED_LIBS`        | Whether the SDKs are built as static or shared libraries.                                                                                                                                                                                                                | Off  (static lib)                                      | N/A                                       |
| `LD_BUILD_EXPORT_ALL_SYMBOLS` | Whether to export all symbols in shared libraries. By default, only C API symbols are exported because C++ does not have an ABI. Only use this feature if you understand the risk and requirements. A mismatch in ABI could cause crashes or other unexpected behaviors. | Off  (hidden)                                          | `LD_BUILD_SHARED_LIBS`                    |
| `LD_DYNAMIC_LINK_BOOST`       | If building SDK as shared lib, whether to dynamically link Boost or not. Ensure that the shared boost libraries are present on the target system.                                                                                                                        | On (link boost dynamically when producing shared libs) | `LD_BUILD_SHARED_LIBS`                    |
| `LD_DYNAMIC_LINK_OPENSSL`     | Whether OpenSSL is dynamically linked or not.                                                                                                                                                                                                                            | Off  (static link)                                     | N/A                                       |
| `LD_BUILD_REDIS_SUPPORT`      | Whether the server-side Redis Source is built or not.                                                                                                                                                                                                                    | Off                                                    | N/A                                       |
| `LD_CURL_NETWORKING`          | Enable CURL-based networking for all HTTP requests (SSE streams and event delivery). When OFF, Boost.Beast/Foxy is used instead. CURL must be available as a dependency when this option is ON.                                                                          | Off                                                    | N/A                                       |
| `LD_BUILD_OTEL_SUPPORT`       | Whether the server-side OpenTelemetry integration package is built or not.                                                                                                                                                                                                | Off                                                    | N/A                                       |
| `LD_BUILD_OTEL_FETCH_DEPS`    | When building OpenTelemetry support, automatically fetch and configure OpenTelemetry dependencies via CMake FetchContent. This is useful for local development and CI. When OFF, you must provide OpenTelemetry yourself via `find_package`.                              | Off                                                    | `LD_BUILD_OTEL_SUPPORT`                   |
| `LD_OTEL_CPP_VERSION`         | Specifies the OpenTelemetry C++ SDK version (git tag or commit hash) to fetch when `LD_BUILD_OTEL_FETCH_DEPS` is enabled. Can be set to any valid git reference from the [opentelemetry-cpp repository](https://github.com/open-telemetry/opentelemetry-cpp).            | `ea1f0d61ce5baa5584b097266bf133d1f31e3607` (v1.23.0)  | `LD_BUILD_OTEL_FETCH_DEPS`                |

> [!WARNING]   
> When building shared libraries C++ symbols are not exported, only the C API will be exported. This is because C++ does
> not have a stable ABI. For this reason, the SDK's unit tests are not built in shared library mode.

## Building the SDK from Source

To configure the SDK's CMake project:

```bash
# Use 'make' as the build system.
cmake -B build -S . -G"Unix Makefiles"
```

To pass in config options defined in the table above, add them using `-D`:

```bash
# Use 'make' as the build system, build shared libs, and disable testing.
cmake -B build -S . -G"Unix Makefiles" \
                    -DLD_BUILD_SHARED_LIBS=On \
                    -DBUILD_TESTING=Off ..
```

The example uses `make`, but you might instead use [Ninja](https://ninja-build.org/),
MSVC, [etc.](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)

### Building with CURL Networking

By default, the SDK uses Boost.Beast/Foxy for HTTP networking. To use CURL instead, enable the `LD_CURL_NETWORKING` option:

```bash
cmake -B build -S . -DLD_CURL_NETWORKING=ON
```

> [!WARNING]
> CURL support for the **server-side SDK** is currently **experimental**. It is subject to change and may not be fully tested for production use.

Proxy support does not apply to the redis persistent store implementation for the server-side SDK.

#### CURL Requirements by Platform

**Linux/macOS:**
Install CURL development libraries via your package manager:
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# macOS
brew install curl
```

**Windows (MSVC):**
CURL must be built from source using MSVC to ensure ABI compatibility. A helper script is provided:

```powershell
.\scripts\build-curl-windows.ps1 -Version "8.11.1" -InstallPrefix "C:\curl-install"
```

Then configure the SDK with:
```powershell
cmake -B build -S . -DLD_CURL_NETWORKING=ON `
    -DCURL_ROOT="C:\curl-install" `
    -DCMAKE_PREFIX_PATH="C:\curl-install"
```

The `build-curl-windows.ps1` script:
- Downloads CURL source from curl.se
- Builds static libraries with MSVC using CMake
- Uses Windows Schannel for SSL (no OpenSSL dependency)
- Installs to the specified prefix directory

> [!NOTE]
> Pre-built CURL binaries from curl.se (MinGW builds) are **not** compatible with MSVC and will cause linker errors.

## Incorporating the SDK via `add_subdirectory`

The SDK can be incorporated into an existing application using CMake via `add_subdirectory.`.

```cmake
# Set SDK build options, for example:
set(LD_BUILD_SHARED_LIBS On)

add_subdirectory(path-to-cpp-sdks-repo)
target_link_libraries(your-app PRIVATE launchdarkly::client)
# ... or launchdarkly::server
````

## Incorporating the SDK via `find_package`

> [!WARNING]   
> Preliminary support for `find_package` is available. The package configuration is subject to change, do not expect it
> to be stable as long as this notice is present.

If you've installed the SDK on the build system via `cmake --install`, you can consume it from
the target application like so:

```cmake
find_package(launchdarkly REQUIRED)
target_link_libraries(your-app PRIVATE launchdarkly::launchdarkly-cpp-client)
# ... or launchdarkly::launchdarkly-cpp-server
```

## LaunchDarkly overview

[LaunchDarkly](https://www.launchdarkly.com) is a feature management platform that serves trillions of feature flags
daily to help teams build better software, faster. [Get started](https://docs.launchdarkly.com/home/getting-started)
using LaunchDarkly today!

[![Twitter Follow](https://img.shields.io/twitter/follow/launchdarkly.svg?style=social&label=Follow&maxAge=2592000)](https://twitter.com/intent/follow?screen_name=launchdarkly)

## Testing

We run integration tests for all our SDKs using a centralized test harness. This approach gives us the ability to test
for consistency across SDKs. These tests cover each method in the SDK, and verify that event sending, flag evaluation,
stream reconnection, and other aspects of the SDK all behave correctly.

## Contributing

We encourage pull requests and other contributions from the community. Check out
our [contributing guidelines](CONTRIBUTING.md) for instructions on how to contribute to this SDK.

## About LaunchDarkly

- LaunchDarkly is a continuous delivery platform that provides feature flags as a service and allows developers to
  iterate quickly and safely. We allow you to easily flag your features and manage them from the LaunchDarkly dashboard.
  With LaunchDarkly, you can:
    - Roll out a new feature to a subset of your users (like a group of users who opt-in to a beta tester group),
      gathering feedback and bug reports from real-world use cases.
    - Gradually roll out a feature to an increasing percentage of users, and track the effect that the feature has on
      key metrics (for instance, how likely is a user to complete a purchase if they have feature A versus feature B?).
    - Turn off a feature that you realize is causing performance problems in production, without needing to re-deploy,
      or even restart the application with a changed configuration file.
    - Grant access to certain features based on user attributes, like payment plan (eg: users on the ‘gold’ plan get
      access to more features than users in the ‘silver’ plan). Disable parts of your application to facilitate
      maintenance, without taking everything offline.
- LaunchDarkly provides feature flag SDKs for a wide variety of languages and technologies.
  Read [our documentation](https://docs.launchdarkly.com/sdk) for a complete list.
- Explore LaunchDarkly
    - [launchdarkly.com](https://www.launchdarkly.com/ 'LaunchDarkly Main Website') for more information
    - [docs.launchdarkly.com](https://docs.launchdarkly.com/ 'LaunchDarkly Documentation') for our documentation and SDK
      reference guides
    - [apidocs.launchdarkly.com](https://apidocs.launchdarkly.com/ 'LaunchDarkly API Documentation') for our API
      documentation
    - [blog.launchdarkly.com](https://blog.launchdarkly.com/ 'LaunchDarkly Blog Documentation') for the latest product
      updates

[//]: # 'libs/common'

[shared-common-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/common.yml/badge.svg

[shared-common-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/common.yml

[package-shared-common-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+shared%2Fcommon%22+

[//]: # 'libs/internal'

[shared-internal-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/internal.yml/badge.svg

[shared-internal-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/internal.yml

[package-shared-internal-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+shared%2Finternal%22+


[//]: # 'libs/server-sent-events'

[shared-sse-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/sse.yml/badge.svg

[shared-sse-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/sse.yml

[package-shared-sse-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+shared%2Fsse%22+


[//]: # 'libs/client-sdk'

[cpp-client-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/client.yml/badge.svg

[cpp-client-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/client.yml

[cpp-client-docs]: https://launchdarkly.github.io/cpp-sdks/libs/client-sdk/docs/html/

[cpp-client-c-docs]: https://launchdarkly.github.io/cpp-sdks/libs/client-sdk/docs/html/sdk_8h.html

[package-cpp-client-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+sdk%2Fclient%22+

[//]: # 'libs/server-sdk'

[cpp-server-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/server.yml/badge.svg

[cpp-server-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/server.yml

[cpp-server-docs]: https://launchdarkly.github.io/cpp-sdks/libs/server-sdk/docs/html/

[cpp-server-c-docs]: https://launchdarkly.github.io/cpp-sdks/libs/server-sdk/docs/html/sdk_8h.html

[package-cpp-server-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+sdk%2Fserver%22+

[//]: # 'libs/server-sdk-redis-source'

[cpp-server-redis-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/server-redis.yml/badge.svg

[cpp-server-redis-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/server-redis.yml

[cpp-server-redis-docs]: https://launchdarkly.github.io/cpp-sdks/libs/server-sdk-redis-source/docs/html/

[cpp-server-redis-c-docs]: https://launchdarkly.github.io/cpp-sdks/libs/server-sdk-redis-source/docs/html/redis__source_8h.html

[package-cpp-server-redis-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aopen+is%3Aissue+label%3A%22package%3A+integration%2Fredis%22
