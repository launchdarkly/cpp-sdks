# LaunchDarkly monorepo for C++ SDKs.

This repository contains LaunchDarkly SDK packages which are written in C++.
This includes shared libraries, used by SDKs and other tools, as well as SDKs.

## Packages

| Readme                                       | issues                                      | tests                                                   | docs                                                                                                                                                                           | latest release                                                                                 |
|----------------------------------------------|---------------------------------------------|---------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|
| [libs/client-sdk](libs/client-sdk/README.md) | [C++ Client SDK][package-cpp-client-issues] | [![Actions Status][cpp-client-ci-badge]][cpp-client-ci] | [![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)](https://launchdarkly.github.io/cpp-sdks/libs/client-sdk/docs/html/) | [On Github](https://github.com/launchdarkly/cpp-sdks/releases?q=%22launchdarkly-cpp-client%22) | 

| Shared packages                                              | issues                                                 | tests                                                         |
|--------------------------------------------------------------|--------------------------------------------------------|---------------------------------------------------------------|
| [libs/common](libs/common/README.md)                         | [Common][package-shared-common-issues]                 | [![Actions Status][shared-common-ci-badge]][shared-common-ci] |
| [libs/server-sent-events](libs/server-sent-events/README.md) | [Common Server-Sent-Events][package-shared-sse-issues] | [![Actions Status][shared-sse-ci-badge]][shared-sse-ci]       |

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
1. Boost version 1.81 or higher
1. OpenSSL

Additional dependencies are fetched via CMake. For details see the `cmake` folder.

GoogleTest is used for testing.

For information on integrating an SDK package please refer to the SDK specific README.

## CMake Usage

Various CMake options are available to customize the client/server SDK builds.

| Option                    | Description                                                                            | Default            | Requires                                  |
|---------------------------|----------------------------------------------------------------------------------------|--------------------|-------------------------------------------|
| `BUILD_TESTING`           | Coarse-grained switch; turn off to disable all testing and only build the SDK targets. | On                 | N/A                                       |
| `LD_BUILD_UNIT_TESTS`     | Whether C++ unit tests are built.                                                      | On                 | `BUILD_TESTING; NOT LD_BUILD_SHARED_LIBS` |
| `LD_TESTING_SANITIZERS`   | Whether sanitizers should be enabled.                                                  | On                 | `LD_BUILD_UNIT_TESTS`                     |
| `LD_BUILD_CONTRACT_TESTS` | Whether the contract test service (used in CI) is built.                               | Off                | `BUILD_TESTING`                           |
| `LD_BUILD_EXAMPLES`       | Whether example apps (hello world) are built.                                          | On                 | N/A                                       |
| `LD_BUILD_SHARED_LIBS`    | Whether the SDKs are built as static or shared libraries.                              | Off  (static lib)  | N/A                                       |
| `LD_DYNAMIC_LINK_OPENSSL` | Whether OpenSSL be dynamically linked.                                                 | Off  (static link) | N/A                                       |

**Note:** _if building the SDKs as shared libraries, then unit tests won't be able to link correctly since the SDK's C++
symbols aren't exposed. To run unit tests, build a static library._

Basic usage example:

```bash
mkdir -p build && cd build
cmake -G"Unix Makefiles" ..
```

Slightly more advanced example - build shared libraries, and don't build any of the testing components:

```bash
mkdir -p build  && cd build
cmake -G"Unix Makefiles" -DLD_BUILD_SHARED_LIBS=On -DBUILD_TESTING=Off ..
```

The example uses `make`, but you might instead use [Ninja](https://ninja-build.org/),
MSVC, [etc.](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)

## LaunchDarkly overview

[LaunchDarkly](https://www.launchdarkly.com) is a feature management platform that serves trillions of feature flags
daily to help teams build better software, faster. [Get started](https://docs.launchdarkly.com/home/getting-started)
using LaunchDarkly today!

[![Twitter Follow](https://img.shields.io/twitter/follow/launchdarkly.svg?style=social&label=Follow&maxAge=2592000)](https://twitter.com/intent/follow?screen_name=launchdarkly)

## Testing

We run integration tests for all our SDKs using a centralized test harness. This approach gives us the ability to test
for consistency across SDKs, as well as test networking behavior in a long-running application. These tests cover each
method in the SDK, and verify that event sending, flag evaluation, stream reconnection, and other aspects of the SDK all
behave correctly.

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

[//]: # 'libs/server-sent-events'

[shared-sse-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/sse.yml/badge.svg

[shared-sse-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/sse.yml

[package-shared-sse-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+shared%2Fsse%22+


[//]: # 'libs/client-sdk'

[cpp-client-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/client.yml/badge.svg

[cpp-client-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/client.yml

[package-cpp-client-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+sdk%2Fclient%22+
