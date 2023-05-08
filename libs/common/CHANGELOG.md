# Changelog

## [0.2.0](https://github.com/launchdarkly/cpp-sdks-private/compare/v0.1.0...v0.2.0) (2023-05-08)


### Features

* add ApplicationInfo ([#6](https://github.com/launchdarkly/cpp-sdks-private/issues/6)) ([17899c1](https://github.com/launchdarkly/cpp-sdks-private/commit/17899c173d319be4a2d096f0ac2212cf9de094cd))
* Add attribute reference support. ([#7](https://github.com/launchdarkly/cpp-sdks-private/issues/7)) ([2203788](https://github.com/launchdarkly/cpp-sdks-private/commit/2203788c658cd1548e2285773652b8420c09bc1b))
* add console backend constructor that gets level from env ([#6](https://github.com/launchdarkly/cpp-sdks-private/issues/6)) ([14f4161](https://github.com/launchdarkly/cpp-sdks-private/commit/14f4161cd3dea5b32d5a1b5eca320377066e0ea0))
* Add context type, value type, and associated builders. ([#5](https://github.com/launchdarkly/cpp-sdks-private/issues/5)) ([b6edb69](https://github.com/launchdarkly/cpp-sdks-private/commit/b6edb6952497eb4171bc8a63506a408a2f85a969))
* add error.hpp and tl::expected library ([#9](https://github.com/launchdarkly/cpp-sdks-private/issues/9)) ([1b4ddc8](https://github.com/launchdarkly/cpp-sdks-private/commit/1b4ddc8587ba8311626e2e07ef725d8164f22cb1))
* add event summarizer ([#19](https://github.com/launchdarkly/cpp-sdks-private/issues/19)) ([3c4845a](https://github.com/launchdarkly/cpp-sdks-private/commit/3c4845a0066ed65078969dd26f423e14d1e70843))
* add ServiceEndpoints builder & struct [1/2] ([#2](https://github.com/launchdarkly/cpp-sdks-private/issues/2)) ([10763e7](https://github.com/launchdarkly/cpp-sdks-private/commit/10763e77f5ed6a637554c9c3af6564a115b538ce))
* Add support for redirection requests. ([#31](https://github.com/launchdarkly/cpp-sdks-private/issues/31)) ([ba5c5ae](https://github.com/launchdarkly/cpp-sdks-private/commit/ba5c5aebe45b5e6bab4fff9b859d83ad2bb58afa))
* Change builder to allow for better loops. ([#8](https://github.com/launchdarkly/cpp-sdks-private/issues/8)) ([0b00f28](https://github.com/launchdarkly/cpp-sdks-private/commit/0b00f283d12512a13d8bcdc288b2dfde845a2673))
* eventsource client ([#1](https://github.com/launchdarkly/cpp-sdks-private/issues/1)) ([ab2b0fe](https://github.com/launchdarkly/cpp-sdks-private/commit/ab2b0feb50ef9f607d19c29ed2dd648f3c47b472))
* foundation of an event processor ([#16](https://github.com/launchdarkly/cpp-sdks-private/issues/16)) ([356bde1](https://github.com/launchdarkly/cpp-sdks-private/commit/356bde11a8b2b66578cc435c019e0a549528d560))
* generate analytic events from evaluations ([#36](https://github.com/launchdarkly/cpp-sdks-private/issues/36)) ([c62dcf6](https://github.com/launchdarkly/cpp-sdks-private/commit/c62dcf69673ef2fcae2dc2f2d143cf0b0f15d076))
* Implement C-binding for Value type. ([#33](https://github.com/launchdarkly/cpp-sdks-private/issues/33)) ([afb943c](https://github.com/launchdarkly/cpp-sdks-private/commit/afb943cb3d8a6b214935087fdd147b74a8a38361))
* Implement client flag data model. ([#12](https://github.com/launchdarkly/cpp-sdks-private/issues/12)) ([ce7ccbc](https://github.com/launchdarkly/cpp-sdks-private/commit/ce7ccbc7356b2c5a9a9318109041a28524e6f9d2))
* implement Client type ([#21](https://github.com/launchdarkly/cpp-sdks-private/issues/21)) ([10265fd](https://github.com/launchdarkly/cpp-sdks-private/commit/10265fda24191172145f0f22e9f82321f2e3dc6b))
* Implement context filtering and JSON serialization. ([#11](https://github.com/launchdarkly/cpp-sdks-private/issues/11)) ([074c691](https://github.com/launchdarkly/cpp-sdks-private/commit/074c6914165987522653e100df1b8b0911bb8565))
* Implement data source configuration. ([#18](https://github.com/launchdarkly/cpp-sdks-private/issues/18)) ([d2cbf8e](https://github.com/launchdarkly/cpp-sdks-private/commit/d2cbf8ebd049df59742ca2d864e8449a3c4519d6))
* implement event delivery  ([#29](https://github.com/launchdarkly/cpp-sdks-private/issues/29)) ([4de5eaa](https://github.com/launchdarkly/cpp-sdks-private/commit/4de5eaaccba0556c4990dceb501277472bab4385))
* Implement flag manager. ([#20](https://github.com/launchdarkly/cpp-sdks-private/issues/20)) ([15199f1](https://github.com/launchdarkly/cpp-sdks-private/commit/15199f111f30b06b99f4ce642d1a614d46b629d1))
* Implement http/https requests. ([#27](https://github.com/launchdarkly/cpp-sdks-private/issues/27)) ([853d3ff](https://github.com/launchdarkly/cpp-sdks-private/commit/853d3ff5a4148a9d3ed933d2a23dc8609c75d36b))
* Implement logging. ([#5](https://github.com/launchdarkly/cpp-sdks-private/issues/5)) ([fadd3a0](https://github.com/launchdarkly/cpp-sdks-private/commit/fadd3a00a336a844de4e14e93ef268318571ea67))
* Implement polling data source. ([#28](https://github.com/launchdarkly/cpp-sdks-private/issues/28)) ([7ef503b](https://github.com/launchdarkly/cpp-sdks-private/commit/7ef503bdcafcf203e63f8faf8431f0baf019c2ee))
* Implement streaming data source. ([#17](https://github.com/launchdarkly/cpp-sdks-private/issues/17)) ([9931b96](https://github.com/launchdarkly/cpp-sdks-private/commit/9931b96f73847d5a1b4456fd4f463d43dade5c1b))
* make EvaluationReason use enums for Kind and ErrorKind ([#40](https://github.com/launchdarkly/cpp-sdks-private/issues/40)) ([c330bb8](https://github.com/launchdarkly/cpp-sdks-private/commit/c330bb89907932bb4b8076a52be60756f84810a8))
* replace Encrypted/Plain clients with foxy library ([#39](https://github.com/launchdarkly/cpp-sdks-private/issues/39)) ([33e92df](https://github.com/launchdarkly/cpp-sdks-private/commit/33e92df2e970c607bead4a912fc737027750c8fb))
* Support handling invalid URLs for asio requests. ([#30](https://github.com/launchdarkly/cpp-sdks-private/issues/30)) ([64b8aaf](https://github.com/launchdarkly/cpp-sdks-private/commit/64b8aafdbac07fbf2a82f1bb9fde762c63fd79e7))


### Bug Fixes

* add operator== and operator!= for Value::Array and Value::Object ([#38](https://github.com/launchdarkly/cpp-sdks-private/issues/38)) ([71759de](https://github.com/launchdarkly/cpp-sdks-private/commit/71759de48fb06b997b2e6a6c0f76c6a5d0e3f3a1))
* make AttributeReference's SetType be a std::set ([#23](https://github.com/launchdarkly/cpp-sdks-private/issues/23)) ([e5eaf22](https://github.com/launchdarkly/cpp-sdks-private/commit/e5eaf2207dcb34b877421c02346a4c3470976d1b))
* remove c++20 designated initializers ([#42](https://github.com/launchdarkly/cpp-sdks-private/issues/42)) ([949962a](https://github.com/launchdarkly/cpp-sdks-private/commit/949962a642938d2d5ceecc3927c65565d3fbc719))


### Dependencies

* The following workspace dependencies were updated
    * launchdarkly-sse:launchdarkly-sse bumped to 0.2.0

## 0.1.0 (2023-05-08)


### Features

* add ApplicationInfo ([#6](https://github.com/launchdarkly/cpp-sdks-private/issues/6)) ([17899c1](https://github.com/launchdarkly/cpp-sdks-private/commit/17899c173d319be4a2d096f0ac2212cf9de094cd))
* Add attribute reference support. ([#7](https://github.com/launchdarkly/cpp-sdks-private/issues/7)) ([2203788](https://github.com/launchdarkly/cpp-sdks-private/commit/2203788c658cd1548e2285773652b8420c09bc1b))
* add console backend constructor that gets level from env ([#6](https://github.com/launchdarkly/cpp-sdks-private/issues/6)) ([14f4161](https://github.com/launchdarkly/cpp-sdks-private/commit/14f4161cd3dea5b32d5a1b5eca320377066e0ea0))
* Add context type, value type, and associated builders. ([#5](https://github.com/launchdarkly/cpp-sdks-private/issues/5)) ([b6edb69](https://github.com/launchdarkly/cpp-sdks-private/commit/b6edb6952497eb4171bc8a63506a408a2f85a969))
* add error.hpp and tl::expected library ([#9](https://github.com/launchdarkly/cpp-sdks-private/issues/9)) ([1b4ddc8](https://github.com/launchdarkly/cpp-sdks-private/commit/1b4ddc8587ba8311626e2e07ef725d8164f22cb1))
* add event summarizer ([#19](https://github.com/launchdarkly/cpp-sdks-private/issues/19)) ([3c4845a](https://github.com/launchdarkly/cpp-sdks-private/commit/3c4845a0066ed65078969dd26f423e14d1e70843))
* add ServiceEndpoints builder & struct [1/2] ([#2](https://github.com/launchdarkly/cpp-sdks-private/issues/2)) ([10763e7](https://github.com/launchdarkly/cpp-sdks-private/commit/10763e77f5ed6a637554c9c3af6564a115b538ce))
* Add support for redirection requests. ([#31](https://github.com/launchdarkly/cpp-sdks-private/issues/31)) ([ba5c5ae](https://github.com/launchdarkly/cpp-sdks-private/commit/ba5c5aebe45b5e6bab4fff9b859d83ad2bb58afa))
* Change builder to allow for better loops. ([#8](https://github.com/launchdarkly/cpp-sdks-private/issues/8)) ([0b00f28](https://github.com/launchdarkly/cpp-sdks-private/commit/0b00f283d12512a13d8bcdc288b2dfde845a2673))
* eventsource client ([#1](https://github.com/launchdarkly/cpp-sdks-private/issues/1)) ([ab2b0fe](https://github.com/launchdarkly/cpp-sdks-private/commit/ab2b0feb50ef9f607d19c29ed2dd648f3c47b472))
* foundation of an event processor ([#16](https://github.com/launchdarkly/cpp-sdks-private/issues/16)) ([356bde1](https://github.com/launchdarkly/cpp-sdks-private/commit/356bde11a8b2b66578cc435c019e0a549528d560))
* generate analytic events from evaluations ([#36](https://github.com/launchdarkly/cpp-sdks-private/issues/36)) ([c62dcf6](https://github.com/launchdarkly/cpp-sdks-private/commit/c62dcf69673ef2fcae2dc2f2d143cf0b0f15d076))
* Implement C-binding for Value type. ([#33](https://github.com/launchdarkly/cpp-sdks-private/issues/33)) ([afb943c](https://github.com/launchdarkly/cpp-sdks-private/commit/afb943cb3d8a6b214935087fdd147b74a8a38361))
* Implement client flag data model. ([#12](https://github.com/launchdarkly/cpp-sdks-private/issues/12)) ([ce7ccbc](https://github.com/launchdarkly/cpp-sdks-private/commit/ce7ccbc7356b2c5a9a9318109041a28524e6f9d2))
* implement Client type ([#21](https://github.com/launchdarkly/cpp-sdks-private/issues/21)) ([10265fd](https://github.com/launchdarkly/cpp-sdks-private/commit/10265fda24191172145f0f22e9f82321f2e3dc6b))
* Implement context filtering and JSON serialization. ([#11](https://github.com/launchdarkly/cpp-sdks-private/issues/11)) ([074c691](https://github.com/launchdarkly/cpp-sdks-private/commit/074c6914165987522653e100df1b8b0911bb8565))
* Implement data source configuration. ([#18](https://github.com/launchdarkly/cpp-sdks-private/issues/18)) ([d2cbf8e](https://github.com/launchdarkly/cpp-sdks-private/commit/d2cbf8ebd049df59742ca2d864e8449a3c4519d6))
* implement event delivery  ([#29](https://github.com/launchdarkly/cpp-sdks-private/issues/29)) ([4de5eaa](https://github.com/launchdarkly/cpp-sdks-private/commit/4de5eaaccba0556c4990dceb501277472bab4385))
* Implement flag manager. ([#20](https://github.com/launchdarkly/cpp-sdks-private/issues/20)) ([15199f1](https://github.com/launchdarkly/cpp-sdks-private/commit/15199f111f30b06b99f4ce642d1a614d46b629d1))
* Implement http/https requests. ([#27](https://github.com/launchdarkly/cpp-sdks-private/issues/27)) ([853d3ff](https://github.com/launchdarkly/cpp-sdks-private/commit/853d3ff5a4148a9d3ed933d2a23dc8609c75d36b))
* Implement logging. ([#5](https://github.com/launchdarkly/cpp-sdks-private/issues/5)) ([fadd3a0](https://github.com/launchdarkly/cpp-sdks-private/commit/fadd3a00a336a844de4e14e93ef268318571ea67))
* Implement polling data source. ([#28](https://github.com/launchdarkly/cpp-sdks-private/issues/28)) ([7ef503b](https://github.com/launchdarkly/cpp-sdks-private/commit/7ef503bdcafcf203e63f8faf8431f0baf019c2ee))
* Implement streaming data source. ([#17](https://github.com/launchdarkly/cpp-sdks-private/issues/17)) ([9931b96](https://github.com/launchdarkly/cpp-sdks-private/commit/9931b96f73847d5a1b4456fd4f463d43dade5c1b))
* make EvaluationReason use enums for Kind and ErrorKind ([#40](https://github.com/launchdarkly/cpp-sdks-private/issues/40)) ([c330bb8](https://github.com/launchdarkly/cpp-sdks-private/commit/c330bb89907932bb4b8076a52be60756f84810a8))
* replace Encrypted/Plain clients with foxy library ([#39](https://github.com/launchdarkly/cpp-sdks-private/issues/39)) ([33e92df](https://github.com/launchdarkly/cpp-sdks-private/commit/33e92df2e970c607bead4a912fc737027750c8fb))
* Support handling invalid URLs for asio requests. ([#30](https://github.com/launchdarkly/cpp-sdks-private/issues/30)) ([64b8aaf](https://github.com/launchdarkly/cpp-sdks-private/commit/64b8aafdbac07fbf2a82f1bb9fde762c63fd79e7))


### Bug Fixes

* add operator== and operator!= for Value::Array and Value::Object ([#38](https://github.com/launchdarkly/cpp-sdks-private/issues/38)) ([71759de](https://github.com/launchdarkly/cpp-sdks-private/commit/71759de48fb06b997b2e6a6c0f76c6a5d0e3f3a1))
* make AttributeReference's SetType be a std::set ([#23](https://github.com/launchdarkly/cpp-sdks-private/issues/23)) ([e5eaf22](https://github.com/launchdarkly/cpp-sdks-private/commit/e5eaf2207dcb34b877421c02346a4c3470976d1b))
* remove c++20 designated initializers ([#42](https://github.com/launchdarkly/cpp-sdks-private/issues/42)) ([949962a](https://github.com/launchdarkly/cpp-sdks-private/commit/949962a642938d2d5ceecc3927c65565d3fbc719))
