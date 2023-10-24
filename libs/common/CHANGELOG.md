# Changelog

### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-sse-client bumped from 0.1.0 to 0.1.1

### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-sse-client bumped from 0.1.1 to 0.1.2

## [0.5.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.4.0...launchdarkly-cpp-common-v0.5.0) (2023-10-23)


### Features

* server-side SDK  ([#160](https://github.com/launchdarkly/cpp-sdks/issues/160)) ([75eece3](https://github.com/launchdarkly/cpp-sdks/commit/75eece3a46870fdb6bf4384c112700558099c4d1))

## [0.4.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.3.7...launchdarkly-cpp-common-v0.4.0) (2023-10-13)


### Features

* clean up LD CMake variables & allow for OpenSSL dynamic link ([#255](https://github.com/launchdarkly/cpp-sdks/issues/255)) ([ed23c9a](https://github.com/launchdarkly/cpp-sdks/commit/ed23c9a347665529a09d18111bb9d3b699381728))

## [0.3.7](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.3.6...launchdarkly-cpp-common-v0.3.7) (2023-10-11)


### Bug Fixes

* treat warnings as errors in CI ([#253](https://github.com/launchdarkly/cpp-sdks/issues/253)) ([7f4f168](https://github.com/launchdarkly/cpp-sdks/commit/7f4f168f47619d7fa8b8952feade485261c69049))

## [0.3.6](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.3.5...launchdarkly-cpp-common-v0.3.6) (2023-09-13)


### Bug Fixes

* stream connections longer than 5 minutes are dropped  ([#244](https://github.com/launchdarkly/cpp-sdks/issues/244)) ([e12664f](https://github.com/launchdarkly/cpp-sdks/commit/e12664f830c84c17242fe9f032d570796555f3d1))

## [0.3.4](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.3.3...launchdarkly-cpp-common-v0.3.4) (2023-08-28)


### Bug Fixes

* initialization of LDFlagListener  ([#218](https://github.com/launchdarkly/cpp-sdks/issues/218)) ([6c263dd](https://github.com/launchdarkly/cpp-sdks/commit/6c263dd9110e4da188a56cabc54f783190e1114c))

## [0.3.3](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.3.2...launchdarkly-cpp-common-v0.3.3) (2023-08-16)


### Bug Fixes

* Fixes required to run with msvc 14.1 (vs2017) ([#195](https://github.com/launchdarkly/cpp-sdks/issues/195)) ([d16b2ea](https://github.com/launchdarkly/cpp-sdks/commit/d16b2ea1131b2a99efcec99b96c90b9384c33dc7))

## [0.3.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.3.0...launchdarkly-cpp-common-v0.3.1) (2023-06-08)


### Bug Fixes

* enforce minimum polling interval of 5 minutes ([#144](https://github.com/launchdarkly/cpp-sdks/issues/144)) ([2d60197](https://github.com/launchdarkly/cpp-sdks/commit/2d60197a72624b40088c0cac22d2dda0f30dd7ac))
* ensure x-launchdarkly-tags is sent in event requests ([#145](https://github.com/launchdarkly/cpp-sdks/issues/145)) ([c8b3aee](https://github.com/launchdarkly/cpp-sdks/commit/c8b3aee72b1ca3d33a7f614822c23f2fee6a093a))

## [0.3.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.2.0...launchdarkly-cpp-common-v0.3.0) (2023-06-01)


### Features

* add AllFlags C binding ([#128](https://github.com/launchdarkly/cpp-sdks/issues/128)) ([9aa0794](https://github.com/launchdarkly/cpp-sdks/commit/9aa07941c1c9d4184f8ff009fccb03db785320c3))
* Add C bindings for data source status. ([#124](https://github.com/launchdarkly/cpp-sdks/issues/124)) ([d175abb](https://github.com/launchdarkly/cpp-sdks/commit/d175abb26fdcdf28700315cdd7347dd1399cbe17))
* Add c bindings for FlagNotifier. ([#119](https://github.com/launchdarkly/cpp-sdks/issues/119)) ([11a7f61](https://github.com/launchdarkly/cpp-sdks/commit/11a7f61d56deb1ee10e73fad134efdb05887f86f))
* Allow for easier creation of contexts from existing contexts. ([#130](https://github.com/launchdarkly/cpp-sdks/issues/130)) ([5e18616](https://github.com/launchdarkly/cpp-sdks/commit/5e18616916dbb2a5ade86b06e17f194b3981fe37))


### Bug Fixes

* rename C iterator bindings to follow new/free pattern ([#129](https://github.com/launchdarkly/cpp-sdks/issues/129)) ([24dff9a](https://github.com/launchdarkly/cpp-sdks/commit/24dff9aebe4626bca02ccc6336ef46f2e41f76c7))

## [0.2.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-common-v0.1.0...launchdarkly-cpp-common-v0.2.0) (2023-05-31)


### Features

* add AllFlags C binding ([#128](https://github.com/launchdarkly/cpp-sdks/issues/128)) ([9aa0794](https://github.com/launchdarkly/cpp-sdks/commit/9aa07941c1c9d4184f8ff009fccb03db785320c3))
* Add C bindings for data source status. ([#124](https://github.com/launchdarkly/cpp-sdks/issues/124)) ([d175abb](https://github.com/launchdarkly/cpp-sdks/commit/d175abb26fdcdf28700315cdd7347dd1399cbe17))
* Add c bindings for FlagNotifier. ([#119](https://github.com/launchdarkly/cpp-sdks/issues/119)) ([11a7f61](https://github.com/launchdarkly/cpp-sdks/commit/11a7f61d56deb1ee10e73fad134efdb05887f86f))
* Allow for easier creation of contexts from existing contexts. ([#130](https://github.com/launchdarkly/cpp-sdks/issues/130)) ([5e18616](https://github.com/launchdarkly/cpp-sdks/commit/5e18616916dbb2a5ade86b06e17f194b3981fe37))


### Bug Fixes

* rename C iterator bindings to follow new/free pattern ([#129](https://github.com/launchdarkly/cpp-sdks/issues/129)) ([24dff9a](https://github.com/launchdarkly/cpp-sdks/commit/24dff9aebe4626bca02ccc6336ef46f2e41f76c7))

## 0.1.0 (2023-05-24)


### Features

* add ApplicationInfo ([#6](https://github.com/launchdarkly/cpp-sdks/issues/6)) ([17899c1](https://github.com/launchdarkly/cpp-sdks/commit/17899c173d319be4a2d096f0ac2212cf9de094cd))
* Add attribute reference support. ([#7](https://github.com/launchdarkly/cpp-sdks/issues/7)) ([2203788](https://github.com/launchdarkly/cpp-sdks/commit/2203788c658cd1548e2285773652b8420c09bc1b))
* add C bindings for Client type ([#92](https://github.com/launchdarkly/cpp-sdks/issues/92)) ([d2852e7](https://github.com/launchdarkly/cpp-sdks/commit/d2852e72708da72c90e949de8cfcb6f36ee78a23))
* add console backend constructor that gets level from env ([#6](https://github.com/launchdarkly/cpp-sdks/issues/6)) ([14f4161](https://github.com/launchdarkly/cpp-sdks/commit/14f4161cd3dea5b32d5a1b5eca320377066e0ea0))
* Add context type, value type, and associated builders. ([#5](https://github.com/launchdarkly/cpp-sdks/issues/5)) ([b6edb69](https://github.com/launchdarkly/cpp-sdks/commit/b6edb6952497eb4171bc8a63506a408a2f85a969))
* add error.hpp and tl::expected library ([#9](https://github.com/launchdarkly/cpp-sdks/issues/9)) ([1b4ddc8](https://github.com/launchdarkly/cpp-sdks/commit/1b4ddc8587ba8311626e2e07ef725d8164f22cb1))
* add event summarizer ([#19](https://github.com/launchdarkly/cpp-sdks/issues/19)) ([3c4845a](https://github.com/launchdarkly/cpp-sdks/commit/3c4845a0066ed65078969dd26f423e14d1e70843))
* add null checks & comments to all commmon C bindings ([#91](https://github.com/launchdarkly/cpp-sdks/issues/91)) ([4e3cf00](https://github.com/launchdarkly/cpp-sdks/commit/4e3cf00c4855c578865698f82e3ed25f59e07908))
* add ServiceEndpoints builder & struct [1/2] ([#2](https://github.com/launchdarkly/cpp-sdks/issues/2)) ([10763e7](https://github.com/launchdarkly/cpp-sdks/commit/10763e77f5ed6a637554c9c3af6564a115b538ce))
* Add support for logging configuration. ([#88](https://github.com/launchdarkly/cpp-sdks/issues/88)) ([6516711](https://github.com/launchdarkly/cpp-sdks/commit/651671100570a46135ed37219e2b6b55e2311b42))
* Add support for redirection requests. ([#31](https://github.com/launchdarkly/cpp-sdks/issues/31)) ([ba5c5ae](https://github.com/launchdarkly/cpp-sdks/commit/ba5c5aebe45b5e6bab4fff9b859d83ad2bb58afa))
* Add the ability to persist and restore flag configuration. ([#93](https://github.com/launchdarkly/cpp-sdks/issues/93)) ([c50e0d1](https://github.com/launchdarkly/cpp-sdks/commit/c50e0d15da0c449caade91df33c2a125298904cf))
* Change builder to allow for better loops. ([#8](https://github.com/launchdarkly/cpp-sdks/issues/8)) ([0b00f28](https://github.com/launchdarkly/cpp-sdks/commit/0b00f283d12512a13d8bcdc288b2dfde845a2673))
* eventsource client ([#1](https://github.com/launchdarkly/cpp-sdks/issues/1)) ([ab2b0fe](https://github.com/launchdarkly/cpp-sdks/commit/ab2b0feb50ef9f607d19c29ed2dd648f3c47b472))
* foundation of an event processor ([#16](https://github.com/launchdarkly/cpp-sdks/issues/16)) ([356bde1](https://github.com/launchdarkly/cpp-sdks/commit/356bde11a8b2b66578cc435c019e0a549528d560))
* generate analytic events from evaluations ([#36](https://github.com/launchdarkly/cpp-sdks/issues/36)) ([c62dcf6](https://github.com/launchdarkly/cpp-sdks/commit/c62dcf69673ef2fcae2dc2f2d143cf0b0f15d076))
* Implement C-binding for Value type. ([#33](https://github.com/launchdarkly/cpp-sdks/issues/33)) ([afb943c](https://github.com/launchdarkly/cpp-sdks/commit/afb943cb3d8a6b214935087fdd147b74a8a38361))
* Implement client flag data model. ([#12](https://github.com/launchdarkly/cpp-sdks/issues/12)) ([ce7ccbc](https://github.com/launchdarkly/cpp-sdks/commit/ce7ccbc7356b2c5a9a9318109041a28524e6f9d2))
* implement Client type ([#21](https://github.com/launchdarkly/cpp-sdks/issues/21)) ([10265fd](https://github.com/launchdarkly/cpp-sdks/commit/10265fda24191172145f0f22e9f82321f2e3dc6b))
* Implement context filtering and JSON serialization. ([#11](https://github.com/launchdarkly/cpp-sdks/issues/11)) ([074c691](https://github.com/launchdarkly/cpp-sdks/commit/074c6914165987522653e100df1b8b0911bb8565))
* Implement data source configuration. ([#18](https://github.com/launchdarkly/cpp-sdks/issues/18)) ([d2cbf8e](https://github.com/launchdarkly/cpp-sdks/commit/d2cbf8ebd049df59742ca2d864e8449a3c4519d6))
* implement event delivery  ([#29](https://github.com/launchdarkly/cpp-sdks/issues/29)) ([4de5eaa](https://github.com/launchdarkly/cpp-sdks/commit/4de5eaaccba0556c4990dceb501277472bab4385))
* Implement flag manager. ([#20](https://github.com/launchdarkly/cpp-sdks/issues/20)) ([15199f1](https://github.com/launchdarkly/cpp-sdks/commit/15199f111f30b06b99f4ce642d1a614d46b629d1))
* Implement http/https requests. ([#27](https://github.com/launchdarkly/cpp-sdks/issues/27)) ([853d3ff](https://github.com/launchdarkly/cpp-sdks/commit/853d3ff5a4148a9d3ed933d2a23dc8609c75d36b))
* implement Identify method ([#89](https://github.com/launchdarkly/cpp-sdks/issues/89)) ([6ab8e82](https://github.com/launchdarkly/cpp-sdks/commit/6ab8e82522ae9eadb4a6c0db60b4d867da34c472))
* Implement logging. ([#5](https://github.com/launchdarkly/cpp-sdks/issues/5)) ([fadd3a0](https://github.com/launchdarkly/cpp-sdks/commit/fadd3a00a336a844de4e14e93ef268318571ea67))
* Implement polling data source. ([#28](https://github.com/launchdarkly/cpp-sdks/issues/28)) ([7ef503b](https://github.com/launchdarkly/cpp-sdks/commit/7ef503bdcafcf203e63f8faf8431f0baf019c2ee))
* implement remaining config C bindings ([#90](https://github.com/launchdarkly/cpp-sdks/issues/90)) ([1b1e66a](https://github.com/launchdarkly/cpp-sdks/commit/1b1e66aee27b1e09e630072dbc5abed29f4de6a3))
* Implement streaming data source. ([#17](https://github.com/launchdarkly/cpp-sdks/issues/17)) ([9931b96](https://github.com/launchdarkly/cpp-sdks/commit/9931b96f73847d5a1b4456fd4f463d43dade5c1b))
* Include version in user-agent. ([#98](https://github.com/launchdarkly/cpp-sdks/issues/98)) ([a33daac](https://github.com/launchdarkly/cpp-sdks/commit/a33daac78b5e64c3419a4a97bf29b638b679784c))
* incomplete C bindings for client configuration ([#45](https://github.com/launchdarkly/cpp-sdks/issues/45)) ([219b9f8](https://github.com/launchdarkly/cpp-sdks/commit/219b9f836651ad794acbcf33a05cb3c13fe7418a))
* make EvaluationReason use enums for Kind and ErrorKind ([#40](https://github.com/launchdarkly/cpp-sdks/issues/40)) ([c330bb8](https://github.com/launchdarkly/cpp-sdks/commit/c330bb89907932bb4b8076a52be60756f84810a8))
* minimal SDK contract tests ([#52](https://github.com/launchdarkly/cpp-sdks/issues/52)) ([5bcf735](https://github.com/launchdarkly/cpp-sdks/commit/5bcf7359471ed71bba353d6bfdfc0205e83d8313))
* Reorganize code to better facilitate encapsulation. ([#87](https://github.com/launchdarkly/cpp-sdks/issues/87)) ([94f94ae](https://github.com/launchdarkly/cpp-sdks/commit/94f94aee4b8961a3001afd39f936e9c744fd9759))
* replace Encrypted/Plain clients with foxy library ([#39](https://github.com/launchdarkly/cpp-sdks/issues/39)) ([33e92df](https://github.com/launchdarkly/cpp-sdks/commit/33e92df2e970c607bead4a912fc737027750c8fb))
* Support handling invalid URLs for asio requests. ([#30](https://github.com/launchdarkly/cpp-sdks/issues/30)) ([64b8aaf](https://github.com/launchdarkly/cpp-sdks/commit/64b8aafdbac07fbf2a82f1bb9fde762c63fd79e7))
* Update windows static builds. ([#103](https://github.com/launchdarkly/cpp-sdks/issues/103)) ([5d08380](https://github.com/launchdarkly/cpp-sdks/commit/5d0838099f7a99de49a604a9b5133325959705ff))
* Use object libraries. ([#99](https://github.com/launchdarkly/cpp-sdks/issues/99)) ([1d848e5](https://github.com/launchdarkly/cpp-sdks/commit/1d848e552def961a0468bfb6bab33cb1c4a86d3b))


### Bug Fixes

* add operator== and operator!= for Value::Array and Value::Object ([#38](https://github.com/launchdarkly/cpp-sdks/issues/38)) ([71759de](https://github.com/launchdarkly/cpp-sdks/commit/71759de48fb06b997b2e6a6c0f76c6a5d0e3f3a1))
* debug event timezone handling ([#79](https://github.com/launchdarkly/cpp-sdks/issues/79)) ([548d750](https://github.com/launchdarkly/cpp-sdks/commit/548d750613343f4add4106704eab29cf75d375f7))
* make AttributeReference's SetType be a std::set ([#23](https://github.com/launchdarkly/cpp-sdks/issues/23)) ([e5eaf22](https://github.com/launchdarkly/cpp-sdks/commit/e5eaf2207dcb34b877421c02346a4c3470976d1b))
* remove c++20 designated initializers ([#42](https://github.com/launchdarkly/cpp-sdks/issues/42)) ([949962a](https://github.com/launchdarkly/cpp-sdks/commit/949962a642938d2d5ceecc3927c65565d3fbc719))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-sse-client bumped from 0.0.0 to 0.1.0
