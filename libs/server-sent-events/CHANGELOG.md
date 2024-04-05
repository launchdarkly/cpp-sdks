# Changelog

## [0.3.2](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.3.1...launchdarkly-cpp-sse-client-v0.3.2) (2024-04-05)


### Bug Fixes

* handle service endpoints with custom ports correctly ([#389](https://github.com/launchdarkly/cpp-sdks/issues/389)) ([f0114e3](https://github.com/launchdarkly/cpp-sdks/commit/f0114e304756fcd606537ffd609f398606cb728f))

## [0.3.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.3.0...launchdarkly-cpp-sse-client-v0.3.1) (2024-04-03)


### Bug Fixes

* do not block identify on SSE client shutdown completion ([#384](https://github.com/launchdarkly/cpp-sdks/issues/384)) ([ca270cd](https://github.com/launchdarkly/cpp-sdks/commit/ca270cd873e97c4b609ecd4656c52ee74d4cbebe))

## [0.3.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.2.0...launchdarkly-cpp-sse-client-v0.3.0) (2023-12-04)


### Features

* server-side data system ([#304](https://github.com/launchdarkly/cpp-sdks/issues/304)) ([9a3737d](https://github.com/launchdarkly/cpp-sdks/commit/9a3737d09b1e1e57e5c7e6d30fb0c92f606d284c))

## [0.2.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.1.3...launchdarkly-cpp-sse-client-v0.2.0) (2023-10-13)


### Features

* clean up LD CMake variables & allow for OpenSSL dynamic link ([#255](https://github.com/launchdarkly/cpp-sdks/issues/255)) ([ed23c9a](https://github.com/launchdarkly/cpp-sdks/commit/ed23c9a347665529a09d18111bb9d3b699381728))

## [0.1.3](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.1.2...launchdarkly-cpp-sse-client-v0.1.3) (2023-09-13)


### Bug Fixes

* stream connections longer than 5 minutes are dropped  ([#244](https://github.com/launchdarkly/cpp-sdks/issues/244)) ([e12664f](https://github.com/launchdarkly/cpp-sdks/commit/e12664f830c84c17242fe9f032d570796555f3d1))

## [0.1.2](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.1.1...launchdarkly-cpp-sse-client-v0.1.2) (2023-08-31)


### Bug Fixes

* allow for specification of initial reconnect delay in streaming data source ([#229](https://github.com/launchdarkly/cpp-sdks/issues/229)) ([d1dde79](https://github.com/launchdarkly/cpp-sdks/commit/d1dde79fde80cc32e19cf384140e138ce64ca02b))

## [0.1.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.1.0...launchdarkly-cpp-sse-client-v0.1.1) (2023-06-30)


### Bug Fixes

* Fix compilation with boost 1.82. ([#157](https://github.com/launchdarkly/cpp-sdks/issues/157)) ([868e9a6](https://github.com/launchdarkly/cpp-sdks/commit/868e9a647487fa78b3316d2d8f6b2c6728903b48))

## 0.1.0 (2023-05-24)


### Features

* add basic eventsource-client ([8c31b8f](https://github.com/launchdarkly/cpp-sdks/commit/8c31b8ff0541c41f792b0f0f6316dbc1cd930a17))
* automatic retries in SSE client ([#101](https://github.com/launchdarkly/cpp-sdks/issues/101)) ([be59e19](https://github.com/launchdarkly/cpp-sdks/commit/be59e19010c65300a3a37fc2827b80f644d0be7e))
* Ensure correct shared library exports and build configuration. ([#105](https://github.com/launchdarkly/cpp-sdks/issues/105)) ([75070a6](https://github.com/launchdarkly/cpp-sdks/commit/75070a6db8b2ec5f2103513f9efc8003a26b0079))
* eventsource client ([#1](https://github.com/launchdarkly/cpp-sdks/issues/1)) ([ab2b0fe](https://github.com/launchdarkly/cpp-sdks/commit/ab2b0feb50ef9f607d19c29ed2dd648f3c47b472))
* follow redirects in SSE client ([#104](https://github.com/launchdarkly/cpp-sdks/issues/104)) ([54ce7f9](https://github.com/launchdarkly/cpp-sdks/commit/54ce7f91aaef73519bde74e3847dfb44a31973db))
* foundation of an event processor ([#16](https://github.com/launchdarkly/cpp-sdks/issues/16)) ([356bde1](https://github.com/launchdarkly/cpp-sdks/commit/356bde11a8b2b66578cc435c019e0a549528d560))
* generate analytic events from evaluations ([#36](https://github.com/launchdarkly/cpp-sdks/issues/36)) ([c62dcf6](https://github.com/launchdarkly/cpp-sdks/commit/c62dcf69673ef2fcae2dc2f2d143cf0b0f15d076))
* Implement http/https requests. ([#27](https://github.com/launchdarkly/cpp-sdks/issues/27)) ([853d3ff](https://github.com/launchdarkly/cpp-sdks/commit/853d3ff5a4148a9d3ed933d2a23dc8609c75d36b))
* implement Identify method ([#89](https://github.com/launchdarkly/cpp-sdks/issues/89)) ([6ab8e82](https://github.com/launchdarkly/cpp-sdks/commit/6ab8e82522ae9eadb4a6c0db60b4d867da34c472))
* Implement streaming data source. ([#17](https://github.com/launchdarkly/cpp-sdks/issues/17)) ([9931b96](https://github.com/launchdarkly/cpp-sdks/commit/9931b96f73847d5a1b4456fd4f463d43dade5c1b))
* Reorganize code to better facilitate encapsulation. ([#87](https://github.com/launchdarkly/cpp-sdks/issues/87)) ([94f94ae](https://github.com/launchdarkly/cpp-sdks/commit/94f94aee4b8961a3001afd39f936e9c744fd9759))
* replace Encrypted/Plain clients with foxy library ([#39](https://github.com/launchdarkly/cpp-sdks/issues/39)) ([33e92df](https://github.com/launchdarkly/cpp-sdks/commit/33e92df2e970c607bead4a912fc737027750c8fb))
* update sse-contract-tests to use foxy's server_session ([#43](https://github.com/launchdarkly/cpp-sdks/issues/43)) ([a4f2d63](https://github.com/launchdarkly/cpp-sdks/commit/a4f2d63f02bcaa63c0d04ef609c4f611ccf001c6))
* Update windows static builds. ([#103](https://github.com/launchdarkly/cpp-sdks/issues/103)) ([5d08380](https://github.com/launchdarkly/cpp-sdks/commit/5d0838099f7a99de49a604a9b5133325959705ff))
* Use object libraries. ([#99](https://github.com/launchdarkly/cpp-sdks/issues/99)) ([1d848e5](https://github.com/launchdarkly/cpp-sdks/commit/1d848e552def961a0468bfb6bab33cb1c4a86d3b))


### Bug Fixes

* include query parameters on streaming requests ([#97](https://github.com/launchdarkly/cpp-sdks/issues/97)) ([e6d8314](https://github.com/launchdarkly/cpp-sdks/commit/e6d8314408120f361ed421d5948f0b1a2c9b71ca))
* remove c++20 designated initializers ([#42](https://github.com/launchdarkly/cpp-sdks/issues/42)) ([949962a](https://github.com/launchdarkly/cpp-sdks/commit/949962a642938d2d5ceecc3927c65565d3fbc719))
