# Changelog

## [0.6.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.6.0...launchdarkly-cpp-sse-client-v0.6.1) (2025-11-12)


### Bug Fixes

* Handle missing data field in SSE parsing. ([#503](https://github.com/launchdarkly/cpp-sdks/issues/503)) ([03b1d11](https://github.com/launchdarkly/cpp-sdks/commit/03b1d11adbb6b2725535d6d3ec1044091fd38fe1))

## [0.6.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.5.5...launchdarkly-cpp-sse-client-v0.6.0) (2025-11-03)


### Features

* Add proxy support when using CURL networking. ([c9a6b17](https://github.com/launchdarkly/cpp-sdks/commit/c9a6b17aa7673c7b2b5f984b3e7027153ab1d16c))
* Add support for CURL networking. ([c9a6b17](https://github.com/launchdarkly/cpp-sdks/commit/c9a6b17aa7673c7b2b5f984b3e7027153ab1d16c))


### Bug Fixes

* Handle null payloads. ([#497](https://github.com/launchdarkly/cpp-sdks/issues/497)) ([d12b7a0](https://github.com/launchdarkly/cpp-sdks/commit/d12b7a00b8055ba5aabc8179d7d459d4d28db32f))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-networking bumped from 0.1.0 to 0.2.0

## [0.5.5](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.5.4...launchdarkly-cpp-sse-client-v0.5.5) (2025-01-28)


### Bug Fixes

* Remove type alias import for JSON system_error that was removed in boost 1.87. ([c0ef518](https://github.com/launchdarkly/cpp-sdks/commit/c0ef518b79d50adfea8c9dabb6061d70119d34b6))
* Remove vendored boost file to compile with boost 1.87. ([c0ef518](https://github.com/launchdarkly/cpp-sdks/commit/c0ef518b79d50adfea8c9dabb6061d70119d34b6))
* Update usage of the steady_timer to be compatible with boost 1.87. ([c0ef518](https://github.com/launchdarkly/cpp-sdks/commit/c0ef518b79d50adfea8c9dabb6061d70119d34b6))

## [0.5.4](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.5.3...launchdarkly-cpp-sse-client-v0.5.4) (2024-10-08)


### Bug Fixes

* guard against overflow in sse backoff calculation ([#455](https://github.com/launchdarkly/cpp-sdks/issues/455)) ([a3c1e58](https://github.com/launchdarkly/cpp-sdks/commit/a3c1e5889a1104131b939615bfee65b7645da0f3))

## [0.5.3](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.5.2...launchdarkly-cpp-sse-client-v0.5.3) (2024-10-01)


### Bug Fixes

* improve handling of streaming error state changes/logging ([#439](https://github.com/launchdarkly/cpp-sdks/issues/439)) ([04e7e0e](https://github.com/launchdarkly/cpp-sdks/commit/04e7e0ef64b1933a63ad8d071a0a8f95ce666dc8))

## [0.5.2](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.5.1...launchdarkly-cpp-sse-client-v0.5.2) (2024-07-25)


### Bug Fixes

* Handle exception closing stream for unavailable adapter on windows. ([#427](https://github.com/launchdarkly/cpp-sdks/issues/427)) ([ae0013c](https://github.com/launchdarkly/cpp-sdks/commit/ae0013cc0fa1d186e3d8cc9c624dc9496ca67472))

## [0.5.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.5.0...launchdarkly-cpp-sse-client-v0.5.1) (2024-07-15)


### Bug Fixes

* more helpful error messages for streaming connection failures ([#419](https://github.com/launchdarkly/cpp-sdks/issues/419)) ([6bd21ba](https://github.com/launchdarkly/cpp-sdks/commit/6bd21ba1eafb5f19275935e1f62f7304d4dc69f5))

## [0.5.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.4.0...launchdarkly-cpp-sse-client-v0.5.0) (2024-05-30)


### Features

* specify a custom CA file for TLS peer verification ([#409](https://github.com/launchdarkly/cpp-sdks/issues/409)) ([857dd28](https://github.com/launchdarkly/cpp-sdks/commit/857dd2824f725ee837737130321121595d95d67c))

## [0.4.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-sse-client-v0.3.2...launchdarkly-cpp-sse-client-v0.4.0) (2024-05-13)


### Features

* add ability to skip TLS peer verification ([#399](https://github.com/launchdarkly/cpp-sdks/issues/399)) ([0422d35](https://github.com/launchdarkly/cpp-sdks/commit/0422d355a9af0af5225e8d60cb853f9d5cf0c35f))

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
