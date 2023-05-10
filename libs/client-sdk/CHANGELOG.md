# Changelog

## 0.1.0 (2023-05-10)


### Features

* Add client support for streaming. ([#25](https://github.com/launchdarkly/cpp-sdks-private/issues/25)) ([0e3c1f2](https://github.com/launchdarkly/cpp-sdks-private/commit/0e3c1f21dc1bf76451284e4e7f7f61cf1c503eb6))
* Add support for redirection requests. ([#31](https://github.com/launchdarkly/cpp-sdks-private/issues/31)) ([ba5c5ae](https://github.com/launchdarkly/cpp-sdks-private/commit/ba5c5aebe45b5e6bab4fff9b859d83ad2bb58afa))
* eventsource client ([#1](https://github.com/launchdarkly/cpp-sdks-private/issues/1)) ([ab2b0fe](https://github.com/launchdarkly/cpp-sdks-private/commit/ab2b0feb50ef9f607d19c29ed2dd648f3c47b472))
* generate analytic events from evaluations ([#36](https://github.com/launchdarkly/cpp-sdks-private/issues/36)) ([c62dcf6](https://github.com/launchdarkly/cpp-sdks-private/commit/c62dcf69673ef2fcae2dc2f2d143cf0b0f15d076))
* implement Client type ([#21](https://github.com/launchdarkly/cpp-sdks-private/issues/21)) ([10265fd](https://github.com/launchdarkly/cpp-sdks-private/commit/10265fda24191172145f0f22e9f82321f2e3dc6b))
* implement event delivery  ([#29](https://github.com/launchdarkly/cpp-sdks-private/issues/29)) ([4de5eaa](https://github.com/launchdarkly/cpp-sdks-private/commit/4de5eaaccba0556c4990dceb501277472bab4385))
* Implement flag manager. ([#20](https://github.com/launchdarkly/cpp-sdks-private/issues/20)) ([15199f1](https://github.com/launchdarkly/cpp-sdks-private/commit/15199f111f30b06b99f4ce642d1a614d46b629d1))
* Implement http/https requests. ([#27](https://github.com/launchdarkly/cpp-sdks-private/issues/27)) ([853d3ff](https://github.com/launchdarkly/cpp-sdks-private/commit/853d3ff5a4148a9d3ed933d2a23dc8609c75d36b))
* Implement polling data source. ([#28](https://github.com/launchdarkly/cpp-sdks-private/issues/28)) ([7ef503b](https://github.com/launchdarkly/cpp-sdks-private/commit/7ef503bdcafcf203e63f8faf8431f0baf019c2ee))
* Implement streaming data source. ([#17](https://github.com/launchdarkly/cpp-sdks-private/issues/17)) ([9931b96](https://github.com/launchdarkly/cpp-sdks-private/commit/9931b96f73847d5a1b4456fd4f463d43dade5c1b))
* make EvaluationReason use enums for Kind and ErrorKind ([#40](https://github.com/launchdarkly/cpp-sdks-private/issues/40)) ([c330bb8](https://github.com/launchdarkly/cpp-sdks-private/commit/c330bb89907932bb4b8076a52be60756f84810a8))
* replace Encrypted/Plain clients with foxy library ([#39](https://github.com/launchdarkly/cpp-sdks-private/issues/39)) ([33e92df](https://github.com/launchdarkly/cpp-sdks-private/commit/33e92df2e970c607bead4a912fc737027750c8fb))
* Support flag change notifications. ([#41](https://github.com/launchdarkly/cpp-sdks-private/issues/41)) ([24c6cd8](https://github.com/launchdarkly/cpp-sdks-private/commit/24c6cd81cea678bdb6930600a919b1bc5a698c88))
* Support handling invalid URLs for asio requests. ([#30](https://github.com/launchdarkly/cpp-sdks-private/issues/30)) ([64b8aaf](https://github.com/launchdarkly/cpp-sdks-private/commit/64b8aafdbac07fbf2a82f1bb9fde762c63fd79e7))


### Bug Fixes

* remove extra call to data_source-&gt;Start() in api.cpp ([#37](https://github.com/launchdarkly/cpp-sdks-private/issues/37)) ([33458a4](https://github.com/launchdarkly/cpp-sdks-private/commit/33458a4f6f7558cca6c4bce721b3d70be5d524f5))

## 0.1.0 (2023-05-10)


### Features

* Add client support for streaming. ([#25](https://github.com/launchdarkly/cpp-sdks-private/issues/25)) ([0e3c1f2](https://github.com/launchdarkly/cpp-sdks-private/commit/0e3c1f21dc1bf76451284e4e7f7f61cf1c503eb6))
* Add support for redirection requests. ([#31](https://github.com/launchdarkly/cpp-sdks-private/issues/31)) ([ba5c5ae](https://github.com/launchdarkly/cpp-sdks-private/commit/ba5c5aebe45b5e6bab4fff9b859d83ad2bb58afa))
* eventsource client ([#1](https://github.com/launchdarkly/cpp-sdks-private/issues/1)) ([ab2b0fe](https://github.com/launchdarkly/cpp-sdks-private/commit/ab2b0feb50ef9f607d19c29ed2dd648f3c47b472))
* generate analytic events from evaluations ([#36](https://github.com/launchdarkly/cpp-sdks-private/issues/36)) ([c62dcf6](https://github.com/launchdarkly/cpp-sdks-private/commit/c62dcf69673ef2fcae2dc2f2d143cf0b0f15d076))
* implement Client type ([#21](https://github.com/launchdarkly/cpp-sdks-private/issues/21)) ([10265fd](https://github.com/launchdarkly/cpp-sdks-private/commit/10265fda24191172145f0f22e9f82321f2e3dc6b))
* implement event delivery  ([#29](https://github.com/launchdarkly/cpp-sdks-private/issues/29)) ([4de5eaa](https://github.com/launchdarkly/cpp-sdks-private/commit/4de5eaaccba0556c4990dceb501277472bab4385))
* Implement flag manager. ([#20](https://github.com/launchdarkly/cpp-sdks-private/issues/20)) ([15199f1](https://github.com/launchdarkly/cpp-sdks-private/commit/15199f111f30b06b99f4ce642d1a614d46b629d1))
* Implement http/https requests. ([#27](https://github.com/launchdarkly/cpp-sdks-private/issues/27)) ([853d3ff](https://github.com/launchdarkly/cpp-sdks-private/commit/853d3ff5a4148a9d3ed933d2a23dc8609c75d36b))
* Implement polling data source. ([#28](https://github.com/launchdarkly/cpp-sdks-private/issues/28)) ([7ef503b](https://github.com/launchdarkly/cpp-sdks-private/commit/7ef503bdcafcf203e63f8faf8431f0baf019c2ee))
* Implement streaming data source. ([#17](https://github.com/launchdarkly/cpp-sdks-private/issues/17)) ([9931b96](https://github.com/launchdarkly/cpp-sdks-private/commit/9931b96f73847d5a1b4456fd4f463d43dade5c1b))
* make EvaluationReason use enums for Kind and ErrorKind ([#40](https://github.com/launchdarkly/cpp-sdks-private/issues/40)) ([c330bb8](https://github.com/launchdarkly/cpp-sdks-private/commit/c330bb89907932bb4b8076a52be60756f84810a8))
* replace Encrypted/Plain clients with foxy library ([#39](https://github.com/launchdarkly/cpp-sdks-private/issues/39)) ([33e92df](https://github.com/launchdarkly/cpp-sdks-private/commit/33e92df2e970c607bead4a912fc737027750c8fb))
* Support flag change notifications. ([#41](https://github.com/launchdarkly/cpp-sdks-private/issues/41)) ([24c6cd8](https://github.com/launchdarkly/cpp-sdks-private/commit/24c6cd81cea678bdb6930600a919b1bc5a698c88))
* Support handling invalid URLs for asio requests. ([#30](https://github.com/launchdarkly/cpp-sdks-private/issues/30)) ([64b8aaf](https://github.com/launchdarkly/cpp-sdks-private/commit/64b8aafdbac07fbf2a82f1bb9fde762c63fd79e7))


### Bug Fixes

* remove extra call to data_source-&gt;Start() in api.cpp ([#37](https://github.com/launchdarkly/cpp-sdks-private/issues/37)) ([33458a4](https://github.com/launchdarkly/cpp-sdks-private/commit/33458a4f6f7558cca6c4bce721b3d70be5d524f5))
