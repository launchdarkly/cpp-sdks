# Changelog

## [3.3.6](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.3.5...launchdarkly-cpp-server-v3.3.6) (2024-04-05)


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.6.0 to 0.6.1
    * launchdarkly-cpp-sse-client bumped from 0.3.1 to 0.3.2

## [3.3.5](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.3.4...launchdarkly-cpp-server-v3.3.5) (2024-04-04)


### Bug Fixes

* Evaluate should not share EvaluationStack between calls ([#374](https://github.com/launchdarkly/cpp-sdks/issues/374)) ([7fd64ef](https://github.com/launchdarkly/cpp-sdks/commit/7fd64efa028f87306c73fe2fd09ee18683ec24f2))

## [3.3.4](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.3.3...launchdarkly-cpp-server-v3.3.4) (2024-04-03)


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-sse-client bumped from 0.3.0 to 0.3.1

## [3.3.3](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.3.2...launchdarkly-cpp-server-v3.3.3) (2024-03-18)


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.5.4 to 0.6.0

## [3.3.2](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.3.1...launchdarkly-cpp-server-v3.3.2) (2024-02-15)


### Bug Fixes

* server sdk should have CPPServer user agent ([#371](https://github.com/launchdarkly/cpp-sdks/issues/371)) ([b403105](https://github.com/launchdarkly/cpp-sdks/commit/b403105f919e42dfb9664cce805b459bd740a4b3))

## [3.3.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.3.0...launchdarkly-cpp-server-v3.3.1) (2024-01-19)


### Bug Fixes

* add missing &lt;cstdint&gt; in various headers ([#360](https://github.com/launchdarkly/cpp-sdks/issues/360)) ([2d9351c](https://github.com/launchdarkly/cpp-sdks/commit/2d9351c6f584881b7164258785270e5926f4db4c))

## [3.3.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.2.0...launchdarkly-cpp-server-v3.3.0) (2023-12-26)


### Features

* add LDAllFlagsState_Map C binding ([#350](https://github.com/launchdarkly/cpp-sdks/issues/350)) ([2aca898](https://github.com/launchdarkly/cpp-sdks/commit/2aca898074b16cbb34498c289869b7687413df51))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.5.3 to 0.5.4
    * launchdarkly-cpp-common bumped from 1.4.0 to 1.5.0

## [3.2.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.1.0...launchdarkly-cpp-server-v3.2.0) (2023-12-22)


### Features

* redis data source C bindings ([#345](https://github.com/launchdarkly/cpp-sdks/issues/345)) ([03b7de1](https://github.com/launchdarkly/cpp-sdks/commit/03b7de195febdcd4739d670448f5aefcbc2e9a2d))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.5.2 to 0.5.3
    * launchdarkly-cpp-common bumped from 1.3.0 to 1.4.0

## [3.1.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.0.1...launchdarkly-cpp-server-v3.1.0) (2023-12-21)


### Features

* add C binding for context keys cache capacity configuration ([#346](https://github.com/launchdarkly/cpp-sdks/issues/346)) ([8793fc4](https://github.com/launchdarkly/cpp-sdks/commit/8793fc446d24fb1fe4999daa2557e5ded2bbecbf))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.5.1 to 0.5.2
    * launchdarkly-cpp-common bumped from 1.2.0 to 1.3.0

## [3.0.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v3.0.0...launchdarkly-cpp-server-v3.0.1) (2023-12-13)


### Bug Fixes

* double variation was returning ints ([#335](https://github.com/launchdarkly/cpp-sdks/issues/335)) ([ef0559d](https://github.com/launchdarkly/cpp-sdks/commit/ef0559d0bfe4a662cfe558a73afed66a9db9d3b5))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.5.0 to 0.5.1
    * launchdarkly-cpp-common bumped from 1.1.0 to 1.2.0

## [3.0.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v0.3.0...launchdarkly-cpp-server-v3.0.0) (2023-12-04)


### Features

* server sdk 3.0 ([#324](https://github.com/launchdarkly/cpp-sdks/issues/324)) ([fb407d8](https://github.com/launchdarkly/cpp-sdks/commit/fb407d8ad2b681a95799f63896d1c03964026b01))

## [0.3.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v0.2.1...launchdarkly-cpp-server-v0.3.0) (2023-12-04)


### Features

* server-side data system ([#304](https://github.com/launchdarkly/cpp-sdks/issues/304)) ([9a3737d](https://github.com/launchdarkly/cpp-sdks/commit/9a3737d09b1e1e57e5c7e6d30fb0c92f606d284c))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.4.0 to 0.5.0
    * launchdarkly-cpp-common bumped from 1.0.0 to 1.1.0
    * launchdarkly-cpp-sse-client bumped from 0.2.0 to 0.3.0

## [0.2.1](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v0.2.0...launchdarkly-cpp-server-v0.2.1) (2023-12-04)


### Bug Fixes

* remove Boost::disable_autolinking from client and server linking ([#316](https://github.com/launchdarkly/cpp-sdks/issues/316)) ([e84c6a0](https://github.com/launchdarkly/cpp-sdks/commit/e84c6a071553b128436e6dd1bb664f0fd752e4d1))

## [0.2.0](https://github.com/launchdarkly/cpp-sdks/compare/launchdarkly-cpp-server-v0.1.0...launchdarkly-cpp-server-v0.2.0) (2023-11-29)


### ⚠ BREAKING CHANGES

* move server side config into lib/server ([#283](https://github.com/launchdarkly/cpp-sdks/issues/283))

### Code Refactoring

* move server side config into lib/server ([#283](https://github.com/launchdarkly/cpp-sdks/issues/283)) ([c58de8f](https://github.com/launchdarkly/cpp-sdks/commit/c58de8f3914bf83fa8662cccf5b284de3179852d))


### Dependencies

* The following workspace dependencies were updated
  * dependencies
    * launchdarkly-cpp-internal bumped from 0.3.0 to 0.4.0
    * launchdarkly-cpp-common bumped from 0.5.0 to 1.0.0

## 0.1.0 (2023-10-25)


### Features

* server-side SDK  ([#160](https://github.com/launchdarkly/cpp-sdks/issues/160)) ([75eece3](https://github.com/launchdarkly/cpp-sdks/commit/75eece3a46870fdb6bf4384c112700558099c4d1))


### Bug Fixes

* allow for installing only the client or server SDK independently ([#269](https://github.com/launchdarkly/cpp-sdks/issues/269)) ([fe08c3c](https://github.com/launchdarkly/cpp-sdks/commit/fe08c3c14600c712ba6480f671fc306eca320044))
