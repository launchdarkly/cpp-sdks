LaunchDarkly Client-Side SDK for C/C++
===================================

[![Actions Status](https://github.com/launchdarkly/cpp-sdks/actions/workflows/client.yml/badge.svg)](https://github.com/launchdarkly/cpp-sdks/actions/workflows/client.yml)
[![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)](https://launchdarkly.github.io/cpp-sdks/libs/client-sdk/docs/html/)

The LaunchDarkly Client-Side SDK for C/C++ is designed primarily for use in desktop and embedded systems applications.
It follows the client-side LaunchDarkly model for single-user contexts (much like our mobile or JavaScript SDKs). It is
not intended for use in multi-user systems such as web servers and applications.

LaunchDarkly overview
-------------------------
[LaunchDarkly](https://www.launchdarkly.com) is a feature management platform that serves trillions of feature flags
daily to help teams build better software, faster. [Get started](https://docs.launchdarkly.com/docs/getting-started)
using LaunchDarkly today!

[![Twitter Follow](https://img.shields.io/twitter/follow/launchdarkly.svg?style=social&label=Follow&maxAge=2592000)](https://twitter.com/intent/follow?screen_name=launchdarkly)

Compatibility
-------------------------

This version of the LaunchDarkly SDK is compatible with POSIX environments (Linux, OS X, BSD) and Windows.

Getting started
---------------

Download a release archive from
the [Github releases](https://github.com/launchdarkly/cpp-sdks/releases?q=cpp-client&expanded=true) for use in your
project.

Refer to the [SDK documentation][reference-guide] for complete instructions on
installing and using the SDK.

### Incorporating the SDK

The SDK can be used via a C++ or C interface and can be incorporated via a static library or shared object. The static
library and shared object each have their own use cases and limitations.

The static library supports both the C++ and C interface. When using the static library, you should ensure that it is
compiled using a compatible configuration and toolchain. For instance, when using MSVC, it needs to be using the same
runtime library.

Using the static library also requires that you have OpenSSL and Boost available at the time of compilation for your
project.

The C++ API does not have a stable ABI, so if this is important to you, consider using the shared object with the C API.

Example of basic compilation using the C++ API with a static library using gcc:

```shell
g++ -I path_to_the_sdk_install/include -O3 -std=c++17 -Llib -fPIE -g main.cpp path_to_the_sdk_install/lib/liblaunchdarkly-cpp-client.a -lpthread -lstdc++ -lcrypto -lssl -lboost_json -lboost_url
```

Example of basic compilation using the C API with a static library using msvc:

```shell
cl /I include /Fe: hello.exe main.cpp /link lib/launchdarkly-cpp-client.lib
```

The shared library (so, DLL, dylib), only supports the C interface. The shared object does not require you to have Boost
or OpenSSL available when linking the shared object to your project.

Example of basic compilation using the C API with a shared library using gcc:

```shell
gcc -I $(pwd)/include -Llib -fPIE -g main.c liblaunchdarkly-cpp-client.so
```

The examples here are to help with getting started, but generally speaking the SDK should be incorporated using your
build system (CMake for instance).

### CMake Usage

First, add the SDK to your project:

```cmake
add_subdirectory(path-to-sdk-repo)
```

Currently `find_package` is not yet supported.

This will expose the `launchdarkly::client` target. Next, link the target to your executable or library:

```cmake
target_link_libraries(my-target PRIVATE launchdarkly::client)
```

Various CMake options are available to customize the SDK build.

| Option                    | Description                                                                            | Default            | Requires              |
|---------------------------|----------------------------------------------------------------------------------------|--------------------|-----------------------|
| `BUILD_TESTING`           | Coarse-grained switch; turn off to disable all testing and only build the SDK targets. | On                 | N/A                   |
| `LD_BUILD_UNIT_TESTS`     | Whether C++ unit tests are built.                                                      | On                 | `BUILD_TESTING`       |
| `LD_TESTING_SANITIZERS`   | Whether sanitizers should be enabled.                                                  | On                 | `LD_BUILD_UNIT_TESTS` |
| `LD_BUILD_CONTRACT_TESTS` | Whether the contract test service (used in CI) is built.                               | Off                | `BUILD_TESTING`       |
| `LD_BUILD_EXAMPLES`       | Whether example apps (hello world) are built.                                          | On                 | N/A                   |
| `LD_BUILD_SHARED_LIBS`    | Whether the SDK is built as a static or shared library.                                | Off  (static lib)  | N/A                   |
| `LD_STATIC_LINK_OPENSSL`  | Whether OpenSSL should be statically or dynamically linked.                            | On   (static link) | N/A                   |

Example usage:

```bash
cmake -GNinja .. -DLD_BUILD_SHARED_LIBS=On
```

Learn more
-----------

Read our [documentation](https://docs.launchdarkly.com) for in-depth instructions on configuring and using LaunchDarkly.
You can also head straight to
the [complete reference guide for this SDK][reference-guide].

Testing
-------

We run integration tests for all our SDKs using a centralized test harness. This approach gives us the ability to test
for consistency across SDKs, as well as test networking behavior in a long-running application. These tests cover each
method in the SDK, and verify that event sending, flag evaluation, stream reconnection, and other aspects of the SDK all
behave correctly.

Contributing
------------

We encourage pull requests and other contributions from the community. Read
our [contributing guidelines](../../CONTRIBUTING.md) for instructions on how to contribute to this SDK.

About LaunchDarkly
-----------

* LaunchDarkly is a continuous delivery platform that provides feature flags as a service and allows developers to
  iterate quickly and safely. We allow you to easily flag your features and manage them from the LaunchDarkly dashboard.
  With LaunchDarkly, you can:
    * Roll out a new feature to a subset of your users (like a group of users who opt-in to a beta tester group),
      gathering feedback and bug reports from real-world use cases.
    * Gradually roll out a feature to an increasing percentage of users, and track the effect that the feature has on
      key metrics (for instance, how likely is a user to complete a purchase if they have feature A versus feature B?).
    * Turn off a feature that you realize is causing performance problems in production, without needing to re-deploy,
      or even restart the application with a changed configuration file.
    * Grant access to certain features based on user attributes, like payment plan (eg: users on the ‘gold’ plan get
      access to more features than users in the ‘silver’ plan). Disable parts of your application to facilitate
      maintenance, without taking everything offline.
* LaunchDarkly provides feature flag SDKs for a wide variety of languages and technologies.
  Read [our documentation](https://docs.launchdarkly.com/docs) for a complete list.
* Explore LaunchDarkly
    * [launchdarkly.com](https://www.launchdarkly.com/ "LaunchDarkly Main Website") for more information
    * [docs.launchdarkly.com](https://docs.launchdarkly.com/  "LaunchDarkly Documentation") for our documentation and
      SDK reference guides
    * [apidocs.launchdarkly.com](https://apidocs.launchdarkly.com/  "LaunchDarkly API Documentation") for our API
      documentation
    * [blog.launchdarkly.com](https://blog.launchdarkly.com/  "LaunchDarkly Blog Documentation") for the latest product
      updates

[reference-guide]: https://docs.launchdarkly.com/sdk/client-side/c-c--
