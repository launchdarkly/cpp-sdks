LaunchDarkly Server-Side SDK for C/C++
===================================

[![Actions Status](https://github.com/launchdarkly/cpp-sdks/actions/workflows/server.yml/badge.svg)](https://github.com/launchdarkly/cpp-sdks/actions/workflows/server.yml)
[![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)](https://launchdarkly.github.io/cpp-sdks/libs/server-sdk/docs/html/)

The LaunchDarkly Server-Side SDK for C/C++ is designed primarily for use in multi-user systems such as web servers
and applications. It follows the server-side LaunchDarkly model for multi-user contexts.
It is not intended for use in desktop and embedded systems applications.

For using LaunchDarkly in client-side C/C++ applications, refer to our [Client-Side C/C++ SDK](../client-sdk/README.md).

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
the [Github releases](https://github.com/launchdarkly/cpp-sdks/releases?q=cpp-server&expanded=true) for use in your
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
g++ -I path_to_the_sdk_install/include -O3 -std=c++17 -Llib -fPIE -g main.cpp path_to_the_sdk_install/lib/liblaunchdarkly-cpp-server.a -lpthread -lstdc++ -lcrypto -lssl -lboost_json -lboost_url
```

Example of basic compilation using the C API with a static library using msvc:

```shell
cl /I include /Fe: hello.exe main.cpp /link lib/launchdarkly-cpp-server.lib
```

The shared library (so, DLL, dylib), only supports the C interface. The shared object does not require you to have Boost
or OpenSSL available when linking the shared object to your project.

Example of basic compilation using the C API with a shared library using gcc:

```shell
gcc -I $(pwd)/include -Llib -fPIE -g main.c liblaunchdarkly-cpp-server.so
```

The examples here are to help with getting started, but generally speaking the SDK should be incorporated using your
build system (CMake for instance).

### CMake Usage

First, add the SDK to your project:

```cmake
add_subdirectory(path-to-sdk-repo)
```

Currently `find_package` is not yet supported.

This will expose the `launchdarkly::server` target. Next, link the target to your executable or library:

```cmake
target_link_libraries(my-target PRIVATE launchdarkly::server)
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

Validating SDK packages with the SLSA framework
-------

LaunchDarkly uses the [SLSA framework](https://slsa.dev/spec/v1.0/about) to help developers make their supply chain more secure by ensuring the authenticity of our published SDK packages. As part of [SLSA requirements for level 3 compliance](https://slsa.dev/spec/v1.0/requirements), LaunchDarkly publishes provenance about our SDK package builds using [Github's generic SLSA3 provenance generator](https://github.com/slsa-framework/slsa-github-generator/blob/main/internal/builders/generic/README.md#generation-of-slsa3-provenance-for-arbitrary-projects) for distribution alongside our packages. 

The SLSA framework specifies some [recommendations for verifying build artifacts](https://slsa.dev/spec/v1.0/verifying-artifacts) in their documentation. For our C/C++ SDKs, we recommend the following steps:
- From the Github releases page for the release version, download the provenance file for the OS you're developing on (`OSNAME-multiple-provenance.intoto.jsonl`)
- Check the provenance payload (`payload` in `OSNAME-multiple-provenance.intoto.jsonl`) to verify the authenticity of the build:
  - The payload is wrapped in a DSEE envelope in encoded in base64. To make the provenance human readable, base64 decode the `payload` parameter, e.g.: 
    ```cat OSNAME-multiple-provenance.intoto.jsonl | jq -r '.payload' | base64 -d | jq```
  - Validate that the subject digests in `payload.subject` match the sha256 checksums of the SDK packages for the OS you're developing in
  - Validate that the provenance builder in `payload.predicate.builder` corresponds to Github's generic SLSA3 provenance generator, which ensures SLSA level 3 compliance
  - Check the source commit referenced in `payload.predicate.invocation.configSource` and `payload.predicate.invocation.environment` for:
    - Source repository is a LaunchDarkly-owned repository
    - Commit author is a LaunchDarkly entity
    - (Optional) Code changes in the commit are trustworthy 
  - Check the build workflow file referenced in `payload.predicate.invocation.configSource.entryPoint` for:
    - Build is triggered by a LaunchDarkly-owned repository
    - Build is executed by a LaunchDarkly-owned Github Actions workflow
    - Build steps are trustworthy 
- Check the provenance signature (`signatures[0].cert` in `OSNAME-multiple-provenance.intoto.jsonl`) to verify the authenticity of the build provenance:
  - Certificate issuer is Sigstore 
  - x509 extension field `1.3.6.1.4.1.57264.1.1` (OIDC Issuer) is `https://token.actions.githubusercontent.com`
  - x509 extension field `1.3.6.1.4.1.57264.1.5` (GitHub Workflow Repository) is a LaunchDarkly-owned repository
  - x509 extension field `1.3.6.1.4.1.57264.1.3` (GitHub Workflow SHA) matches the SHA of the source commit in the provenance payload
  
The recommendations above may be adjusted to fit your organization's needs and supply chain security policies. For additional questions, please contact [security@launchdarkly.com](mailto:security@launchdarkly.com).

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
