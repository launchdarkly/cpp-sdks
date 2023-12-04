LaunchDarkly Server-Side SDK - Redis Source for C/C++
===================================

### ⚠️ This repository contains alpha software and should not be considered ready for production use while this message is visible.

### Breaking changes may occur.

[![Actions Status](https://github.com/launchdarkly/cpp-sdks/actions/workflows/server-redis.yml/badge.svg)](https://github.com/launchdarkly/cpp-sdks/actions/workflows/server.yml)
[![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)](https://launchdarkly.github.io/cpp-sdks/libs/server-sdk-redis-source/docs/html/)

The LaunchDarkly Server-Side SDK Redis Source for C/C++ is designed for use with the Server-Side SDK.

This component allows the Server-Side SDK to retrieve feature flag configurations from Redis, rather than
from LaunchDarkly.

LaunchDarkly overview
-------------------------
[LaunchDarkly](https://www.launchdarkly.com) is a feature management platform that serves trillions of feature flags
daily to help teams build better software, faster. [Get started](https://docs.launchdarkly.com/docs/getting-started)
using LaunchDarkly today!

[![Twitter Follow](https://img.shields.io/twitter/follow/launchdarkly.svg?style=social&label=Follow&maxAge=2592000)](https://twitter.com/intent/follow?screen_name=launchdarkly)

Compatibility
-------------------------

This component is compatible with POSIX environments (Linux, OS X, BSD) and Windows.

Getting started
---------------

Download a release archive from
the [Github releases](https://github.com/launchdarkly/cpp-sdks/releases?q=cpp-server-redis&expanded=true) for use in
your project.

Refer to the [SDK documentation][reference-guide] for complete instructions on
installing and using the SDK.

### Incorporating the Redis Source

The component can be used via a C++ or C interface and can be incorporated via a static library or shared object. The
static library and shared object each have their own use cases and limitations.

The static library supports both the C++ and C interface. When using the static library, you should ensure that it is
compiled using a compatible configuration and toolchain. For instance, when using MSVC, it needs to be using the same
runtime library.

The C++ API does not have a stable ABI, so if this is important to you, consider using the shared object with the C API.

The shared library (so, DLL, dylib), only supports the C interface.

The examples here are to help with getting started, but generally speaking this component should be incorporated using
your
build system (CMake for instance).

### CMake Usage

When configuring the SDK via CMake, you need to explicitly enable this component (example):

```
cmake -GNinja -D LD_BUILD_REDIS_SUPPORT=ON ..
```

It is disabled by default to avoid pulling in the `redis++` and `hiredis` dependencies that this component is
implemented with.

This will expose the `launchdarkly::server_redis_source` target.

Next, link the target to your executable or library:

```cmake
target_link_libraries(my-target PRIVATE launchdarkly::server_redis_source)
```

This will cause `launchdarkly::server` to be linked as well.

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

[reference-guide]: https://docs.launchdarkly.com/sdk/server-side/c-c--
