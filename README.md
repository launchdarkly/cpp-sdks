# LaunchDarkly monorepo for C++ SDKs.

This repository contains LaunchDarkly SDK packages which are written in C++.
This includes shared libraries, used by SDKs and other tools, as well as SDKs.

This repository contains beta software and should not be considered ready for production use while this message is visible.

## Packages

| SDK packages                                          | issues                                      | tests                                                    | docs                     |
|-------------------------------------------------------|---------------------------------------------|----------------------------------------------------------|--------------------------|
| [libs/client-sdk](libs/client-sdk/README.md) | [C++ Client SDK][package-cpp-client-issues] | [![Actions Status][cpp-client-ci-badge]][cpp-client-ci]  |[![Documentation](https://img.shields.io/static/v1?label=GitHub+Pages&message=API+reference&color=00add8)](https://launchdarkly.github.io/cpp-sdks/libs/client-sdk/docs/html/)

| Shared packages                                              | issues                                            | tests                                                                 |
|--------------------------------------------------------------|---------------------------------------------------|-----------------------------------------------------------------------|
| [libs/common](libs/common/README.md)                         | [Common][package-shared-common-issues]            | [![Actions Status][shared-common-ci-badge]][shared-common-ci]         |
| [libs/server-sent-events](libs/server-sent-events/README.md) | [Common Server][package-shared-sdk-server-issues] | [![Actions Status][shared-sse-ci-badge-badge]][shared-sdk-server-ci] |

## Organization

[TODO]

## LaunchDarkly overview

[LaunchDarkly](https://www.launchdarkly.com) is a feature management platform that serves over 100 billion feature flags
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
- LaunchDarkly provides feature flag SDKs for a wide variety of languages and technologies. Check
  out [our documentation](https://docs.launchdarkly.com/sdk) for a complete list.
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
[shared-sse-ci-badge-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/sse.yml/badge.svg
[shared-sdk-server-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/sse.yml
[package-shared-sdk-server-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+shared%2Fsse%22+


[//]: # 'libs/client-sdk'
[cpp-client-ci-badge]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/client.yml/badge.svg
[cpp-client-ci]: https://github.com/launchdarkly/cpp-sdks/actions/workflows/client.yml
[package-cpp-client-issues]: https://github.com/launchdarkly/cpp-sdks/issues?q=is%3Aissue+is%3Aopen+label%3A%22package%3A+sdk%2Fclient%22+
