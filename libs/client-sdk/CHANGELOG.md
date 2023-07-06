# Changelog

All notable changes to the LaunchDarkly Client-Side SDK for C/C++ will be documented in this file. This project adheres to [Semantic Versioning](https://semver.org).

## [3.0.2](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-client-v3.0.1...launchdarkly-cpp-client-v3.0.2) (2023-06-30)


### Bug Fixes

* Fix compilation with boost 1.82. ([#157](https://github.com/launchdarkly/cpp-sdks/issues/157)) ([868e9a6](https://github.com/launchdarkly/cpp-sdks/commit/868e9a647487fa78b3316d2d8f6b2c6728903b48))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.1.4 to 0.1.5
    * launchdarkly-cpp-common bumped from 0.3.1 to 0.3.2

## [3.0.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-client-v3.0.0...launchdarkly-cpp-client-v3.0.1) (2023-06-08)


### Bug Fixes

* enforce minimum polling interval of 5 minutes ([#144](https://github.com/launchdarkly/cpp-sdks/issues/144)) ([2d60197](https://github.com/launchdarkly/cpp-sdks/commit/2d60197a72624b40088c0cac22d2dda0f30dd7ac))
* ensure x-launchdarkly-tags is sent in event requests ([#145](https://github.com/launchdarkly/cpp-sdks/issues/145)) ([c8b3aee](https://github.com/launchdarkly/cpp-sdks/commit/c8b3aee72b1ca3d33a7f614822c23f2fee6a093a))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.1.3 to 0.1.4
    * launchdarkly-cpp-common bumped from 0.3.0 to 0.3.1

## [3.0.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-client-v0.2.0...launchdarkly-cpp-client-v3.0.0) (2023-06-01)


### Features

* 3.0.0 Client Release ([19cebd9](https://github.com/launchdarkly/cpp-sdks/commit/19cebd9a06fe515986d8199d45c009856ffd06de))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.1.2 to 0.1.3
    * launchdarkly-cpp-common bumped from 0.2.0 to 0.3.0

## [0.2.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-client-v0.1.0...launchdarkly-cpp-client-v0.2.0) (2023-05-31)


### Features

* add AllFlags C binding ([#128](https://github.com/launchdarkly/cpp-sdks/issues/128)) ([9aa0794](https://github.com/launchdarkly/cpp-sdks/commit/9aa07941c1c9d4184f8ff009fccb03db785320c3))
* Add C bindings for data source status. ([#124](https://github.com/launchdarkly/cpp-sdks/issues/124)) ([d175abb](https://github.com/launchdarkly/cpp-sdks/commit/d175abb26fdcdf28700315cdd7347dd1399cbe17))
* Add c bindings for FlagNotifier. ([#119](https://github.com/launchdarkly/cpp-sdks/issues/119)) ([11a7f61](https://github.com/launchdarkly/cpp-sdks/commit/11a7f61d56deb1ee10e73fad134efdb05887f86f))
* add Version method to obtain SDK version ([#122](https://github.com/launchdarkly/cpp-sdks/issues/122)) ([1003117](https://github.com/launchdarkly/cpp-sdks/commit/10031170b30f75fa7d182aab51a36ada5e126250))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.1.1 to 0.1.2

## 0.1.0 (2023-05-24)

This is a prerelease version and is not considered suitable for production.

### Features

* add C bindings for Client type ([#92](https://github.com/launchdarkly/cpp-sdks/issues/92)) ([d2852e7](https://github.com/launchdarkly/cpp-sdks/commit/d2852e72708da72c90e949de8cfcb6f36ee78a23))
* Add client support for streaming. ([#25](https://github.com/launchdarkly/cpp-sdks/issues/25)) ([0e3c1f2](https://github.com/launchdarkly/cpp-sdks/commit/0e3c1f21dc1bf76451284e4e7f7f61cf1c503eb6))
* add StartAsync ([#110](https://github.com/launchdarkly/cpp-sdks/issues/110)) ([a5b19ed](https://github.com/launchdarkly/cpp-sdks/commit/a5b19edbc1690ce8b897a161d982391ff52785b4))
* Add support for basic offline mode. ([#94](https://github.com/launchdarkly/cpp-sdks/issues/94)) ([6f968ed](https://github.com/launchdarkly/cpp-sdks/commit/6f968ede4619cef2263e8bfb23b4e33055952f05))
* Add support for logging configuration. ([#88](https://github.com/launchdarkly/cpp-sdks/issues/88)) ([6516711](https://github.com/launchdarkly/cpp-sdks/commit/651671100570a46135ed37219e2b6b55e2311b42))
* Add support for redirection requests. ([#31](https://github.com/launchdarkly/cpp-sdks/issues/31)) ([ba5c5ae](https://github.com/launchdarkly/cpp-sdks/commit/ba5c5aebe45b5e6bab4fff9b859d83ad2bb58afa))
* Add the ability to persist and restore flag configuration. ([#93](https://github.com/launchdarkly/cpp-sdks/issues/93)) ([c50e0d1](https://github.com/launchdarkly/cpp-sdks/commit/c50e0d15da0c449caade91df33c2a125298904cf))
* Ensure correct shared library exports and build configuration. ([#105](https://github.com/launchdarkly/cpp-sdks/issues/105)) ([75070a6](https://github.com/launchdarkly/cpp-sdks/commit/75070a6db8b2ec5f2103513f9efc8003a26b0079))
* eventsource client ([#1](https://github.com/launchdarkly/cpp-sdks/issues/1)) ([ab2b0fe](https://github.com/launchdarkly/cpp-sdks/commit/ab2b0feb50ef9f607d19c29ed2dd648f3c47b472))
* generate analytic events from evaluations ([#36](https://github.com/launchdarkly/cpp-sdks/issues/36)) ([c62dcf6](https://github.com/launchdarkly/cpp-sdks/commit/c62dcf69673ef2fcae2dc2f2d143cf0b0f15d076))
* implement Client type ([#21](https://github.com/launchdarkly/cpp-sdks/issues/21)) ([10265fd](https://github.com/launchdarkly/cpp-sdks/commit/10265fda24191172145f0f22e9f82321f2e3dc6b))
* implement event delivery  ([#29](https://github.com/launchdarkly/cpp-sdks/issues/29)) ([4de5eaa](https://github.com/launchdarkly/cpp-sdks/commit/4de5eaaccba0556c4990dceb501277472bab4385))
* Implement flag manager. ([#20](https://github.com/launchdarkly/cpp-sdks/issues/20)) ([15199f1](https://github.com/launchdarkly/cpp-sdks/commit/15199f111f30b06b99f4ce642d1a614d46b629d1))
* Implement http/https requests. ([#27](https://github.com/launchdarkly/cpp-sdks/issues/27)) ([853d3ff](https://github.com/launchdarkly/cpp-sdks/commit/853d3ff5a4148a9d3ed933d2a23dc8609c75d36b))
* implement Identify method ([#89](https://github.com/launchdarkly/cpp-sdks/issues/89)) ([6ab8e82](https://github.com/launchdarkly/cpp-sdks/commit/6ab8e82522ae9eadb4a6c0db60b4d867da34c472))
* Implement polling data source. ([#28](https://github.com/launchdarkly/cpp-sdks/issues/28)) ([7ef503b](https://github.com/launchdarkly/cpp-sdks/commit/7ef503bdcafcf203e63f8faf8431f0baf019c2ee))
* Implement streaming data source. ([#17](https://github.com/launchdarkly/cpp-sdks/issues/17)) ([9931b96](https://github.com/launchdarkly/cpp-sdks/commit/9931b96f73847d5a1b4456fd4f463d43dade5c1b))
* Include version in user-agent. ([#98](https://github.com/launchdarkly/cpp-sdks/issues/98)) ([a33daac](https://github.com/launchdarkly/cpp-sdks/commit/a33daac78b5e64c3419a4a97bf29b638b679784c))
* make EvaluationReason use enums for Kind and ErrorKind ([#40](https://github.com/launchdarkly/cpp-sdks/issues/40)) ([c330bb8](https://github.com/launchdarkly/cpp-sdks/commit/c330bb89907932bb4b8076a52be60756f84810a8))
* minimal SDK contract tests ([#52](https://github.com/launchdarkly/cpp-sdks/issues/52)) ([5bcf735](https://github.com/launchdarkly/cpp-sdks/commit/5bcf7359471ed71bba353d6bfdfc0205e83d8313))
* Reorganize code to better facilitate encapsulation. ([#87](https://github.com/launchdarkly/cpp-sdks/issues/87)) ([94f94ae](https://github.com/launchdarkly/cpp-sdks/commit/94f94aee4b8961a3001afd39f936e9c744fd9759))
* replace Encrypted/Plain clients with foxy library ([#39](https://github.com/launchdarkly/cpp-sdks/issues/39)) ([33e92df](https://github.com/launchdarkly/cpp-sdks/commit/33e92df2e970c607bead4a912fc737027750c8fb))
* Support flag change notifications. ([#41](https://github.com/launchdarkly/cpp-sdks/issues/41)) ([24c6cd8](https://github.com/launchdarkly/cpp-sdks/commit/24c6cd81cea678bdb6930600a919b1bc5a698c88))
* Support handling invalid URLs for asio requests. ([#30](https://github.com/launchdarkly/cpp-sdks/issues/30)) ([64b8aaf](https://github.com/launchdarkly/cpp-sdks/commit/64b8aafdbac07fbf2a82f1bb9fde762c63fd79e7))
* Update windows static builds. ([#103](https://github.com/launchdarkly/cpp-sdks/issues/103)) ([5d08380](https://github.com/launchdarkly/cpp-sdks/commit/5d0838099f7a99de49a604a9b5133325959705ff))
* Use object libraries. ([#99](https://github.com/launchdarkly/cpp-sdks/issues/99)) ([1d848e5](https://github.com/launchdarkly/cpp-sdks/commit/1d848e552def961a0468bfb6bab33cb1c4a86d3b))


### Bug Fixes

* include application tags in streaming/polling requests ([#96](https://github.com/launchdarkly/cpp-sdks/issues/96)) ([76647b1](https://github.com/launchdarkly/cpp-sdks/commit/76647b102d2800e7ca866b872d713cf2c3aea28b))
* make passing of LD_NONBLOCKING in C bindings consistent ([#107](https://github.com/launchdarkly/cpp-sdks/issues/107)) ([36f56b5](https://github.com/launchdarkly/cpp-sdks/commit/36f56b5057b465c2afffc212f078ffcd55d33757))
* remove extra call to data_source-&gt;Start() in api.cpp ([#37](https://github.com/launchdarkly/cpp-sdks/issues/37)) ([33458a4](https://github.com/launchdarkly/cpp-sdks/commit/33458a4f6f7558cca6c4bce721b3d70be5d524f5))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.0.0 to 0.1.0
    * launchdarkly-cpp-common bumped from 0.0.0 to 0.1.0
